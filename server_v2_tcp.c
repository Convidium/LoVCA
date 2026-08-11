#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>

#include "protocol.h"
#include "net_utils.h"

typedef enum {
    SERVER_CONTINUE = 0,
    SERVER_STOP = 1
} server_control_t;

typedef struct {
    int fd;
    char nickname[MAX_NICKNAME_LEN + 1];
} client_t;
client_t client_fds[MAX_CLIENTS];

int get_listener_socket(void) {
    int listener_fd;
    struct addrinfo hints, *servinfo;
    int yes = 1;

    int addr_status;
    int bind_status;
    int listen_status;

    memset(&hints, 0, sizeof hints);
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;

    addr_status = getaddrinfo(NULL, PORT, &hints, &servinfo);
    if (addr_status != 0) {
        fprintf(stderr, "Error: getaddrinfo: %s\n", gai_strerror(addr_status));
        return -1;
    }

    listener_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (listener_fd == -1) {
        perror("Error: socket()");
        freeaddrinfo(servinfo);
        return -1;
    }

    setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    bind_status = bind(listener_fd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (bind_status != 0) {
        perror("Error: bind()");
        close(listener_fd);
        freeaddrinfo(servinfo);
        return -1;
    }

    freeaddrinfo(servinfo);

    listen_status = listen(listener_fd, BACKLOG);
    if (listen_status != 0) {
        perror("Error: listen()");
        close(listener_fd);
        return -1;
    }

    return listener_fd;
}

void handle_new_connection(int listener) {
    struct sockaddr_storage remote_addr; // Client address
    socklen_t addrlen;
    int remote_fd;

    char remote_ip[INET6_ADDRSTRLEN];
    int remote_port;

    addrlen = sizeof remote_addr;
    remote_fd = accept(listener, (struct sockaddr *)&remote_addr, &addrlen);
    if (remote_fd == -1) {
        perror("Error: accept()");
        return;
    }
    
    msg_header_t header;
    int bytes = recv(remote_fd, &header, sizeof(header), 0);
    if (bytes <= 0 || header.type != MSG_AUTH) {
        printf("\n{-} Client failed authentication header. Closing socket %d.\n", remote_fd);
        close(remote_fd);
        return;
    }

    uint16_t name_len = ntohs(header.length);
    if (name_len == 0 || name_len > MAX_NICKNAME_LEN) {
        printf("\n{-} Invalid nickname length (%d bytes). Connection rejected.\n", name_len);
        close(remote_fd);
        return;
    }

    char temp_nick[MAX_NICKNAME_LEN + 1];
    int nick_bytes = recv(remote_fd, &temp_nick, name_len, 0);
    if (nick_bytes <= 0) {
        close(remote_fd);
        return;
    }
    temp_nick[nick_bytes] = '\0';

    sockaddr_get_ip((struct sockaddr *)&remote_addr, remote_ip, sizeof remote_ip);
    remote_port = sockaddr_get_port((struct sockaddr *)&remote_addr);
    
    int added = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i].fd == -1) {
            client_fds[i].fd = remote_fd;
            strncpy(client_fds[i].nickname, temp_nick, MAX_NICKNAME_LEN);
            client_fds[i].nickname[MAX_NICKNAME_LEN] = '\0';

            printf("\n{+} New client '%s' connected from: %s:%d (Slot %d)\n", client_fds[i].nickname, remote_ip, remote_port, i);
            added = 1;
            break;
        }
    }
    if (!added) {
        printf("\n{-} Server full! Connection rejected for '%s'.\n", temp_nick);
        close(remote_fd);
    }
}

void handle_client_message(int slot_index) {
    int client_fd = client_fds[slot_index].fd;
    msg_header_t header;

    int bytes = recv(client_fd, &header, sizeof(header), 0);
    if (bytes <= 0 || header.type != MSG_TEXT) {
        printf("\n{-} Client failed to send a message.\n");
        return;
    }

    uint16_t message_len = ntohs(header.length);
    if (message_len == 0 || message_len > MAX_BUFF_LEN) {
        printf("\n{-} Invalid message length (%d bytes)\n", message_len);
        return;
    }

    char message[MAX_BUFF_LEN + 1];
    int message_bytes = recv(client_fd, &message, message_len, 0);
    if (message_bytes <= 0) {
        return;
    }
    message[message_bytes] = '\0';
    printf("\nClient (%d) {%s} > %s\n", slot_index, client_fds[slot_index].nickname, message);
}

server_control_t handle_stdin_input(void) {
    char send_buf[MAX_BUFF_LEN];
    if (fgets(send_buf, sizeof send_buf, stdin) == NULL) {
        clearerr(stdin);
        printf("\n[Server] EOF detected on stdin. Exiting server...\n");
        return SERVER_STOP;
    }

    send_buf[strcspn(send_buf, "\r\n")] = '\0';

    if (strcmp(send_buf, "exit") == 0) {
        printf("Exiting server...\n");
        return SERVER_STOP;
    }

    return SERVER_CONTINUE;
}

server_control_t process_connections(struct pollfd fds[], int listen_fd) {
    if (fds[0].revents & (POLLIN | POLLHUP)) {
        return handle_stdin_input();
    }

    if (fds[1].revents & POLLIN) {
        handle_new_connection(listen_fd);
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i].fd != -1 && 
            (fds[i + 2].revents & (POLLIN | POLLHUP | POLLERR))) 
        {
            handle_client_message(i);
        }
    }

    return SERVER_CONTINUE;
}

int main (void) {
    signal(SIGPIPE, SIG_IGN);

    int listener_fd = get_listener_socket();
    if (listener_fd == -1) {
        fprintf(stderr, "Failed to initialize listener socket.\n");
        return 1;
    }

    printf("=== Server started on port %s ===\n", PORT);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i].fd = -1;
        client_fds[i].nickname[0] = '\0';
    }

    struct pollfd fds[MAX_CLIENTS+2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = listener_fd;
    fds[1].events = POLLIN;

    int running = 1;
    while (running) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            fds[i + 2].fd = client_fds[i].fd;
            fds[i + 2].events = (client_fds[i].fd != -1) ? POLLIN : 0;
        }

        int poll_count = poll(fds, MAX_CLIENTS+2, -1);
        if (poll_count == -1) {
            perror("Error: poll()");
            break;
        }

        server_control_t status = process_connections(fds, listener_fd);
        if (status == SERVER_STOP) {
            running = 0;
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i].fd != -1) close(client_fds[i].fd);
    }
    close(listener_fd);
    return 0;
}
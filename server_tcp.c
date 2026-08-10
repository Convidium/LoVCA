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

#define PORT "8841"
#define BACKLOG 10
#define MAX_CLIENTS 20
#define MAX_BUFF_LEN 256

#include "protocol.h"

typedef enum {
    SERVER_CONTINUE = 0,
    SERVER_STOP = 1
} server_control_t;

typedef struct {
    int fd;
    char nickname[MAX_NICKNAME_LEN + 1];
} client_t;

client_t clients[MAX_CLIENTS];


// Convert socket to IP address string.
// addr: struct sockaddr_in or struct sockaddr_in6
const char *inet_ntop2(void *addr, char *buf, size_t size) {
    struct sockaddr_storage *sas = addr;
    struct sockaddr_in *sa4;
    struct sockaddr_in6 *sa6;
    void *src;

    switch (sas->ss_family) {
        case AF_INET:
            sa4 = addr;
            src = &(sa4->sin_addr);
            break;
        case AF_INET6:
            sa6 = addr;
            src = &(sa6->sin6_addr);
            break;
        default:
            return NULL;
    }
    return inet_ntop(sas->ss_family, src, buf, size);
}

// Create and establish new socket (fd listener)
int get_listener_socket(void) {
    int listen_fd;
    struct addrinfo hints, *servinfo;
    int yes = 1;

    int addr_status;
    int bind_status;

    memset(&hints, 0, sizeof hints);
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;

    addr_status = getaddrinfo(NULL, PORT, &hints, &servinfo);
    if (addr_status != 0) {
        fprintf(stderr, "Error: getaddrinfo()\n");
        exit(1);
    }

    listen_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (listen_fd == -1) {
        perror("Error: socket()");
        freeaddrinfo(servinfo);
        exit(1);
    }

    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    bind_status = bind(listen_fd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (bind_status == -1) {
        perror("Error: socket()");
        close(listen_fd);
        freeaddrinfo(servinfo);
        exit(1);
    }

    freeaddrinfo(servinfo);

    if (listen(listen_fd, BACKLOG) == -1) {
        perror("Error: listen()");
        close(listen_fd);
        return -1;
    }

    return listen_fd;
}

void handle_incoming_connection(int listener) {
    struct sockaddr_storage new_addr; // Client address
    socklen_t addrlen;
    int newfd;  // Newly accept()'ed socket descriptor
    char remoteIP[INET6_ADDRSTRLEN];
    int client_port = 0;

    addrlen = sizeof new_addr;
    newfd = accept(listener, (struct sockaddr *)&new_addr, &addrlen);

    if (newfd == -1) {
        perror("Error: accept()");
        return;
    }

    msg_header_t header;
    int bytes = recv(newfd, &header, sizeof(header), 0);
    if (bytes <= 0 || header.type != MSG_AUTH) {
        printf("\n{-} Client failed authentication header. Closing socket %d.\n", newfd);
        close(newfd);
        return;
    }

    uint16_t name_len = ntohs(header.length);
    if (name_len == 0 || name_len > MAX_NICKNAME_LEN) {
        printf("\n{-} Invalid nickname length (%d bytes). Connection rejected.\n", name_len);
        close(newfd);
        return;
    }

    char temp_nick[MAX_NICKNAME_LEN + 1];
    int nick_bytes = recv(newfd, &temp_nick, name_len, 0);
    if (nick_bytes <= 0) {
        close(newfd);
        return;
    }
    temp_nick[nick_bytes] = '\0';

    if (new_addr.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&new_addr;
        client_port = ntohs(s->sin_port); // Big-Endian -> Host Order
        inet_ntop(AF_INET, &s->sin_addr, remoteIP, sizeof remoteIP);
    } else { // Else if it's IPv6, i.e. AF_INET6
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&new_addr;
        client_port = ntohs(s->sin6_port); // Big-Endian -> Host Order
        inet_ntop(AF_INET6, &s->sin6_addr, remoteIP, sizeof remoteIP);

    }
    
    int added = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == -1) {
            clients[i].fd = newfd;
            strncpy(clients[i].nickname, temp_nick, MAX_NICKNAME_LEN);
            clients[i].nickname[MAX_NICKNAME_LEN] = '\0';

            printf("\n{+} New client '%s' connected from: %s:%d (Slot %d)\n", clients[i].nickname, remoteIP, client_port, i);
            added = 1;
            break;
        }
    }
    if (!added) {
        printf("\n{-} Server full! Connection rejected for '%s'.\n", temp_nick);
        close(newfd);
    }
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

    int len = strlen(send_buf);
    if (len == 0) return SERVER_CONTINUE;

    int sent_count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1) {
            char msg_with_newline[MAX_BUFF_LEN + 2];
            snprintf(msg_with_newline, sizeof(msg_with_newline), "%s\n", send_buf);

            int bytes = send(clients[i].fd, msg_with_newline, strlen(msg_with_newline), 0);
            
            if (bytes == -1) {
                printf("{-} Failed to send to slot %d, closing connection.\n", i);
                close(clients[i].fd);
                clients[i].fd = -1;
            } else {
                sent_count++;
            }
        }
    }
    printf("Broadcasted to %d clients.\n", sent_count);
    return SERVER_CONTINUE;
}

server_control_t process_connections(struct pollfd fds[], int listen_fd) {
    if (fds[1].revents & POLLIN) {
        handle_incoming_connection(listen_fd);
    }

    if (fds[0].revents & (POLLIN | POLLHUP)) {
        return handle_stdin_input();
    }

    return SERVER_CONTINUE;
}


int main(void) {
    signal(SIGPIPE, SIG_IGN);
    
    int listen_fd = get_listener_socket();
    if (listen_fd == -1) {
        fprintf(stderr, "Failed to initialize listener socket.\n");
        return 1;
    }

    printf("=== Broadcast Chat Server started on port %s ===\n", PORT);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].nickname[0] = '\0';
    }

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = listen_fd;
    fds[1].events = POLLIN;

    int running = 1;
    while (running) {
        printf("Server > ");
        fflush(stdout);

        int poll_count = poll(fds, 2, -1);
        if (poll_count == -1) {
            perror("Error: poll()");
            break;
        }

        server_control_t status = process_connections(fds, listen_fd);
        if (status == SERVER_STOP) {
            running = 0;
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1) close(clients[i].fd);
    }
    close(listen_fd);
    return 0;
}
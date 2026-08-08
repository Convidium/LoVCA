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

typedef enum {
    SERVER_CONTINUE = 0,
    SERVER_STOP = 1
} server_control_t;


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

void handle_incoming_connection(int listener, int client_sockets[]) {
    struct sockaddr_storage new_addr; // Client address
    socklen_t addrlen;
    int newfd;  // Newly accept()'ed socket descriptor

    addrlen = sizeof new_addr;
    newfd = accept(listener, (struct sockaddr *)&new_addr, &addrlen);

    if (newfd == -1) {
        perror("Error: accept()");
        return;
    }
    
    int added = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == -1) {
            client_sockets[i] = newfd;
            printf("\n{+} New client connected! (Slot %d)\n", i);
            added = 1;
            break;
        }
    }
    if (!added) {
        printf("\n{-} Server full! Connection rejected.\n");
        close(newfd);
    }
}

server_control_t handle_stdin_input(int client_sockets[]) {
    char send_buf[MAX_BUFF_LEN];
    if (fgets(send_buf, sizeof send_buf, stdin) == NULL) {
        clearerr(stdin);
        printf("\n[Server] EOF detected on stdin. Exiting server...\n");
        return SERVER_STOP;
    }

    send_buf[strcspn(send_buf, "\n")] = '\0';

    if (strcmp(send_buf, "exit") == 0) {
        printf("Exiting server...\n");
        return SERVER_STOP;
    }

    int len = strlen(send_buf);
    if (len == 0) return SERVER_CONTINUE;

    int sent_count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != -1) {
            char msg_with_newline[MAX_BUFF_LEN + 2];
            snprintf(msg_with_newline, sizeof(msg_with_newline), "%s\n", send_buf);

            int bytes = send(client_sockets[i], msg_with_newline, strlen(msg_with_newline), 0);
            
            if (bytes == -1) {
                printf("{-} Failed to send to slot %d, closing connection.\n", i);
                close(client_sockets[i]);
                client_sockets[i] = -1;
            } else {
                sent_count++;
            }
        }
    }
    printf("Broadcasted to %d clients.\n", sent_count);
    return SERVER_CONTINUE;
}

server_control_t process_connections(struct pollfd fds[], int listen_fd, int client_sockets[]) {
    if (fds[1].revents & POLLIN) {
        handle_incoming_connection(listen_fd, client_sockets);
    }

    if (fds[0].revents & (POLLIN | POLLHUP)) {
        return handle_stdin_input(client_sockets);
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

    int client_sockets[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = -1;
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

        server_control_t status = process_connections(fds, listen_fd, client_sockets);
        if (status == SERVER_STOP) {
            running = 0;
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != -1) close(client_sockets[i]);
    }
    close(listen_fd);
    return 0;
}
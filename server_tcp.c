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

int main(void) {
    signal(SIGPIPE, SIG_IGN);
    
    int listen_fd;
    struct addrinfo hints, *servinfo;

    int addr_status;
    int bind_status;

    memset(&hints, 0, sizeof hints);
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;

    addr_status = getaddrinfo(NULL, PORT, &hints, &servinfo);
    if (addr_status != 0) {
        fprintf(stderr, "Error: getaddrinfo()\n");
        return 1;
    }

    listen_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (listen_fd == -1) {
        perror("Error: socket()");
        freeaddrinfo(servinfo);
        return 2;
    }

    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    bind_status = bind(listen_fd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (bind_status == -1) {
        perror("Error: socket()");
        close(listen_fd);
        freeaddrinfo(servinfo);
        return 3;
    }

    freeaddrinfo(servinfo);

    if (listen(listen_fd, BACKLOG) == -1) {
        perror("Error: listen()");
        close(listen_fd);
        return 4;
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

    char send_buf[MAX_BUFF_LEN];

    while (1) {
        printf("Server > ");
        fflush(stdout);

        int poll_count = poll(fds, 2, -1);
        if (poll_count == -1) {
            perror("Error: poll()");
            break;
        }

        if (fds[1].revents & POLLIN) {
            struct sockaddr_storage client_addr;
            socklen_t addr_len = sizeof client_addr;
            int new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);

            if (new_fd != -1) {
                int added = 0;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (client_sockets[i] == -1) {
                        client_sockets[i] = new_fd;
                        printf("\n{+} New client connected! (Slot %d)\n", i);
                        added = 1;
                        break;
                    }
                }
                if (!added) {
                    printf("\n{-} Server full! Connection rejected.\n");
                    close(new_fd);
                }
            }
        }

        if (fds[0].revents & (POLLIN | POLLHUP)) {
            if (fgets(send_buf, sizeof send_buf, stdin) == NULL) {
                clearerr(stdin);
                printf("\n[Server] EOF detected on stdin. Exiting server...\n");
                continue;
            }

            send_buf[strcspn(send_buf, "\n")] = '\0';

            if (strcmp(send_buf, "exit") == 0) {
                printf("Exiting server...\n");
                break;
            }

            int len = strlen(send_buf);
            if (len == 0) continue;

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
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != -1) close(client_sockets[i]);
    }
    close(listen_fd);
    return 0;
}
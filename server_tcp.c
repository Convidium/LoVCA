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
#define MAX_BUFF_LEN 100

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void broadcast_message(struct pollfd fds[], int sender_fd, const char *msg, int msg_len) {
    for (int i = 1; i < MAX_CLIENTS; i++) {
        if (fds[i].fd != -1 && fds[i].fd != sender_fd) {
            if (send(fds[i].fd, msg, msg_len, 0) == -1) {
                perror("Error: send() in broadcast");
            }
        }
    }
}

int main(void) {
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

    struct pollfd fds[MAX_CLIENTS + 1];

    // Initializing listening socket (us)
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN; // Interested in an event "new connection"

    // We leave the rest of the sockets as -1
    for (int i = 1; i <= MAX_CLIENTS; i++) {
        fds[i].fd = -1;
    }

    char buf[MAX_BUFF_LEN];

    while (1) {
        int poll_count = poll(fds, MAX_CLIENTS+1, -1);
        if (poll_count == -1) {
            perror("Error: poll()");
            break;
        }

        if (fds[0].revents & POLLIN) {
            struct sockaddr_storage client_addr;
            socklen_t addr_len = sizeof client_addr;
            int new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);

            if (new_fd != -1) {
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), client_ip, sizeof client_ip);

                int added = 0;
                for (int i = 0; i <= MAX_CLIENTS; i++) {
                    if (fds[i].fd == -1) {
                        fds[i].fd = new_fd;
                        fds[i].events = POLLIN;
                        printf("[+] New client connected from %s (slot %d)\n", client_ip, i);
                            added = 1;
                            break;
                    }
                }

                if (!added) {
                    printf("{-} Server full! Rejected connection from %s\n", client_ip);
                    const char *full_msg = "Server is full (max 20 clients).\n";
                    send(new_fd, full_msg, strlen(full_msg), 0);
                    close(new_fd);
                }
            }
        }

        for (int i = 1; i <= MAX_CLIENTS; i++) {
            if (fds[i].fd != -1 && (fds[i].revents & POLLIN)) {
                int bytes_received = recv(fds[i].fd, buf, sizeof(buf) - 1, 0);

                if (bytes_received <= 0) {
                    if (bytes_received == 0) {
                        printf("{-} Client on slot %d disconnected.\n", i);
                    } else {
                        perror("Error: recv()");
                    }
                    close(fds[i].fd);
                    fds[i].fd = -1;
                } else {
                    buf[bytes_received] = '\0';
                    printf("[Slot %d]: %s", i, buf);

                    broadcast_message(fds, fds[i].fd, buf, bytes_received);
                }
            }
        }
    }

    close(listen_fd);
    return 0;
}
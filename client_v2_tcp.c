#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <poll.h>

#include <arpa/inet.h>

#include "protocol.h"
#include "net_utils.h"

typedef enum {
    SERVER_CONTINUE = 0,
    SERVER_STOP = 1
} client_control_t;

int get_client_socket(char address[]) {
    int client_fd;
    struct addrinfo hints, *servinfo;
    char server_ip[INET_ADDRSTRLEN];

    int addr_status;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addr_status = getaddrinfo(address, PORT, &hints, &servinfo);
    if (addr_status != 0) {
        fprintf(stderr, "Error: getaddrinfo() %s\n", gai_strerror(addr_status));
        return -1;
    }

    client_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (client_fd == -1) {
        perror("Error: socket()");
        freeaddrinfo(servinfo);
        return -1;
    }

    sockaddr_get_ip((struct sockaddr *)servinfo->ai_addr, server_ip, sizeof server_ip);
    printf("Client: attempting connection to %s...\n", server_ip);

    if (connect(client_fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        perror("Error: connect()");
        close(client_fd);
        freeaddrinfo(servinfo);
        return -1;
    }

    printf("Client: connected to %s\n", server_ip);
    printf("Type 'exit' or press Ctrl+D to disconnect this server.\n\n");

    freeaddrinfo(servinfo);

    return client_fd;
}

int send_auth_message(int client_fd, const char *nickname) {
    msg_header_t header;
    header.type = MSG_AUTH;
    header.length = htons(strlen(nickname));

    if (send(client_fd, &header, sizeof(header), MSG_NOSIGNAL) == -1) {
        return -1;
    }

    if (send(client_fd, nickname, strlen(nickname), MSG_NOSIGNAL) == -1) {
        return -1;
    }

    return 0;
}

int send_text_message(int client_fd, const char *text_message) {
    msg_header_t header;
    header.type = MSG_TEXT;
    header.length = htons(strlen(text_message));

    if (send(client_fd, &header, sizeof(header), MSG_NOSIGNAL) == -1) {
        return -1;
    }
    
    if (send(client_fd, text_message, strlen(text_message), MSG_NOSIGNAL) == -1) {
        return -1;
    }

    return 0;
}

client_control_t handle_server_message(int sockfd) {
    char buf[MAX_BUFF_LEN];
    int bytes_received = recv(sockfd, buf, MAX_BUFF_LEN - 1, 0);

    if (bytes_received == -1) {
        perror("Error: recv()");
        return SERVER_STOP;
    } else if (bytes_received == 0) {
        printf("\nClient: Server closed connection without sending data.\n");
        return SERVER_STOP;
    }

    buf[bytes_received] = '\0';
    buf[strcspn(buf, "\r\n")] = '\0'; 
    printf("Server (%d bytes)> \"%s\"\n", bytes_received, buf);

    return SERVER_CONTINUE;
}

client_control_t handle_stdin_input(int client_fd) {
    char send_buf[MAX_BUFF_LEN];
    if (fgets(send_buf, sizeof send_buf, stdin) == NULL) {
        printf("\nClient: closing connection...\n");    // Either Ctrl + D (EOF) was pressed or some write error happened
        clearerr(stdin);                                // Clear the flag that signalizes EOF
        return SERVER_STOP;
    }

    if (strcmp(send_buf, "exit\n") == 0) {
        printf("Client: Disconnecting by operator command.\n");
        return SERVER_STOP;
    }

    if (send_text_message(client_fd, send_buf) == -1) {
        perror("Error: send_text_message()");
    }

    return SERVER_CONTINUE;
}

client_control_t process_connections(struct pollfd fds[], int sockfd) {
    if (fds[1].revents & (POLLIN | POLLHUP)) {
        if (handle_server_message(sockfd) == SERVER_STOP) {
            return SERVER_STOP;
        }
    }

    if (fds[0].revents & (POLLIN | POLLHUP)) {
        if (handle_stdin_input(sockfd) == SERVER_STOP) {
            return SERVER_STOP;
        }
    }

    return SERVER_CONTINUE;
}

int main(int argc, char *argv[]) {
    int sockfd;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP/hostname> <nickname>\n", argv[0]);
        return SERVER_STOP;
    }
    char *server_ip = argv[1];
    char *nickname = argv[2];

    sockfd = get_client_socket(server_ip);
    if (sockfd == -1) {
        fprintf(stderr, "Failed to initialize client socket.\n");
        return SERVER_STOP;
    }

    printf("Authenticating as '%s'...\n", nickname);
    if (send_auth_message(sockfd, nickname) == -1) {
        perror("Failed to send auth message.");
        close(sockfd);
        return SERVER_STOP;
    }

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = sockfd;
    fds[1].events = POLLIN;

    int running = 1;
    while(running) {
        printf("> ");
        fflush(stdout);
        
        int poll_count = poll(fds, 2, -1);
        if (poll_count == -1) {
            perror("Error: poll()");
            break;
        }

        client_control_t status = process_connections(fds, sockfd);
        if (status == SERVER_STOP) {
            running = 0;
        }
    }

    close(sockfd);
    printf("Client: disconnecting and exiting.\n");

    return 0;
}
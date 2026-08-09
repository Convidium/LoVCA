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

#define PORT "8841"
#define MAX_BUFF_LEN 256

typedef enum {
    SERVER_CONTINUE = 0,
    SERVER_STOP = 1
} server_control_t;

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int get_client_socket(char address[]) {
    int sockfd;
    char server_ip[INET_ADDRSTRLEN];

    int addr_status;
    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addr_status = getaddrinfo(address, PORT, &hints, &servinfo);

    if (addr_status != 0) {
        fprintf(stderr, "Error: getaddrinfo: %s\n", gai_strerror(addr_status));
        return -1;
    }

    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sockfd == -1) {
        perror("Error: socket()");
        freeaddrinfo(servinfo);
        return -1;
    }

    inet_ntop(servinfo->ai_family, get_in_addr((struct sockaddr *)servinfo->ai_addr), server_ip, sizeof server_ip);
    printf("Client: attempting connection to %s\n", server_ip);

    if (connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        perror("Client: connect()");
        close(sockfd);
        freeaddrinfo(servinfo);
        return -1;
    }

    inet_ntop(servinfo->ai_family, get_in_addr((struct sockaddr *)servinfo->ai_addr), server_ip, sizeof server_ip);
    printf("Client: connected to %s\n", server_ip);
    printf("Type 'exit' or press Ctrl+D to disconnect this server.\n\n");

    freeaddrinfo(servinfo);

    return sockfd;
}

server_control_t handle_server_message(int sockfd) {
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
    printf("Server message (%d bytes):\n\"%s\"\n", bytes_received, buf);

    return SERVER_CONTINUE;
}

server_control_t handle_stdin_input() {
    char send_buf[MAX_BUFF_LEN];
    if (fgets(send_buf, sizeof send_buf, stdin) == NULL) {
        // Either Ctrl + D (EOF) was pressed or some write error happened
        printf("\nClient: closing connection...\n");
        // Clear the flag that signalizes EOF
        clearerr(stdin); 
        return SERVER_STOP;
    }

    if (strcmp(send_buf, "exit\n") == 0) {
        printf("Client: Disconnecting by operator command.\n");
        return SERVER_STOP;
    }

    return SERVER_CONTINUE;
}

server_control_t process_connections(struct pollfd fds[], int sockfd) {
    if (fds[1].revents & (POLLIN | POLLHUP)) {
        if (handle_server_message(sockfd) == SERVER_STOP) {
            return SERVER_STOP;
        }
    }

    if (fds[0].revents & (POLLIN | POLLHUP)) {
        if (handle_stdin_input() == SERVER_STOP) {
            return SERVER_STOP;
        }
    }

    return SERVER_CONTINUE;
}

int main(int argc, char *argv[]) {
    int sockfd;

    if (argc != 2) {
        fprintf(stderr, "Error: client hostname required\n");
        return SERVER_STOP;
    }

    sockfd = get_client_socket(argv[1]);
    if (sockfd == -1) {
        fprintf(stderr, "Failed to initialize client socket.\n");
        return SERVER_STOP;
    }

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = sockfd;
    fds[1].events = POLLIN;

    int running = 1;
    while(running) {
        int poll_count = poll(fds, 2, -1);
        if (poll_count == -1) {
            perror("Error: poll()");
            break;
        }

        server_control_t status = process_connections(fds, sockfd);
        if (status == SERVER_STOP) {
            running = 0;
        }
    }

    close(sockfd);
    printf("Client: disconnecting and exiting.\n");

    return 0;
}
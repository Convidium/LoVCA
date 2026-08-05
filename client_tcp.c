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

#include <arpa/inet.h>

#define PORT "8841"

#define MAX_BUFF_LEN 100

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd;
    char buf[MAX_BUFF_LEN];
    char server_ip[INET_ADDRSTRLEN];

    int addr_status, connect_status;
    struct addrinfo hints, *servinfo;

    if (argc != 2) {
        fprintf(stderr, "Error: client hostname required\n");
        return 1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addr_status = getaddrinfo(argv[1], PORT, &hints, &servinfo);

    if (addr_status != 0) {
        fprintf(stderr, "Error: getaddrinfo: %s\n", gai_strerror(addr_status));
        freeaddrinfo(servinfo);
        return 2;
    }

    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sockfd == -1) {
        perror("Error: socket()");
        freeaddrinfo(servinfo);
        return 3;
    }

    inet_ntop(servinfo->ai_family, get_in_addr((struct sockaddr *)servinfo->ai_addr), server_ip, sizeof server_ip);
    printf("Client: attempting connection to %s\n", server_ip);

    connect_status = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (connect_status == -1) {
        perror("Client: connect()");
        close(sockfd);
        freeaddrinfo(servinfo);
        return 4;
    }

    inet_ntop(servinfo->ai_family, get_in_addr((struct sockaddr *)servinfo->ai_addr), server_ip, sizeof server_ip);
    printf("client: connected to %s\n", server_ip);

    freeaddrinfo(servinfo);

    while(1) {
        int bytes_received = recv(sockfd, buf, MAX_BUFF_LEN - 1, 0);
        if (bytes_received == -1) {
            perror("Error: recv()");
            break;
        } else if (bytes_received == 0) {
            printf("\nClient: Server closed connection without sending data.\n");
            break;
        }

        buf[bytes_received] = '\0';
        printf("Server message (%d bytes):\n\"%s\"\n", bytes_received, buf);
    }

    close(sockfd);
    printf("Client: disconnecting and exiting.\n");

    return 0;
}
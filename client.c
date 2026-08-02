#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define SERVER_PORT "8080"
#define MESSAGE "Hello, Server!"

int main(int argc, char *argv[]) {
    int sockfd;
    struct addrinfo hints, *servinfo;
    int rv;
    ssize_t numbytes;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <hostname/IP>\n", argv[0]);
        return 1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1], SERVER_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
        perror("talker: socket");
    }

    if (servinfo == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        freeaddrinfo(servinfo);
        return 2;
    }

    if ((numbytes = sendto(sockfd, MESSAGE, strlen(MESSAGE), 0,
                           servinfo->ai_addr, servinfo->ai_addrlen)) == -1) {
        perror("talker: sendto");
        close(sockfd);
        freeaddrinfo(servinfo);
        return 1;
    }

    freeaddrinfo(servinfo);
    printf("talker: sent %ld bytes to %s\n", numbytes, argv[1]);
    close(sockfd);
    return 0;
}
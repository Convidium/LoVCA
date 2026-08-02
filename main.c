#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define SERVER_PORT "8080"
#define MAXBUFLEN 1024

int main(void) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char buf[MAXBUFLEN];
    struct sockaddr_storage requesting_addr;
    socklen_t addr_len;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, SERVER_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("listener: waiting to recvfrom...\n");

    addr_len = sizeof requesting_addr;
    ssize_t numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                                (struct sockaddr *)&requesting_addr, &addr_len);

    if (numbytes == -1) {
        perror("recvfrom");
        return 1;
    }

    buf[numbytes] = '\0';
    printf("listener: packet is %ld bytes long\n", numbytes);
    printf("listener: packet contains \"%s\"\n", buf);

    close(sockfd);

    return 0;
}
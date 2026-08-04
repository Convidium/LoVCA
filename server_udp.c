#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>

#define SERVER_PORT "8841"
#define MAX_BUFF_LEN 100

int main(void) {
    int socketfd;

    int status;
    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_DGRAM;

    getaddrinfo(NULL, SERVER_PORT, &hints, &servinfo);

    socketfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    if (socketfd == -1) {
        perror("Error: something with socket()");
        return 1;
    }

    status = bind(socketfd, servinfo->ai_addr, servinfo->ai_addrlen);

    if (status == -1) {
        close(socketfd);
        perror("Error: something with bind()");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("Server: waiting to recvfrom...\n");

    
        
    return 0;
}
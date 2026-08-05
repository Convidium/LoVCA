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

#define SERVER_PORT "8841"
#define BACKLOG 10   // how many pending connections queue will hold
#define MAX_BUFF_LEN 100

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void) {
    int sockfd, new_fd;

    int addr_status, bind_status, listen_status;
    struct addrinfo hints, *servinfo;

    struct sockaddr_storage client_addr; // connector's address info
    socklen_t their_addr_size;
    char client_ip[INET_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;

    addr_status = getaddrinfo(NULL, SERVER_PORT, &hints, &servinfo);
    if (addr_status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addr_status));
        return 1;
    }

    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sockfd == -1) {
        perror("Error: socket()");
        return 2;
    }

    bind_status = bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (bind_status == -1) {
        perror("Error: bind()");
        close(sockfd);
        return 3;
    }

    freeaddrinfo(servinfo);

    listen_status = listen(sockfd, BACKLOG);

    if (listen_status == -1) {
        perror("Error: listen()");
        return 3;
    }

    printf("Server: waiting for connections...\n");

    while (1) {
        their_addr_size = sizeof client_addr;

        new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &their_addr_size);
        if (new_fd == -1) {
            perror("Error: accept()");
            continue;
        }

        inet_ntop(client_addr.ss_family,
                  get_in_addr((struct sockaddr *)&client_addr),
                  client_ip, sizeof client_ip);
        printf("Server: got connection from %s\n", client_ip);
        printf("Type your messages below. Type 'exit' or press Ctrl+D to disconnect this client.\n\n");

        char send_buf[MAX_BUFF_LEN];

        while(1) {
            printf("Server > ");
            fflush(stdout);

            if (fgets(send_buf, sizeof send_buf, stdin) == NULL) {
                // Either Ctrl + D (EOF) was pressed or some write error happened
                printf("\nServer: End of input detected. Closing connection\n");
                // Clear the flag that signalizes EOF, so that a new socket could join.
                clearerr(stdin); 
                break;
            }

            if (strcmp(send_buf, "exit\n") == 0) {
                printf("Server: Disconnecting client by operator command.\n");
                break;
            }

            send_buf[strcspn(send_buf, "\n")] = '\0';
            int len = strlen(send_buf);
            int bytes_sent = send(new_fd, send_buf, len, 0);

            if (bytes_sent == -1) {
                perror("Error: send()");
                break;
            }
        }
        close(new_fd);
        printf("Server: Connection closed with %s.\n", client_ip);
        printf("Waiting for next client...\n");
        continue;
    }

    close(sockfd);
    return 0;
}
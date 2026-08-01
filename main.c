#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> 

#define TARGET_IP "127.0.0.1"
#define PORT 9999

int main() {int sockfd;
    struct sockaddr_in dest_addr;
    char *message = "Test UDP packet for Wireshark";

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error while creating socket");
        exit(1);
    }
    // Fill the memory block that represents our IP adress with 0's
    memset(&dest_addr, 0, sizeof(dest_addr));

    // Set the address format to our socket as AF_INET (IPv4, AF_INET6 ==> IPv6)
    dest_addr.sin_family = AF_INET;
    // Set the desired port destination to our socket
    // htons() transforms our 16-bit number into a platform-specific byte order
    dest_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, TARGET_IP, &dest_addr.sin_addr) <= 0) {
        perror("Incorrect IP address!");
        close(sockfd);
        exit(1);
    }


    printf("Countdown:\n");

    for (int i = 5; i > 0; i--) {
        printf("%d...\n", i);
        int bytes_sent = sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));

        if (bytes_sent < 0) {
            perror("Error while sending message");
        } else {
            printf("Succesfully sent %d bytes to %s:%d\n", bytes_sent, TARGET_IP, PORT);
        }
        // Pause execution for 1 second
        sleep(1);
    }

    printf("Time's up!\n");
    close(sockfd);
    return 0;
}
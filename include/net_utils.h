#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stddef.h>




// Gets IPv4 or IPv6 address as a text string.
// Returns a pointer to a buffer if succeed, or NULL if fails
const char *sockaddr_get_ip(const struct sockaddr *sa, char *buf, size_t size);

// Gets port number as a number in Host Byte Order.
// Returns value from 0 to 65535, or -1 if fails to get port number.
int sockaddr_get_port(const struct sockaddr *sa);




/* For sendall/recvall I implemented according to Beej's guide.
Their in/out interface fully replicates the in/out interface of send/recv 
for simplicity reasons*/

// Sends exactly *len bytes from buf to socket s.
// Returns 0 on success, or -1 on error. *len contains the number of bytes actually sent.
int sendall(int s, char *buf, int *len);

// Receives exactly *len bytes from socket s into buf.
// Returns 0 on success, -1 on error/closure. *len contains the number of bytes actually received.
int recvall(int s, char *buf, int *len);

#endif
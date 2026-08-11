#ifndef NET_UTILS_H
#define NET_UTILS_H

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

#endif
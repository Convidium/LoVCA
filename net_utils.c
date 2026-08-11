#include "net_utils.h"

const char *sockaddr_get_ip(const struct sockaddr *sa, char *buf, size_t size) {
    if (sa == NULL || buf == NULL) {
        return NULL;
    }

    if (sa->sa_family == AF_INET) {
        const struct sockaddr_in *sin = (const struct sockaddr_in *)sa;
        return inet_ntop(AF_INET, &(sin->sin_addr), buf, size);
    } else if (sa->sa_family == AF_INET6) {
        const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)sa;
        return inet_ntop(AF_INET6, &(sin6->sin6_addr), buf, size);
    }

    return NULL;
}

int sockaddr_get_port(const struct sockaddr *sa) {
    if (sa == NULL) {
        return -1;
    }

    if (sa->sa_family == AF_INET) {
        const struct sockaddr_in *sin = (const struct sockaddr_in *)sa;
        return ntohs(sin->sin_port);
    } else if (sa->sa_family == AF_INET6) {
        const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)sa;
        return ntohs(sin6->sin6_port);
    }

    return -1;
}
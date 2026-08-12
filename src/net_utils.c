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

int sendall(int s, const void *buf, int *len) {
    int bytes_sent = 0;
    int bytes_left = *len;
    int send_status;

    while (bytes_sent < *len) {
        send_status = send(s, buf + bytes_sent, bytes_left, MSG_NOSIGNAL);
        if (send_status == -1) { break; }
        bytes_sent += send_status;
        bytes_left -= send_status;
    }

    // Setting this to bytes_sent so that we'd 
    // know how many bytes have been succesfully sent
    *len = bytes_sent;

    return send_status == -1 ? -1:0;
}

int recvall(int s, void *buf, int *len) {
    int bytes_received = 0;
    int bytes_left = *len;
    int recv_status;

    while (bytes_received < *len) {
        recv_status = recv(s, buf+bytes_received, bytes_left, 0);
        if (recv_status <= 0) { break; }
        bytes_received += recv_status;
        bytes_left -= recv_status;
    }

    *len = bytes_received;

    if (recv_status == -1 || (recv_status == 0 && bytes_received < *len)) {
        return -1;
    }

    return 0;
}
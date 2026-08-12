#define _POSIX_C_SOURCE 200112L

#include "packet.h"

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int packet_send_auth(int fd, const char *nick) {
    msg_header_t header;
    header.type = MSG_AUTH;
    header.length = htons(strlen(nick));

    if (send(fd, &header, sizeof(header), MSG_NOSIGNAL) == -1) {
        return -1;
    }

    if (send(fd, nick, strlen(nick), MSG_NOSIGNAL) == -1) {
        return -1;
    }

    return 0;
}

int packet_send_text(int fd, const char *text) {
    if (text == NULL) {
        return -1;
    }
    
    msg_header_t header;
    header.type = MSG_TEXT;
    header.length = htons(strlen(text));

    int header_len = sizeof(header);
    if (sendall(fd, &header, &header_len) == -1) {
        return -1;
    }
    
    int text_len = (int)strlen(text);
    if (sendall(fd, text, text_len) == -1) {
        return -1;
    }

    return 0;
}

int packet_read_header(int fd, msg_header_t *out_header) {
    if (out_header == NULL) {
        return -1;
    }

    int len = sizeof(msg_header_t);
    return recvall(fd, out_header, &len);
}

int packet_read_payload(int fd, void *out_buf, size_t len) {
    if (out_buf == NULL || len == 0) {
        return -1;
    }

    int read_len = (int)len;
    return recvall(fd, out_buf, &read_len);
}
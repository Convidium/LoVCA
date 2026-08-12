#define _POSIX_C_SOURCE 200112L
#include "net_socket.h"

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

int create_listener_socket(const char *port, int backlog) {
    int listener_fd;
    struct addrinfo hints, *servinfo;
    int yes = 1;

    int addr_status;
    int bind_status;
    int listen_status;

    memset(&hints, 0, sizeof hints);
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;

    addr_status = getaddrinfo(NULL, port, &hints, &servinfo);
    if (addr_status != 0) {
        return -1;
    }

    listener_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (listener_fd == -1) {
        goto cleanup;
    }

    if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        close(listener_fd);
        listener_fd = -1;
        goto cleanup;
    }

    bind_status = bind(listener_fd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (bind_status != 0) {
        listener_fd = -1;
        goto cleanup;
    }

    freeaddrinfo(servinfo);

    listen_status = listen(listener_fd, backlog);
    if (listen_status != 0) {
        listener_fd = -1;
        goto cleanup;
    }

cleanup:
    if (servinfo != NULL) {
        freeaddrinfo(servinfo);
    }
    return listener_fd;
}

int create_client_socket(const char *host, const char *port) {
    int client_fd;
    struct addrinfo hints, *servinfo;

    int addr_status;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addr_status = getaddrinfo(host, port, &hints, &servinfo);
    if (addr_status != 0) {
        return -1;
    }

    client_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (client_fd == -1) {
        goto cleanup;
    }

    if (connect(client_fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        close(client_fd);
        client_fd = -1;
        goto cleanup;
    }
    // printf("Type 'exit' or press Ctrl+D to disconnect this server.\n\n");

cleanup:
    if (servinfo != NULL) {
        freeaddrinfo(servinfo);
    }
    return client_fd;
}
#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include <sys/socket.h>

/* Create local listener socket fd on port PORT, with a backlog of BACKLOG.
* Returns file descriptor (>= 3) on success, or -1 on error. */
int create_listener_socket(const char *port, int backlog);


/* Create local client socket fd on port PORT, that connects to server on HOST.
* Returns connected file descriptor (>= 3) on success, or -1 on error. */
int create_client_socket(const char *host, const char *port);
#endif
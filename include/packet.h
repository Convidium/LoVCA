#ifndef PACKET_H
#define PACKET_H

#include "protocol.h"
#include "net_utils.h"
#include <stddef.h>

/* Sends an authentication packet with nickname.
 * Returns 0 on success, -1 on network error. */
int packet_send_auth(int client_fd, const char *nickname);

/* Sends a text message packet.
 * Returns 0 on success, -1 on network error. */
int packet_send_text(int client_fd, const char *text_message);


/* Receives a header packet and reads it.
Writes result to *out_header*/
int packet_read_header(int fd, msg_header_t *out_header);

/* Receives a payload packet and reads it.
Writes result to *out_buf*/
int packet_read_payload(int fd, void *out_buf, size_t len);

#endif // PACKET_H
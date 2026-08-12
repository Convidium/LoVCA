#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <poll.h>

#include "protocol.h"
#include "net_socket.h"
#include "net_utils.h"
#include "packet.h"

typedef struct {
    int fd;
    char nickname[MAX_NICKNAME_LEN + 1];
} client_slot_t;

static client_slot_t client_fds[MAX_CLIENTS];
static struct pollfd pfds[MAX_CLIENTS+2];               // 0: stdin, 1: listener_fd, 2..N: client_fd
static int poll_size = 2;

static void init_server(void) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i].fd = -1;
        client_fds[i].nickname[0] = '\0';
    }
}

static void broadcast_message(const char *msg, int sender_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i].fd != -1 && client_fds[i].fd != sender_fd) {
            if (packet_send_text(client_fds[i].fd, msg) == -1) {
                fprintf(stderr, "{-} Failed to send to '%s', closing.\n", client_fds[i].nickname);
                close(client_fds[i].fd);
                client_fds[i].fd = -1;
            }
        }
    }
}

static void handle_new_connection(int listener_fd) {
    struct sockaddr_storage remote_addr;
    socklen_t addr_len = sizeof(remote_addr);
    int new_fd = accept(listener_fd, (struct sockaddr *)&remote_addr, &addr_len);
    if (new_fd == -1) return;

    msg_header_t hdr;
    if (packet_read_header(new_fd, &hdr) == -1 || hdr.type != MSG_AUTH) {
        close(new_fd);
        return;
    }

    uint16_t name_len = ntohs(hdr.length);
    if (name_len == 0 || name_len > MAX_NICKNAME_LEN) {
        close(new_fd);
        return;
    }

    char nick[MAX_NICKNAME_LEN + 1];
    if (packet_read_payload(new_fd, nick, name_len) == -1) {
        close(new_fd);
        return;
    }
    nick[name_len] = '\0';

    int slot = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i].fd == -1) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        printf("{-} Server full, rejected '%s'\n", nick);
        close(new_fd);
        return;
    }

    client_fds[slot].fd = new_fd;
    strncpy(client_fds[slot].nickname, nick, MAX_NICKNAME_LEN);

    pfds[poll_size].fd = new_fd;
    pfds[poll_size].events = POLLIN;
    poll_size++;

    char ip[INET6_ADDRSTRLEN];
    sockaddr_get_ip((struct sockaddr *)&remote_addr, ip, sizeof(ip));
    int port = sockaddr_get_port((struct sockaddr *)&remote_addr);

    printf("{+} Client '%s' connected from %s:%d (Slot %d)\n", nick, ip, port, slot);
}

static void handle_client_data(int pfd_index) {
    int fd = pfds[pfd_index].fd;
    msg_header_t hdr;

    if (packet_read_header(fd, &hdr) == -1) {
        close(fd);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i].fd == fd) {
                printf("{-} Client '%s' disconnected.\n", client_fds[i].nickname);
                client_fds[i].fd = -1;
                break;
            }
        }
        pfds[pfd_index] = pfds[poll_size - 1];
        poll_size--;
        return;
    }

    if (hdr.type == MSG_TEXT) {
        uint16_t len = ntohs(hdr.length);
        char buf[MAX_BUFF_LEN + 1];
        if (packet_read_payload(fd, buf, len) != -1) {
            buf[len] = '\0';
            printf("[Msg] %s\n", buf);
            broadcast_message(buf, fd);
        }
    }
}

int main(void) {
    signal(SIGPIPE, SIG_IGN);
    init_server();

    int listener_fd = create_listener_socket(PORT, BACKLOG);
    if (listener_fd == -1) {
        perror("Failed to start listener");
        return 1;
    }

    printf("=== Star Server started on port %s ===\n", PORT);

    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;

    pfds[1].fd = listener_fd;
    pfds[1].events = POLLIN;

    int running = 1;
    while (running) {
        if (poll(pfds, poll_size, -1) == -1) {
            perror("poll error");
            break;
        }

        if (pfds[1].revents & POLLIN) {
            handle_new_connection(listener_fd);
        }

        for (int i = 2; i < poll_size; i++) {
            if (pfds[i].revents & (POLLIN | POLLHUP | POLLERR)) {
                handle_client_data(i);
            }
        }

        if (pfds[0].revents & POLLIN) {
            char line[256];
            if (fgets(line, sizeof(line), stdin) == NULL || strcmp(line, "exit\n") == 0) {
                running = 0;
            } else {
                line[strcspn(line, "\r\n")] = '\0';
                broadcast_message(line, -1);
            }
        }
    }

    close(listener_fd);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i].fd != -1) close(client_fds[i].fd);
    }
    return 0;
}
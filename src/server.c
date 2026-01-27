#define _POSIX_C_SOURCE	200809L
#define _GNU_SOURCE
#include "server.h"
#include "common.h"
#include <time.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>

typedef struct {
	// one for the server socket itself
	struct pollfd peers[SERVER_CONNECTION_MAX + 1];
	nfds_t npeers;
} network_t;

server_t server_internal_main;
network_t network;

static void error(const char *err) {
	server_internal_main.info.type = SERVER_INFO_ERR;

	if (errno) {
		snprintf(server_internal_main.info.text, SERVER_INFO_MAX, "%s: %s", err, errno_string);
		errno = 0;
	} else {
		strncpy(server_internal_main.info.text, err, SERVER_INFO_MAX);
	}
}

static void notification(const char *notif) {
	server_internal_main.info.type = SERVER_INFO_NOTIF;
	strncpy(server_internal_main.info.text, notif, SERVER_INFO_MAX);
}

void server_init(void) {
	server_internal_main.info.type = SERVER_INFO_QUIET;

	server_internal_main.sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server_internal_main.sock == -1) {
		error("unable to create socket");
		return;
	}

	const int yes = 1;
	if (setsockopt(server_internal_main.sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) {
		error("unable to set socket option");
		return;
	}

	server_internal_main.addr.sin_family = AF_INET;
	server_internal_main.addr.sin_addr.s_addr = INADDR_ANY;
	server_internal_main.addr.sin_port = htons(SERVER_DEFAULT_PORT);

	server_internal_main.start_time = time(NULL);
	server_internal_main.status = SERVER_IDLE;

	return;
}

// should be called deinit but terminate sounds cooler
void server_terminate(void) {
	close(server_internal_main.sock);
	server_internal_main.status = SERVER_DEAD;
}


bool server_error(void) {
	return server_internal_main.info.type == SERVER_INFO_ERR;
}

server_info_t server_read_info(void) {
	server_info_t info = server_internal_main.info;

	server_internal_main.info.type = SERVER_INFO_QUIET;

	return info;
}


static void network_init(void) {
	network.peers[network.npeers++] = (struct pollfd) {
		.fd = server_internal_main.sock,
		.events = (POLLIN | POLLOUT),
	};
}

void server_host(void) {
	if (server_internal_main.status == SERVER_HOST) {
		notification("already hosting");
		return;
	}

	notification("hosting...");

	if (fcntl(server_internal_main.sock, F_SETFL, O_NONBLOCK)) {
		error("unable to set socket as nonblocking");
		return;
	}

	if (bind(server_internal_main.sock, (struct sockaddr *)&server_internal_main.addr, sizeof(server_internal_main.addr))) {
		error("unable to bind to socket");
		return;
	}

	if (listen(server_internal_main.sock, SERVER_CONNECTION_MAX)) {
		error("unable to setup socket for listening");
		return;
	}

	network_init();

	server_internal_main.status = SERVER_HOST;
}

static void network_del(const nfds_t peer) {
	shutdown(network.peers[peer].fd, SHUT_RDWR);
	close(network.peers[peer].fd);
	network.peers[peer].fd = -1;
}

static void network_update(void) {
	ptrdiff_t pos = -1;

	for (nfds_t i = 0; i < network.npeers; i++) {
		if (network.peers[i].fd == -1) {
			pos = i;
			break;
		}
	}

	if (pos == -1)
		return;

	ptrdiff_t free = pos;
	ptrdiff_t dels = 0;
	for (nfds_t i = pos; i < network.npeers; i++) {
		if (network.peers[i].fd == -1) {
			dels++;
		} else {
			network.peers[free] = network.peers[i];
			network.peers[i].fd = -1;
		}
	}

	network.npeers -= dels;
}


static void message(const char *msg, const size_t len) {
	if (!poll(network.peers, network.npeers, 0)) {
		error("unable to send message");
		return;
	}

	for (nfds_t peer = 0; peer < network.npeers; peer++) {
		const struct pollfd *pfd = network.peers + peer;

		if (pfd->fd == server_internal_main.sock) {
			notification(msg);
			continue;
		}

		if (pfd->revents & (POLLERR | POLLHUP | POLLNVAL)) {
			error("unable to send message to peer");
			network_del(peer);
			continue;
		}

		if (!(pfd->revents & POLLOUT)) {
			error("unable to send message to peer");
			network_del(peer);
		} else {
			write(pfd->fd, msg, len);
		}
	}
}

static inline bool nonblocking_errno(void) {
	return errno == EAGAIN || errno == EWOULDBLOCK;
}

#define ADDR_MAX		64
#define NOTIFICATION_MAX	256
void server_probe() {
	char addr_str[ADDR_MAX];

	socklen_t peer_addrlen;
	struct sockaddr_in peer_addr;

	if (!poll(network.peers, network.npeers, 0))
		return;

	char notif[NOTIFICATION_MAX];

	for (nfds_t peer = 0; peer < network.npeers; peer++) {
		if (network.peers[peer].fd == server_internal_main.sock) {
			if (network.peers[peer].revents & (POLLERR | POLLHUP | POLLNVAL)) {
				error("unable to accept connection");
				continue;
			}

			if (!(network.peers[peer].revents & (POLLIN)))
				continue;

			const int peer_sock = accept(server_internal_main.sock, (struct sockaddr *)&peer_addr, &peer_addrlen);

			inet_ntop(AF_INET, &peer_addr, addr_str, sizeof(addr_str));
			snprintf(notif, sizeof(notif), "peer connected (%s:%hu)", addr_str, ntohs(peer_addr.sin_port));

			notification(notif);

			network.peers[network.npeers++] = (struct pollfd) {
				.fd = peer_sock,
				.events = (POLLOUT | POLLIN),
			};
		} else if (network.peers[peer].revents & (POLLERR | POLLHUP | POLLNVAL)) {
			notification("peer connection error");
			network_del(peer);
		} else if (network.peers[peer].revents & POLLIN) {
			const ssize_t len = read(network.peers[peer].fd, notif, NOTIFICATION_MAX - 1);

			if (len <= 0) {
				notification("peer eof");
				network_del(peer);
				continue;
			}

			notif[len - 1] = '\0';
			message(notif, len - 1);
		}
	}

	network_update();
}

void server_unhost(void) {
	notification("unhosting...");

	todo("server_cleanup_host");
}


void server_connect(void) {
	notification("connecting...");

	todo("server_connect");
}

void server_disconnect(void) {
	notification("disconnecting...");

	todo("server_disconnect");
}

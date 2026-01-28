#define _POSIX_C_SOURCE	200809L
#define _GNU_SOURCE
#include "server.h"
#include "common.h"
#include <time.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>

typedef struct {
	// one for the server socket itself
	struct pollfd peers[SERVER_CONNECTION_MAX + 1];
	nfds_t npeers;
} network_t;

server_t server_internal_main;
network_t network;


static void error(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	server_internal_main.info.type = SERVER_INFO_ERR;

	vsnprintf(server_internal_main.info.text, SERVER_INFO_MAX, fmt, args);

	va_end(args);
}

static void notification(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	server_internal_main.info.type = SERVER_INFO_NOTIF;
	vsnprintf(server_internal_main.info.text, SERVER_INFO_MAX, fmt, args);

	va_end(args);
}

void message(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	server_internal_main.info.type = SERVER_INFO_MSG;
	vsnprintf(server_internal_main.info.text, SERVER_INFO_MAX, fmt, args);

	va_end(args);
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


bool server_in_error(void) {
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

static void network_add(const int peer) {
	network.peers[network.npeers++] = (struct pollfd) {
		.fd = peer,
		.events = (POLLOUT | POLLIN),
	};
}

static void network_del(const nfds_t pos) {
	shutdown(network.peers[pos].fd, SHUT_RDWR);
	close(network.peers[pos].fd);
	network.peers[pos].fd = -1;
}

static void network_update(void) {
	ptrdiff_t free = -1;

	for (nfds_t i = 0; i < network.npeers; i++) {
		if (network.peers[i].fd == -1) {
			free = i;
			break;
		}
	}

	if (free == -1)
		return;

	ptrdiff_t dels = 0;
	for (nfds_t i = free; i < network.npeers; i++) {
		if (network.peers[i].fd == -1) {
			dels++;
		} else {
			network.peers[free++] = network.peers[i];
			network.peers[i].fd = -1;
		}
	}

	network.npeers -= dels;
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

#define SERVER_MSG_FMT(uid, msg) \
	"(user%d): %.*s", (uid), SERVER_MSG_MAX, (msg)

void server_distribute_message(const int user_id, const char *msg) {
	/*
		network either unitialized (npeers == 0) or has no peers (npeers == 1)
		just echo the message back to the server
	*/
	if (network.npeers <= 1) {
		message(SERVER_MSG_FMT(user_id, msg));
		return;
	}

	if (!poll(network.peers, network.npeers, 0)) {
		error("unable to send message");
		return;
	}

	for (nfds_t i = 0; i < network.npeers; i++) {
		const int peer = network.peers[i].fd;
		const short events = network.peers[i].revents;

		if (peer == server_internal_main.sock) {
			message(SERVER_MSG_FMT(user_id, msg));
		} else if (events & POLLOUT) {
			dprintf(peer, SERVER_MSG_FMT(user_id, msg));
			dprintf(peer, "\n");
		} else {
			error("unable to send message to peer");
			network_del(peer);
		}
	}
}

static void message_peers(const int peer_from) {
	char msg[SERVER_MSG_MAX + 1];
	ssize_t len = read(peer_from, msg, SERVER_MSG_MAX);

	if (len <= 0)
		return;

	// overwrite newline for now, this is just for testing
	if (msg[len - 1] == '\n')
		len--;

	if (len <= 0)
		return;

	msg[len] = '\0';

	server_distribute_message(peer_from, msg);
}

#define IP_ADDR_MAX	64
static void connect_peer(void) {
	char addr_str[IP_ADDR_MAX];

	socklen_t peer_addrlen;
	struct sockaddr_in peer_addr;
	const int peer = accept(server_internal_main.sock, (struct sockaddr *)&peer_addr, &peer_addrlen);

	inet_ntop(AF_INET, &peer_addr, addr_str, sizeof(addr_str));
	notification("'user%d' connected (%s:%hu)", peer, addr_str, ntohs(peer_addr.sin_port));

	network_add(peer);
}

void server_probe() {
	if (!poll(network.peers, network.npeers, 0))
		return;

	for (nfds_t i = 0; i < network.npeers; i++) {
		const int peer = network.peers[i].fd;
		const short events = network.peers[i].revents;

		if (peer == server_internal_main.sock) {
			if (events & (POLLERR | POLLHUP | POLLNVAL)) {
				error("unable to accept connection");
			} else if (events & (POLLIN)) {
				connect_peer();
			}

		} else if (events & (POLLHUP)) {
			notification("'user%d' disconnected", peer);
			network_del(i);

		} else if (events & (POLLERR | POLLNVAL)) {
			error("peer connection error");
			network_del(i);

		} else if (events & POLLIN) {
			message_peers(peer);
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

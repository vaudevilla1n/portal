#define _POSIX_C_SOURCE	200809L
#define _GNU_SOURCE

#include "server.h"

#include "tui.h"
#include "common.h"

#include <time.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>

#define error			tui_warn
#define notification		tui_info
#define message(msg)		tui_puts("%s", (msg))

typedef struct {
	uint32_t user_id;
	uint32_t len;
	const char text[SERVER_MSG_MAX + 1]; 
} server_msg_t;

typedef struct {
	// one for the server socket itself
	struct pollfd peers[SERVER_CONNECTION_MAX + 1];
	nfds_t npeers;
} network_t;

typedef struct {
	struct sockaddr_in addr;
	int sock;

	time_t start_time;

	server_status_t status;

	// server_info_t info;
} server_t;

server_t internal_server;
network_t server_network;


/* str_addr and str_port are ugly names */
int parse_addr(const char *str_addr, const char *str_port, struct sockaddr_in *addr) {
	if (!str_addr) {
		addr->sin_addr.s_addr = SERVER_DEFAULT_ADDR;
	} else {
		if (!inet_aton(str_addr, &addr->sin_addr)) {
			error("invalid address: '%s'", str_addr);
			return 1;
		}
	}

	if (!str_port) {
		addr->sin_port = htons(SERVER_DEFAULT_PORT);
	} else {
		errno = 0;
		char *end;
		uint32_t _port = strtoul(str_port, &end, 10);

		if (errno || *end || _port > USHRT_MAX) {
			error("invalid port: '%s'", str_port);
			return 1;
		}

		addr->sin_port = htons((uint16_t)_port);
	}

	return 0;
}

static void shutdown_sequence(const int sock) {
	const char eof = SERVER_EOF;
	write(sock, &eof, 1);
	shutdown(sock, SHUT_RDWR);
	close(sock);
}


static void network_init(void) {
	server_network.peers[server_network.npeers++] = (struct pollfd) {
		.fd = internal_server.sock,
		.events = (POLLIN | POLLOUT),
	};
}

static int network_add(const int peer) {
	if (server_network.npeers == SERVER_CONNECTION_MAX)
		return 1;

	server_network.peers[server_network.npeers++] = (struct pollfd) {
		.fd = peer,
		.events = (POLLIN),
	};
	return 0;
}

static void network_del(const nfds_t pos) {
	shutdown_sequence(server_network.peers[pos].fd);
	server_network.peers[pos].fd = -1;
}

static void network_deinit(void) {
	/*
		forgot that the first peer is the server itself
		if i shutdown that first i can't send anymore transmissions
	*/
	for (nfds_t i = 1; i < server_network.npeers; i++)
		if (server_network.peers[i].fd > 0)
			network_del(i);
	
	shutdown(server_network.peers[0].fd, SHUT_WR);
	close(server_network.peers[0].fd);

	server_network.peers[0] = (struct pollfd){ 0 };
	server_network.npeers = 0;
}

static bool network_poll_event(void) {
	return poll(server_network.peers, server_network.npeers, 0) > 0;
}

static void network_update(void) {
	ptrdiff_t free = -1;

	for (nfds_t i = 0; i < server_network.npeers; i++) {
		if (server_network.peers[i].fd == -1) {
			free = i;
			break;
		}
	}

	if (free == -1)
		return;

	ptrdiff_t dels = 0;
	for (nfds_t i = free; i < server_network.npeers; i++) {
		if (server_network.peers[i].fd == -1) {
			dels++;
		} else {
			server_network.peers[free++] = server_network.peers[i];
			server_network.peers[i].fd = -1;
		}
	}

	server_network.npeers -= dels;
}


void server_init(void) {
	internal_server.start_time = time(NULL);
	internal_server.status = SERVER_IDLE;
}


static void host_distribute_message(const char *msg) {
	/*
		network either unitialized (npeers == 0) or has no peers (npeers == 1)
		just echo the message back to the server
	*/
	if (server_network.npeers <= 1) {
		message(msg);
		return;
	}

	for (nfds_t i = 0; i < server_network.npeers; i++) {
		const int peer = server_network.peers[i].fd;

		if (peer == internal_server.sock)
			message(msg);
		else
			dprintf(peer, "%s", msg);
	}
}

static bool host_receive_message(const int peer) {
	char msg[SERVER_MSG_MAX + 1];
	const ssize_t len = read(peer, msg, SERVER_MSG_MAX);

	if (len == 1 && msg[0] == SERVER_EOF)
		return false;

	if (len > 0) {
		msg[len] = '\0';
		host_distribute_message(msg);
	}

	return true;
}

#define IP_ADDR_MAX	64
static void add_connection(void) {
	char addr_str[IP_ADDR_MAX];

	socklen_t peer_addrlen;
	struct sockaddr_in peer_addr;
	const int peer = accept(internal_server.sock, (struct sockaddr *)&peer_addr, &peer_addrlen);

	inet_ntop(AF_INET, &peer_addr, addr_str, sizeof(addr_str));
	notification("user connected (%d) (%s:%hu)", peer, addr_str, ntohs(peer_addr.sin_port));

	if (network_add(peer)) {
		error("server at maximum capacity");
		shutdown(peer, SHUT_RDWR);
		close(peer);
	}
}

static void host_probe() {
	if (!network_poll_event())
		return;

	for (nfds_t i = 0; i < server_network.npeers; i++) {
		const int peer = server_network.peers[i].fd;
		const short events = server_network.peers[i].revents;

		if (peer == internal_server.sock) {
			if (events & (POLLERR | POLLHUP | POLLNVAL)) {
				error("unable to accept connection");
			} else if (events & (POLLIN)) {
				add_connection();
			}

		} else if (events & (POLLHUP | POLLERR)) {
			notification("user disconnected (%d)", peer);
			network_del(i);

		} else if (events & (POLLNVAL)) {
			error("peer connection error");
			network_del(i);

		} else if (events & POLLIN) {
			// EOF received
			if (!host_receive_message(peer)) {
				notification("user disconnected (%d)", peer);
				network_del(i);
			}
		}
	}

	network_update();
}


void server_host(const char *str_addr, const char *str_port) {
	switch (internal_server.status) {
	case SERVER_DEAD:	error("server has moved on"); return;
	case SERVER_HOST:	error("already hosting"); return;
	case SERVER_CONNECT:	error("can't host while connected to a server"); return;

	case SERVER_IDLE:	break;

	default:		__unreachable("server host");
	}

	notification("hosting...");

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;

	if (parse_addr(str_addr, str_port, &addr))
		return;

	internal_server.sock = socket(AF_INET, SOCK_STREAM, 0);
	if (internal_server.sock == -1) {
		error("unable to create socket");
		return;
	}

	const int yes = 1;
	if (setsockopt(internal_server.sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) {
		error("unable to set socket option");
		return;
	}

	if (fcntl(internal_server.sock, F_SETFL, O_NONBLOCK)) {
		error("unable to set socket as nonblocking");
		return;
	}

	if (bind(internal_server.sock, (struct sockaddr *)&addr, sizeof(addr))) {
		error("unable to bind to socket");
		return;
	}

	if (listen(internal_server.sock, SERVER_CONNECTION_MAX)) {
		error("unable to setup socket for listening");
		return;
	}

	network_init();

	internal_server.status = SERVER_HOST;

	notification("successfully hosted");
}

void server_unhost(void) {
	if (internal_server.status != SERVER_HOST) {
		error("no server is being hosted");
		return;
	}

	notification("unhosting...");

	network_deinit();

	internal_server.status = SERVER_IDLE;

	notification("successfully unhosted");
}


static bool peer_receive_message(void) {
	char msg[SERVER_MSG_MAX + 1];
	const ssize_t len = read(internal_server.sock, msg, SERVER_MSG_MAX);

	if (len == 1 && msg[0] == SERVER_EOF)
		return false;

	if (len > 0) {
		msg[len] = '\0';
		message(msg);
	}

	return true;
}

static void peer_probe(void) {
	struct pollfd pfd = {
		.fd = internal_server.sock,
		.events = (POLLIN),
	};

	if (!poll(&pfd, 1, 0))
		return;
	
	/* network consists only of the socket to the host */
	const int events = pfd.revents;
	if (events & (POLLERR | POLLHUP)) {
		notification("server has disconnected");
		server_disconnect();

	} else if (events & (POLLNVAL)) {
		error("host died or sm idk");
		server_disconnect();

	} else if (events & (POLLIN)) {
		// EOF received
		if (!peer_receive_message()) {
			notification("server has disconnected");
			server_disconnect();
		}
	}
}

static void peer_send_message(const char *msg) {
	dprintf(internal_server.sock, "%s", msg);
}

void server_connect(const char *str_addr, const char *str_port) {
	switch (internal_server.status) {
	case SERVER_DEAD:	error("server died man"); return;
	case SERVER_CONNECT:	error("already connected to a host"); return;
	case SERVER_HOST:	error("can't connect while hosting a server"); return;

	case SERVER_IDLE:	break;

	default:		__unreachable("server connect");
	}

	notification("connecting...");

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;

	if (parse_addr(str_addr, str_port, &addr))
		return;

	internal_server.sock = socket(AF_INET, SOCK_STREAM, 0);
	if (internal_server.sock == -1) {
		error("unable to create socket");
		return;
	}

	if (connect(internal_server.sock, (struct sockaddr *)&addr, sizeof(addr))) {
		error("unable to connect to host");
		return;
	}

	if (fcntl(internal_server.sock, F_SETFL, O_NONBLOCK)) {
		error("unable to set socket as nonblocking");
		return;
	}

	internal_server.status = SERVER_CONNECT;

	notification("successfully connected");
}

void server_disconnect(void) {
	if (internal_server.status != SERVER_CONNECT) {
		error("not connected to a server");
		return;
	}

	notification("disconnecting...");

	shutdown_sequence(internal_server.sock);
	internal_server.status = SERVER_IDLE;

	notification("successfully disconnected");
}


void server_status(void) {
	switch (internal_server.status) {
	case SERVER_DEAD:	notification("server is dead"); break;
	case SERVER_IDLE:	notification("server is idle"); break;
	case SERVER_HOST:	notification("currently hosting a server"); break;
	case SERVER_CONNECT:	notification("currently connected to a server"); break;

	default:	__unreachable("server_status");
	}
}


/*
	TODO instead of switching the status i could make function pointers
*/

void server_send_message(const int user_id, const char *text) {
	char msg[SERVER_MSG_MAX];
	snprintf(msg, SERVER_MSG_MAX, SERVER_MSG_FMT(user_id, text));

	switch (internal_server.status) {
	case SERVER_IDLE:	message(msg); break;
	case SERVER_HOST:	host_distribute_message(msg); break;
	case SERVER_CONNECT:	peer_send_message(msg); break;

	default:		break;
	}
}


void server_iteration(void) {
	switch(internal_server.status) {
	case SERVER_HOST:	host_probe(); break;
	case SERVER_CONNECT:	peer_probe(); break;

	default:		break;
	}
}


// should be called deinit but terminate sounds cooler
void server_terminate(void) {

	switch (internal_server.status) {
	case SERVER_HOST:	server_unhost(); break;
	case SERVER_CONNECT:	server_disconnect(); break;

	default:		break;
	}

	internal_server.status = SERVER_DEAD;
}


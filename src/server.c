#define _POSIX_C_SOURCE	200809L
#define _GNU_SOURCE
#include "server.h"
#include "common.h"
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>

static void server_error(server_t *server, const char *err) {
	server->info_type = SERVER_INFO_ERR;
	strncpy(server->info, err, SERVER_INFO_MAX);
}

static void server_notification(server_t *server, const char *notif) {
	server->info_type = SERVER_INFO_NOTIF;
	strncpy(server->info, notif, SERVER_INFO_MAX);
}

server_t server_init(void) {
	server_t server = { 0 };
	
	server.info_type = SERVER_INFO_QUIET;
	server.info = calloc(SERVER_INFO_MAX, sizeof(*(server.info)));

	server.sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server.sock == -1) {
		server_error(&server, "unable to create socket");
		return server;
	}

	const int yes = 1;
	if (setsockopt(server.sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) {
		server_error(&server, "unable to set socket option");
		return server;
	}

	server.addr.sin_family = AF_INET;
	server.addr.sin_addr.s_addr = INADDR_ANY;
	server.addr.sin_port = htons(SERVER_DEFAULT_PORT);

	server.start_time = time(NULL);
	server.status = SERVER_IDLE;

	return server;
}

// should be called deinit but terminate sounds cooler
void server_terminate(server_t *server) {
	shutdown(server->sock, SHUT_RDWR);
	close(server->sock);

	free(server->info);

	*server = (server_t){ 0 };
}

#define ADDR_MAX		64
#define NOTIFICATION_MAX	256
void server_handle_connection(server_t *server) {
	char addr_str[ADDR_MAX];

	socklen_t peer_addrlen;
	struct sockaddr_in peer_addr;

	const int peer = accept(server->sock, (struct sockaddr *)&peer_addr, &peer_addrlen);
	if (peer == -1) {
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			server_error(server, "unable to accept connection");

		return;
	}

	inet_ntop(AF_INET, &peer_addr, addr_str, sizeof(addr_str));

	char notif[NOTIFICATION_MAX];
	snprintf(notif, sizeof(notif), "peer connected (%s:%hu)", addr_str, ntohs(peer_addr.sin_port));

	server_notification(server, notif);

	write(peer, "hello\n", 6);

	if (shutdown(peer, SHUT_WR))
		server_error(server, "unable to shutdown peer's connection");
}

void server_host(server_t *server) {
	if (server->status == SERVER_HOST) {
		server_notification(server, "already hosting");
		return;
	}

	server_notification(server, "hosting...");

	if (bind(server->sock, (struct sockaddr *)&server->addr, sizeof(server->addr))) {
		server_error(server, "unable to bind to socket");
		return;
	}

	if (listen(server->sock, SERVER_DEFAULT_BACKLOG)) {
		server_error(server, "unable to setup socket for listening");
		return;
	}

	if (fcntl(server->sock, F_SETFL, O_NONBLOCK)) {
		server_error(server, "unable to set socket as nonblocking");
		return;
	}

	server->status = SERVER_HOST;
}

void server_unhost(server_t *server) {
	unused(server);

	server_notification(server, "unhosting...");

	todo("server_cleanup_host");
}


void server_connect(server_t *server) {
	unused(server);

	server_notification(server, "connecting...");

	todo("server_connect");
}

void server_disconnect(server_t *server) {
	unused(server);

	server_notification(server, "disconnecting...");

	todo("server_disconnect");
}

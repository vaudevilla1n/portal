#define _POSIX_C_SOURCE	200809L
#define _GNU_SOURCE
#include "server.h"
#include "common.h"
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>

server_t server_internal_main;

static void error(const char *err) {
	server_internal_main.info.type = SERVER_INFO_ERR;

	if (errno)
		snprintf(server_internal_main.info.text, SERVER_INFO_MAX, "%s: %s", err, errno_string);
	else
		strncpy(server_internal_main.info.text, err, SERVER_INFO_MAX);
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
}


bool server_error(void) {
	return server_internal_main.info.type == SERVER_INFO_ERR;
}

server_info_t server_read_info(void) {
	server_info_t info = server_internal_main.info;

	server_internal_main.info.type = SERVER_INFO_QUIET;

	return info;
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

	if (listen(server_internal_main.sock, SERVER_DEFAULT_BACKLOG)) {
		error("unable to setup socket for listening");
		return;
	}

	server_internal_main.status = SERVER_HOST;
}

#define ADDR_MAX		64
#define NOTIFICATION_MAX	256
void server_handle_connection(void) {
	char addr_str[ADDR_MAX];

	socklen_t peer_addrlen;
	struct sockaddr_in peer_addr;

	const int peer = accept(server_internal_main.sock, (struct sockaddr *)&peer_addr, &peer_addrlen);
	if (peer == -1) {
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			error("unable to accept connection");

		return;
	}

	inet_ntop(AF_INET, &peer_addr, addr_str, sizeof(addr_str));

	char notif[NOTIFICATION_MAX];
	snprintf(notif, sizeof(notif), "peer connected (%s:%hu)", addr_str, ntohs(peer_addr.sin_port));

	notification(notif);

	write(peer, "hello\n", 6);

	if (shutdown(peer, SHUT_WR))
		error("unable to shutdown peer's connection");
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

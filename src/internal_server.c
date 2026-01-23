#define _POSIX_C_SOURCE	200809L
#include "internal_server.h"
#include "common.h"
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>

/*
	TODO

	consider internal server structure
	main_internal_server

	done, hooray!
*/

pid_t main_program_pid;
internal_server_t main_internal_server;

[[noreturn]] void internal_error(error_t err) {
	union sigval val = {
		.sival_ptr = err,
	};

	sigqueue(main_program_pid, SERVER_SIGNOTIFY, val);
	exit(1);
}

void cleanup(void) {
	if (!main_internal_server.sock)
		return;

	if (shutdown(main_internal_server.sock, SHUT_RDWR)) {
		internal_error(error_from_errno("unable to shutdown server"));
		return;
	}

	if (close(main_internal_server.sock)) {
		internal_error(error_from_errno("unable to close server socket"));
		return;
	}

}

[[noreturn]] void exit_server(void) {
	cleanup();
	exit(0);
}

static void signal_handler(const int sig, siginfo_t *siginfo, void *ctx) {
	unused(siginfo);
	unused(ctx);

	switch (sig) {
	case SIGINT:	exit_server();

	default: break;
	}
}

static void set_signals(void) {
	struct sigaction act = { 0 };

	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGINT);

	act.sa_sigaction = signal_handler;

	if (sigaction(SIGINT, &act, nullptr))
		internal_error(error_from_errno("unable to set handlers for server process signals"));
}

internal_server_t init_server(void) {
	internal_server_t server = { 0 };

	server.sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server.sock == -1)
		internal_error(error_from_errno("unable to create socket"));

	const int yes = 1;
	if (setsockopt(server.sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)))
		internal_error(error_from_errno("unable to set socket option"));

	server.addr.sin_family = AF_INET;
	server.addr.sin_addr.s_addr = INADDR_ANY;
	server.addr.sin_port = htons(DEFAULT_PORT);

	if (bind(server.sock, (struct sockaddr *)&server.addr, sizeof(server.addr)))
		internal_error(error_from_errno("unable to bind to socket"));

	if (listen(server.sock, DEFAULT_BACKLOG))
		internal_error(error_from_errno("unable to setup socket for listening"));
	
	return server;
}


void internal_server_init(void) {
	main_program_pid = getppid();
	main_internal_server = init_server();

	set_signals();
}

// only need 11 but I want to be safe
#define ADDR_MAX	64

void internal_server_main_loop(void) {
	char addr_str[ADDR_MAX];

	socklen_t peer_addrlen;
	struct sockaddr_in peer_addr;

	for (;;) {
		const int peer = accept(main_internal_server.sock, (struct sockaddr *)&peer_addr, &peer_addrlen);
		if (peer == -1)
			internal_error(error_from_errno("unable to accept connection"));

		inet_ntop(AF_INET, &peer_addr, addr_str, sizeof(addr_str));

		printf("peer connected (%s:%hu)\n", addr_str, ntohs(peer_addr.sin_port));
		write(peer, "hello\n", 6);

		if (shutdown(peer, SHUT_WR))
			internal_error(error_from_errno("unable to shutdown peer's connection"));
	}

	exit_server();
}

#define _POSIX_C_SOURCE	200809L
#include "server.h"
#include "common.h"
#include "internal_server.h"
#include <unistd.h>
#include <signal.h>

static void signal_handler(const int sig, siginfo_t *siginfo, void *ctx) {
	unused(ctx);

	switch (sig) {
	case SIGCHLD: {
		error_t err = siginfo->si_value.sival_ptr;
		warn("internal server error: %s", err);
	} break;

	default: break;
	}
}

static int set_signals(void) {
	struct sigaction act;

	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGCHLD);

	act.sa_sigaction = signal_handler;

	if (sigaction(SIGINT, &act, nullptr))
		return 1;
	
	return 0;
}

void kill_server(const pid_t server_pid) {
	kill(server_pid, SIGKILL);
}

server_t server_host(void) {
	server_t server = { 0 };

	info("hosting...");

	server.pid = fork();
	if (server.pid == -1) {
		 server.err = error_from_errno("unable to spawn server process");
		 return server;
	}

	if (server.pid) {
		if (set_signals()) {
			kill_server(server.pid);

			server.err = error_from_errno("unable to set handler for main process signals");
			return server;
		}

		return server;
	}

	internal_server_init();
	internal_server_main_loop();

	unreachable("server_host");
	return (server_t){ 0 };
}

server_t server_connect(void) {
	info("connecting...");
	return (server_t){ 0 };
}

void server_disconnect(const server_t *server) {
	unused(server);
	info("disconnecting...");
}

void server_terminate(const server_t *server) {
	info("terminating...");

	kill(server->pid, SIGINT);
}

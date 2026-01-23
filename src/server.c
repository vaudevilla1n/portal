#define _POSIX_C_SOURCE	200809L
#include "server.h"
#include "common.h"
#include "internal_server.h"
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

static void signal_handler(const int sig, siginfo_t *siginfo, void *ctx) {
	unused(ctx);

	switch (sig) {
	case SERVER_SIGNOTIFY: {
		char *msg = siginfo->si_value.sival_ptr;
		info("\ninternal server: %s", msg);
	} break;

	case SERVER_SIGERR: {
		error_t err = siginfo->si_value.sival_ptr;
		warn("\ninternal server: error: %s", err);
	} break;

	default: break;
	}
}

static int set_signals(void) {
	struct sigaction act;

	sigemptyset(&act.sa_mask);

	act.sa_sigaction = signal_handler;

	if (sigaction(SIGCHLD, &act, nullptr))
		return 1;
	if (sigaction(SERVER_SIGNOTIFY, &act, nullptr))
		return 1;
	if (sigaction(SERVER_SIGERR, &act, nullptr))
		return 1;
	
	return 0;
}

static void kill_server(const pid_t server_pid) {
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
	if (!server->pid) {
		warn("no server running");
		return;
	}

	info("terminating...");

	kill(server->pid, SIGINT);
	waitpid(server->pid, NULL, 0);
}

#define _POSIX_C_SOURCE	200809L
#include "tui.h"
#include "server.h"
#include "common.h"
#include "internal_server.h"
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

static void signal_handler(const int sig, siginfo_t *siginfo, void *ctx) {
	unused(ctx);

	switch (sig) {
	case SIGCHLD: {
		tui_info("internal server exited");
	} break;

	case SERVER_SIGNOTIFY: {
		char *msg = siginfo->si_value.sival_ptr;
		tui_info(msg);
	} break;

	case SERVER_SIGERR: {
		char *err = siginfo->si_value.sival_ptr;
		tui_warn(err);
	} break;

	default: break;
	}
}

static int set_signals(void) {
	struct sigaction act;

	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGCHLD);
	sigaddset(&act.sa_mask, SERVER_SIGNOTIFY);
	sigaddset(&act.sa_mask, SERVER_SIGERR);

	act.sa_flags = SA_SIGINFO | SA_NOCLDSTOP;
	act.sa_sigaction = signal_handler;

	if (sigaction(SIGCHLD, &act, nullptr)
		|| sigaction(SERVER_SIGNOTIFY, &act, nullptr)
		|| sigaction(SERVER_SIGERR, &act, nullptr))
		return 1;
	
	return 0;
}


server_t server_init(void) {
	server_t server = { 0 };

	if (set_signals()) {
		server.err = "unable to set handler for main process signals";
		return server;
	}

	server.pid = fork();
	if (server.pid == -1) {
		 server.err = "unable to spawn server process";
		 return server;
	}

	if (server.pid)
		return server;

	internal_server_init();
	internal_server_main_loop();

	__unreachable("server_init");
}

static void internal_server_action(server_t *server, const server_act_t act) {
	union sigval val = {
		.sival_int = act,
	};

	if (sigqueue(server->pid, SERVER_SIGNOTIFY, val))
		server->err = "unable to signal internal server";
}

// should be called deinit but terminate sounds cooler
void server_terminate(server_t *server) {
	if (!server->pid) {
		tui_warn("no server running");
		return;
	}

	tui_info("terminating server...");
	internal_server_action(server, SERVER_QUIT);

	waitpid(server->pid, nullptr, 0);
}

void server_host(server_t *server) {
	tui_info("hosting...");

	internal_server_action(server, SERVER_HOST);

	if (server->err)
		tui_warn("unable to host server");
}

void server_unhost(server_t *server) {
	tui_info("unhosting...");
	internal_server_action(server, SERVER_UNHOST);
}

void server_connect(server_t *server) {
	tui_info("connecting...");

	internal_server_action(server, SERVER_CONNECT);

	if (server->err)
		tui_warn("unable to connect to server");
}

void server_disconnect(server_t *server) {
	tui_info("disconnecting...");
	internal_server_action(server, SERVER_DISCONNECT);
}

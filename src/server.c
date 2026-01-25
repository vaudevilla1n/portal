#define _POSIX_C_SOURCE	200809L
#include "tui.h"
#include "server.h"
#include "common.h"
#include "internal_server.h"
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

server_t main_server;

static void signal_handler(const int sig, siginfo_t *siginfo, void *ctx) {
	unused(ctx);

	switch (sig) {
	case SIGCHLD: {
		main_server.running = false;
	} break;

	case SERVER_SIGNOTIFY: {
		char *notif = siginfo->si_value.sival_ptr;
		main_server.notif = notif;
	} break;

	case SERVER_SIGERR: {
		char *err = siginfo->si_value.sival_ptr;
		main_server.err = err;
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


void server_init(void) {
	main_server.running = true;

	die_if(set_signals(), "unable to set handler for main process signals: %s", errno_string);

	main_server.pid = fork();
	die_if(main_server.pid == -1, "unable to spawn server process: %s", errno_string);

	if (main_server.pid)
		return;

	internal_server_init();
	internal_server_main_loop();
}

static void internal_server_action(const server_status_t status) {
	union sigval val = {
		.sival_int = status,
	};

	if (sigqueue(main_server.pid, SERVER_SIGNOTIFY, val))
		main_server.err = "unable to signal internal server";
}

// should be called deinit but terminate sounds cooler
void server_terminate(void) {
	if (main_server.pid <= 0) {
		tui_warn("no server running");
		return;
	}

	tui_info("terminating server...");
	internal_server_action(SERVER_QUIT);

	waitpid(main_server.pid, nullptr, 0);
}

void server_host(void) {
	tui_info("hosting...");

	internal_server_action(SERVER_HOST);

	if (main_server.err)
		tui_warn("unable to host server");
}

void server_unhost(void) {
	tui_info("unhosting...");
	internal_server_action(SERVER_UNHOST);
}

void server_connect(void) {
	tui_info("connecting...");

	internal_server_action(SERVER_CONNECT);

	if (main_server.err)
		tui_warn("unable to connect to server");
}

void server_disconnect(void) {
	tui_info("disconnecting...");
	internal_server_action(SERVER_DISCONNECT);
}

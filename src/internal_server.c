#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE	200809L
#include "internal_server.h"
#include "common.h"
#include <signal.h>
#include <stdarg.h>
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


#define NOTIFICATION_MAX	4096

static void internal_notification(char *notif) {
	union sigval val = {
		.sival_ptr = notif,
	};

	sigqueue(main_program_pid, SERVER_SIGNOTIFY, val);
}


static void cleanup(void) {
	if (!main_internal_server.sock)
		return;

	shutdown(main_internal_server.sock, SHUT_RDWR);
	close(main_internal_server.sock);
}

static void internal_error(char * err) {
	union sigval val = {
		.sival_ptr = err,
	};

	sigqueue(main_program_pid, SERVER_SIGERR, val);

	cleanup();
	exit(1);
}


static void exit_server(void) {
	if (!main_internal_server.running)
		return;

	cleanup();
	main_internal_server.running = false;
	exit(0);
}


// only need 11 but I want to be safe
#define ADDR_MAX	64
static char notif[NOTIFICATION_MAX];

static void host_server(void) {
	if (bind(main_internal_server.sock, (struct sockaddr *)&main_internal_server.addr, sizeof(main_internal_server.addr)))
		internal_error("unable to bind to socket");
	if (listen(main_internal_server.sock, DEFAULT_BACKLOG))
		internal_error("unable to setup socket for listening");

	char addr_str[ADDR_MAX];

	socklen_t peer_addrlen;
	struct sockaddr_in peer_addr;

	for (;;) {
		const int peer = accept(main_internal_server.sock, (struct sockaddr *)&peer_addr, &peer_addrlen);
		if (peer == -1)
			internal_error("unable to accept connection");

		inet_ntop(AF_INET, &peer_addr, addr_str, sizeof(addr_str));

		snprintf(notif, sizeof(notif), "peer connected (%s:%hu)", addr_str, ntohs(peer_addr.sin_port));
		internal_notification(notif);

		write(peer, "hello\n", 6);

		if (shutdown(peer, SHUT_WR))
			internal_error("unable to shutdown peer's connection");
	}
}


/*
	TODO

	complete the rest of the server actions
*/
static void signal_handler(const int sig, siginfo_t *siginfo, void *ctx) {
	unused(ctx);

	switch (sig) {
	case SIGINT:			exit_server(); break;

	case SERVER_SIGNOTIFY: {
		const server_act_t act = siginfo->si_value.sival_int;

		switch (act) {
		case SERVER_QUIT:	exit_server(); break;

		case SERVER_HOST:	host_server(); break;
		case SERVER_UNHOST: 	return;

		case SERVER_CONNECT: 	return;
		case SERVER_DISCONNECT:	return;

		}
	} break;

	default: break;
	}
}

static void set_signals(void) {
	struct sigaction act = { 0 };

	act.sa_flags = SA_SIGINFO;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SERVER_SIGNOTIFY);

	act.sa_sigaction = signal_handler;

	if (sigaction(SIGINT, &act, nullptr) || sigaction(SERVER_SIGNOTIFY, &act, nullptr))
		internal_error("unable to set handlers for server process signals");
}


void internal_server_init(void) {
	main_program_pid = getppid();
	main_internal_server = (internal_server_t){ 0 };

	main_internal_server.sock = socket(AF_INET, SOCK_STREAM, 0);
	if (main_internal_server.sock == -1)
		internal_error("unable to create socket");

	const int yes = 1;
	if (setsockopt(main_internal_server.sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)))
		internal_error("unable to set socket option");

	main_internal_server.addr.sin_family = AF_INET;
	main_internal_server.addr.sin_addr.s_addr = INADDR_ANY;
	main_internal_server.addr.sin_port = htons(DEFAULT_PORT);

	set_signals();

	main_internal_server.start_time = time(NULL);
	main_internal_server.running = true;
}


static void wait_for_signal(void) {
	// TODO sigsuspend
	pause();
}

void internal_server_main_loop(void) {
	for (;;)
		wait_for_signal();
	
	exit_server();
}

#pragma once

#include "common.h"
#include "server_signals.h"
#include <sys/types.h>

/*
	thats all folks
*/

typedef struct {
	bool running;

	pid_t pid;

	char *err;
	char *notif;
} server_t;

extern server_t main_server;

void server_init(void);
void server_terminate(void);

void server_host(void);
void server_unhost(void);

void server_connect(void);
void server_disconnect(void);

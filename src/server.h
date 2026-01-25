#pragma once

#include "common.h"
#include "server_signals.h"
#include <sys/types.h>

/*
	thats all folks
*/

typedef struct {
	pid_t pid;
	char *err;
} server_t;

server_t server_init(void);
void server_terminate(server_t *server);

void server_host(server_t *server);
void server_unhost(server_t *server);

void server_connect(server_t *server);
void server_disconnect(server_t *server);

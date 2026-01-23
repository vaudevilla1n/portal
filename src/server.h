#pragma once

#include "common.h"
#include "server_signals.h"
#include <sys/types.h>

/*
	thats all folks
*/

typedef struct {
	pid_t pid;
	error_t err;
} server_t;

server_t server_host(void);
server_t server_connect(void);

void server_disconnect(const server_t *server);
void server_terminate(const server_t *server);

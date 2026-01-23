#pragma once
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>

/*
	thats all folks
*/

typedef struct {
	int sock;
	struct sockaddr_in sin;

	time_t start_time;

	const char *err;
} server_t;

server_t server_host(void);
server_t server_connect(void);

void server_disconnect(server_t *s);
void server_terminate(server_t *s);

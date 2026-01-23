#pragma once

#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define DEFAULT_ADDR	INET_ANY
#define DEFAULT_PORT	9166
#define DEFAULT_BACKLOG	16

#define SERVER_SIGNOTIFY	SIGCHLD

typedef struct {
	int sock;
	struct sockaddr_in addr;

	time_t start_time;
} internal_server_t;

void internal_server_init(void);
void internal_server_main_loop(void);

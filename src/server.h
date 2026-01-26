#pragma once

#include "common.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_DEFAULT_PORT	9166
#define SERVER_DEFAULT_BACKLOG	16

#define SERVER_INFO_MAX		4096

/*
	thats all folks
*/

typedef enum {
	SERVER_DEAD,

	SERVER_IDLE,

	SERVER_HOST,
	SERVER_CONNECT,
} server_status_t;

typedef enum {
	SERVER_INFO_QUIET,

	SERVER_INFO_ERR,
	SERVER_INFO_NOTIF,
} server_info_t;

typedef struct {
	struct sockaddr_in addr;
	int sock;

	time_t start_time;

	server_status_t status;

	char *info;
	server_info_t info_type;
} server_t;

server_t server_init(void);
void server_terminate(server_t *server);

void server_host(server_t *server);
void server_handle_connection(server_t *server);
void server_unhost(server_t *server);

void server_connect(server_t *server);
void server_disconnect(server_t *server);

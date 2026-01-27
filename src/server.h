#pragma once

#include "common.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_DEFAULT_PORT	9166
#define SERVER_CONNECTION_MAX	256

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
} server_info_type_t;

typedef struct {
	char text[SERVER_INFO_MAX];
	server_info_type_t type;
} server_info_t;

typedef struct {
	struct sockaddr_in addr;
	int sock;

	time_t start_time;

	server_status_t status;

	server_info_t info;
} server_t;

extern server_t server_internal_main;

void server_init(void);
void server_terminate(void);

bool server_error(void);

server_info_t server_read_info(void);

void server_host(void);
void server_probe(void);
void server_unhost(void);

void server_connect(void);
void server_disconnect(void);

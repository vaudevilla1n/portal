#pragma once

#include "common.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_EOF		(-1)

#define SERVER_DEFAULT_ADDR	INADDR_ANY
#define SERVER_DEFAULT_PORT	9166

#define SERVER_CONNECTION_MAX	256

#define SERVER_INFO_MAX		4096

#define SERVER_MSG_MAX		1024
#define SERVER_MSG_SIZE		(SERVER_MSG_MAX + 8)

/*
	thats all folks
*/

typedef enum {
	SERVER_DEAD,

	SERVER_IDLE,

	SERVER_HOST,
	SERVER_CONNECT,
} server_status_t;

void server_init(void);

void server_host(const char *addr, const char *port);
void server_unhost(void);

void server_connect(const char *addr, const char *port);
void server_disconnect(void);

void server_status(void);

#define SERVER_MSG_FMT(uid, msg) \
	"(user%d): %s", (uid), (msg)
void server_send_message(const int userid, const char *text);

void server_iteration(void);

void server_terminate(void);

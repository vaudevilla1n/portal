#include "common.h"
#include "server.h"

__attribute((unused)) server_t init_server(void) {
	todo("init_server");
}

server_t server_host(void) {
	info("hosting...");
	return (server_t){ 0 };
}

server_t server_connect(void) {
	info("connecting...");
	return (server_t){ 0 };
}

void server_disconnect(server_t *s) {
	unused(s);
	info("disconnecting...");
}

void server_terminate(server_t *s) {
	unused(s);
	info("terminating...");
}

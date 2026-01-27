#define _DEFAULT_SOURCE
#include "tui.h"
#include "common.h"
#include "server.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

typedef enum {
	CMD_UNKNOWN,
	CMD_QUIT,
	CMD_HOST,
	CMD_CONN,
} cmd_t;

bool is_command(const char *input, const size_t len);
cmd_t command(const char *cmd);


typedef enum {
	CLIENT_IDLE,
	CLIENT_HOSTING,
	CLIENT_CONNECTED,
} client_status_t;

typedef struct {
	int user_id;
	client_status_t status;
} client_t;

client_t client_new(void);


static void send_chat(const int user_id, const char *input) {
	char line[TUI_INPUT_MAX + 256];
	snprintf(line, sizeof(line), "[user%d]: %s", user_id, input);

	tui_puts(line);
}

static void report_server_info(void) {
	const server_info_t info = server_read_info();
	switch (info.type) {
	case SERVER_INFO_ERR:	
	case SERVER_INFO_NOTIF: {
		tui_puts(info.text);
	} break;

	default:
		break;
	}
}

static void handle_server_status(void) {
	switch (server_internal_main.status) {
	case SERVER_HOST: {
		server_handle_connection();
		report_server_info();
	} break;
	
	default:
		break;
	}
}

static void command_server(const cmd_t cmd) {
	switch (cmd) {
	case CMD_HOST: server_host(); break;

	case CMD_CONN: server_connect(); break;

	case CMD_UNKNOWN: tui_puts("unknown command"); break;

	default: __unreachable("main: command");
	}
}

int main(void) {
	server_init();

	if (server_error())
		die("unable to initialize portal server: %s", server_read_info().text);

	client_t client = client_new();

	tui_enter();

	for (;;) {
		tui_draw();

		report_server_info();

		handle_server_status();

		const char *input = tui_prompt();
		if (!input)
			continue;

		const size_t len = strlen(input);

		if (is_command(input, len)) {
			const cmd_t cmd = command(input);

			if (cmd == CMD_QUIT)
				break;

			command_server(cmd);
		} else {
			send_chat(client.user_id, input);
		}
	}

	server_terminate();

	tui_exit();

	puts("later");
}

client_t client_new(void) {
	return (client_t) {
		.user_id = time(NULL),
		.status = CLIENT_IDLE,
	};
}

bool is_command(const char *input, const size_t len) {
	if (len <= 1)
		return false;

	return input[0] == '\\' && input[1] != '\\';
}

cmd_t command(const char *cmd) {
	// skip '\'
	cmd++;

	if (STREQ(cmd, "host"))
		return CMD_HOST;
	if (STREQ(cmd, "connect"))
		return CMD_CONN;
	if (STREQ(cmd, "quit"))
		return CMD_QUIT;

	return CMD_UNKNOWN;
}


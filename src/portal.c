#define _DEFAULT_SOURCE
#include "tui.h"
#include "common.h"
#include "server.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef enum {
	CMD_UNKNOWN,
	CMD_QUIT,
	CMD_HOST,
	CMD_CONN,
} cmd_t;

bool is_command(const char *input, const size_t len);
cmd_t run_command(const char *cmd);


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
	char line[TUI_LINE_MAX];
	snprintf(line, sizeof(line), "[user%d]: %s\n", user_id, input);

	tui_push_line(line);
}

int main(void) {
	server_t server = server_init();
	if (server.err)
		die("unable to initialize portal server: %s", server.err);

	client_t client = client_new();

	// tui_enter();

	char input[4096];
	for (;;) {
		/*
		size_t input_len;
		const char *input = tui_read_line(&input_len);
		*/

		if (!fgets(input, sizeof(input), stdin)) {
			printf("\n");
			break;
		}

		const size_t input_len = strcspn(input, "\n");
		input[input_len] = '\0';

		if (!input_len)
			continue;

		if (!is_command(input, input_len)) {
			send_chat(client.user_id, input);
			continue;
		}

		const cmd_t cmd = run_command(input);

		if (cmd == CMD_QUIT)
			break;
		
		switch (cmd) {
		case CMD_HOST: server_host(&server); break;

		case CMD_CONN: server_connect(&server); break;

		// skip '\'
		case CMD_UNKNOWN: tui_warn("unknown command"); break;

		default: __unreachable("main: command");
		}
	}

	server_terminate(&server);

	// tui_exit();

	printf("later\n");
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

cmd_t run_command(const char *cmd) {
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


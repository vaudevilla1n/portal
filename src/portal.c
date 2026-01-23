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

cmd_t run_command(const char *cmd);


void send_chat(const int user_id, const char *input) {
	printf("[user%d]: %s\n", user_id, input);
}

static inline bool is_command(const char *input, const size_t len) {
	if (len <= 1)
		return false;

	return input[0] == '\\' && input[1] != '\\';
}

static inline void print_prompt(void) {
	printf("]> ");
	fflush(stdout);
}

void assert_tty(void) {
	if (!isatty(STDIN_FILENO))
		die("stdin is not a tty. exiting...");
	if (!isatty(STDOUT_FILENO))
		die("stdout is not a tty. exiting...");
}

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
void client_cleanup(const client_t *client, server_t *s);

#define INPUT_MAX	4096

int main(void) {
	assert_tty();

	server_t server;
	client_t client = client_new();

	char input[INPUT_MAX];
	for (;;) {
		print_prompt();

		if (!fgets(input, sizeof(input), stdin)) {
			printf("\n");
			break;
		}
		const size_t input_len = strcspn(input, "\n");
		if (!input_len)
			continue;

		input[input_len] = '\0';

		if (!is_command(input, input_len)) {
			send_chat(client.user_id, input);
			continue;
		}

		const cmd_t cmd = run_command(input);

		if (cmd == CMD_QUIT)
			break;
		
		switch (cmd) {
		case CMD_HOST: {
			server = server_host();

			if (!server.err) {
				client.status = CLIENT_HOSTING;
				break;
			}

			server_terminate(&server);

			warn("unable to host server: %s", server.err);
		} break;

		case CMD_CONN: {
			server = server_connect();

			if (!server.err) {
				client.status = CLIENT_CONNECTED;
				break;
			}

			warn("unable to host server: %s", server.err);
		} break;

		// skip '\'
		case CMD_UNKNOWN: {
			warn("unknown command: '%s'\n", input + 1); break;
		} break;

		default: unreachable("main: command");
		}
	}

	client_cleanup(&client, &server);

	printf("later\n");
}

client_t client_new(void) {
	return (client_t) {
		.user_id = time(NULL),
		.status = CLIENT_IDLE,
	};
}

void client_cleanup(const client_t *client, server_t *s) {
	switch (client->status) {
	case CLIENT_HOSTING:	server_terminate(s); break;
	case CLIENT_CONNECTED:	server_disconnect(s); break;

	default:		break;
	}
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


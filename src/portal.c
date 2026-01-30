#define _DEFAULT_SOURCE

#include "tui.h"
#include "common.h"
#include "server.h"
#include "command.h"

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

static void command_server(const cmd_t cmd) {
	const char *addr = cmd.argc > 0 ? cmd.argv[0] : nullptr;
	const char *port = cmd.argc > 1 ? cmd.argv[1] : nullptr;

	if ((addr || port) && (cmd.type == CMD_UNHOST || CMD_DISCONN || CMD_STATUS))
		tui_warn("too many arguments specified");

	switch (cmd.type) {
	case CMD_HOST:			server_host(addr, port); break;
	case CMD_UNHOST:		server_unhost(); break;

	case CMD_CONN:			server_connect(addr, port); break;
	case CMD_DISCONN:		server_disconnect(); break;

	case CMD_STATUS:		server_status(); break;

	case CMD_UNKNOWN:		break;

	default: __unreachable("main: command");
	}
}

int main(void) {
	server_init();

	const int user_id = time(NULL);

	tui_set_prompt(user_id);
	tui_enter();

	for (;;) {
		server_iteration();

		tui_draw();

		size_t len = 0;
		char *input = tui_repl(&len);

		if (!input || !len)
			continue;

		if (is_command(input)) {
			const cmd_t cmd = command_parse(input);

			if (cmd.type == CMD_QUIT)
				break;

			command_server(cmd);
		} else {
			server_send_message(user_id, input);
		}

		free(input);
	}

	server_terminate();

	tui_exit();

	puts("later");
}

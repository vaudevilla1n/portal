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

int main(void) {
	server_init();

	const int user_id = time(NULL);

	tui_set_prompt(user_id);
	tui_enter();

	char input[TUI_INPUT_MAX + 1];

	for (;;) {
		server_iteration();

		tui_draw();

		const ptrdiff_t len = tui_read_line(input, sizeof(input));
		if (!len)
			continue;

		if (is_command(input, len)) {
			const cmd_t cmd = command_parse(input);

			if (cmd.type == CMD_QUIT)
				break;

			server_command(cmd);
		} else {
			server_send_message(user_id, input);
		}
	}

	server_terminate();

	tui_exit();

	puts("later");
}

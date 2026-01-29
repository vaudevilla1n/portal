#include "command.h"

#include "tui.h"
#include "common.h"

#include <string.h>

#define WHITESPACE	" \b\t\n"

bool is_command(const char *input) {
	return input[0] == '\\' && input[1] != '\\';
}

cmd_t command_parse(char *input) {
	cmd_t cmd = { 0 };

	const char *type = strtok(input, WHITESPACE);
	// skip '\'
	type++;

	/* ik ik its slow but im too lazy to make a map */
	if (STREQ(type, "host")) {
		cmd.type = CMD_HOST;
	} else if (STREQ(type, "unhost")) {
		cmd.type = CMD_UNHOST;
	} else if (STREQ(type, "connect")) {
		cmd.type = CMD_CONN;
	} else if (STREQ(type, "disconnect")) {
		cmd.type = CMD_DISCONN;
	} else if (STREQ(type, "quit")) {
		cmd.type = CMD_QUIT;
	} else {
		tui_warn("unknown command: '%s'", type);
		goto unknown_command;
	}

	const char *arg;
	while ((arg = strtok(NULL, WHITESPACE)) && cmd.argc < CMD_ARGC_MAX)
		cmd.argv[cmd.argc++] = arg;
	
	if (cmd.argc == CMD_ARGC_MAX && strtok(NULL, WHITESPACE)) {
		tui_warn("too many arguments (max %d)", CMD_ARGC_MAX);
		goto unknown_command;
	}

	return cmd;

unknown_command:
	cmd.type = CMD_UNKNOWN;
	return cmd;
}


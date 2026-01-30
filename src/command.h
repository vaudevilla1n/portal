#pragma once

#include <stddef.h>

#define CMD_ARGC_MAX	2

typedef enum {
	CMD_UNKNOWN,

	CMD_QUIT,

	CMD_STATUS,

	CMD_HOST,
	CMD_UNHOST,

	CMD_CONN,
	CMD_DISCONN,
} cmd_type_t;


typedef struct {
	cmd_type_t type;

	size_t argc;
	const char *argv[CMD_ARGC_MAX];
} cmd_t;

bool is_command(const char *input);

cmd_t command_parse(char *input);

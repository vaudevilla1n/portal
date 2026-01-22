#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define BOLD	"\033[1m"
#define GREEN	"\033[38;2;0;255;0m"
#define RESET	"\033[0m"

#define errno_string	strerror(errno)

#define info(fmt, ...)	\
	do { printf(BOLD GREEN fmt RESET __VA_OPT__(,)__VA_ARGS__ ); } while (0)
#define warn(fmt, ...)	\
	do { fprintf(stderr, fmt __VA_OPT__(,)__VA_ARGS__); fputc('\n', stderr); } while (0)
#define die(fmt, ...)	\
	do { warn(fmt __VA_OPT__(,)__VA_ARGS__); exit(1); } while (0)

#define STREQ(s, t)	(!strcmp((s), (t)))


typedef enum {
	CMD_IDLE,
	CMD_QUIT,
	CMD_HOST,
	CMD_CONN,
} cmd_t;

static inline bool is_command(const char *input) {
	return input[0] == '\\';
}

static cmd_t run_command(const char *cmd) {
	// skip '\'
	cmd++;

	if (STREQ(cmd, "host")) {
		printf("hosting...\n");
		return CMD_HOST;
	}

	if (STREQ(cmd, "connect")) {
		printf("connecting...\n");
		return CMD_CONN;
	}

	if (STREQ(cmd, "quit")) {
		printf("later\n");
		return CMD_CONN;
	}

	printf("unknown command: %s\n", cmd);
	return CMD_IDLE;
}

static inline void print_prompt(void) {
	printf("]>");
	fflush(stdout);
}

static void assert_tty(void) {
	if (!isatty(STDIN_FILENO))
		die("stdin is not a tty. exiting...");
	if (!isatty(STDOUT_FILENO))
		die("stdout is not a tty. exiting...");
}

#define INPUT_MAX	4096

int main(void) {
	assert_tty();

	char input[INPUT_MAX];

	for (;;) {
		print_prompt();

		if (!fgets(input, sizeof(input), stdin)) {
			printf("\n");
			break;
		}
		const size_t input_len = strcspn(input, "\n");
		input[input_len] = '\0';

		if (!input_len)
			continue;

		if (is_command(input)) {
			run_command(input);
		}
	}
}

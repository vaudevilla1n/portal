#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE	200809L
#include "tui.h"
#include "ansi.h"
#include "common.h"
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>

typedef struct {
	char lines[TUI_DISPLAY_MAX][TUI_LINE_MAX];
	ptrdiff_t pos;
	ptrdiff_t len;
} display_buffer_t;

typedef struct {
	char dat[TUI_INPUT_MAX];
	ptrdiff_t len;
} input_buffer_t;

typedef struct {
	struct termios prev_attr;
	struct termios attr;

	ptrdiff_t rows;
	ptrdiff_t cols;

	bool update;
	bool resize;
} tui_context_t;

input_buffer_t tui_input_buffer;
display_buffer_t tui_display_buffer;
tui_context_t tui_internal_context;

bool tui_prompt_set = false;
char tui_prompt[TUI_PROMPT_MAX];

#define TUI_TITLE		"(portal)"
#define TUI_DEFAULT_PROMPT	")> "

void tui_set_prompt(const int user_id) {
	snprintf(tui_prompt, TUI_PROMPT_MAX, "(user%d)> ", user_id);
	tui_prompt_set = true;
}

static void display_prompt(void) {
	ansi_move(1, TUI_HEIGHT);
	ansi_clear_line();
	printf("%s", tui_prompt);

	if (tui_input_buffer.len) {
		tui_input_buffer.dat[tui_input_buffer.len] = '\0';
		fputs(tui_input_buffer.dat, stdout);
	}
}

static void display_title(void) {
	ansi_move(1, 1);
	ansi_clear_line();
	printf(ANSI_BOLD TUI_TITLE ANSI_RESET);
}

static void signal_handler(int sig) {
	switch (sig) {
	case SIGWINCH:	tui_internal_context.resize = true; break;

	default:
		break;
	}
}

static void set_signals(void) {
	struct sigaction act = { 0 };

	sigemptyset(&act.sa_mask);
	act.sa_handler = signal_handler;

	die_if(sigaction(SIGWINCH, &act, nullptr), 
			"unable to set signal handler for tui: %s", errno_string);
}

static void set_dimensions(void) {
	struct winsize ws;

	die_if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1,
			"unable to get terminal dimensions: %s", errno_string);
	
	tui_internal_context.rows = ws.ws_row;
	tui_internal_context.cols = ws.ws_col;
	tui_internal_context.update = true;
}

static void assert_tty(void) {
	die_if(!isatty(STDIN_FILENO), "stdin is not a tty. exiting...");
	die_if(!isatty(STDOUT_FILENO), "stdout is not a tty. exiting...");
}


void tui_enter(void) {
	tui_internal_context = (tui_context_t){ 0 };

	assert_tty();

	die_if(tcgetattr(STDOUT_FILENO, &tui_internal_context.prev_attr),
			"unable to get terminal attributes: %s", errno_string);

	cfmakeraw(&tui_internal_context.attr);
	die_if(tcsetattr(STDOUT_FILENO, TCSANOW, &tui_internal_context.attr),
			"unable to set terminal attributes: %s", errno_string);

	die_if(fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK), "unable to set stdin as nonblocking: %s", errno_string);

	set_signals();

	ansi_cursor_visible(false);
	ansi_clear();

	tui_internal_context.resize = true;
	tui_internal_context.update = true;

	if (!tui_prompt_set) {
		snprintf(tui_prompt, TUI_PROMPT_MAX, TUI_DEFAULT_PROMPT);
	}
}

void tui_exit(void) {
	ansi_clear();
	ansi_cursor_visible(true);

	die_if(tcsetattr(STDOUT_FILENO, TCSANOW, &tui_internal_context.prev_attr),
			"unable to set terminal attributes: %s", errno_string);

	tui_internal_context = (tui_context_t){ 0 };
}


static inline ptrdiff_t display_buffer_idx(const ptrdiff_t i) {
	return (tui_display_buffer.pos + i) % countof(tui_display_buffer.lines);
}
static inline char *display_buffer_at(const ptrdiff_t i) {
	return tui_display_buffer.lines[display_buffer_idx(i)];
}

void tui_draw(void) {
	if (!tui_internal_context.update && !tui_internal_context.resize)
		return;

	if (tui_internal_context.resize) {
		tui_internal_context.resize = false;
		set_dimensions();
	}

	tui_internal_context.update = false;

	const ptrdiff_t view_lines = (TUI_VIEW_END - TUI_VIEW_START) + 1;
	const ptrdiff_t start_line = MAX(0, tui_display_buffer.len - view_lines);

	ansi_clear();
	ansi_move(1, TUI_VIEW_START);
	for (ptrdiff_t i = start_line; i < tui_display_buffer.len; i++) {
		ansi_clear_line();
		printf("%s\r\n", display_buffer_at(i));
	}
	
	display_title();
	display_prompt();
}

void tui_puts(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	if (tui_display_buffer.len == TUI_DISPLAY_MAX) {
		tui_display_buffer.len--;
		tui_display_buffer.pos = display_buffer_idx(1);
	}

	char *line = display_buffer_at(tui_display_buffer.len);
	vsnprintf(line, TUI_LINE_MAX, fmt, args);

	tui_display_buffer.len++;

	tui_internal_context.update = true;

	va_end(args);
}


static const char *read_line(void) {
	bool line_read = false;
	bool reading = true;

	while (reading) {
		const int c = getchar();

		switch (c) {
		case EOF: {
			reading = false;
		} break;

		case ANSI_ENTER: {
			if (tui_input_buffer.len)
				line_read = true;

			reading = false;
		} break;

		case ANSI_BS: {
			if (!tui_input_buffer.len)
				continue;

			ansi_move_left(1);
			ansi_clear_line();
			tui_input_buffer.len--;
		} break;

		default: {
			if (tui_input_buffer.len == TUI_INPUT_MAX)
				break;

			if (!isprint(c))
				break;

			putchar(c);
			tui_input_buffer.dat[tui_input_buffer.len++] = c;
		} break;
		}
	}

	if (line_read) {
		tui_input_buffer.dat[tui_input_buffer.len] = '\0';
		tui_input_buffer.len = 0;

		return tui_input_buffer.dat;
	} else {
		return nullptr;
	}
}

const char *tui_repl(void) {
	const char *input = read_line();

	if (!input)
		return nullptr;

	display_prompt();
	return input;
}

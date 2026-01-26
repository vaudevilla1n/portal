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

tui_context_t tui_main_context = { 0 };

static void assert_tty(void) {
	die_if(!isatty(STDIN_FILENO), "stdin is not a tty. exiting...");
	die_if(!isatty(STDOUT_FILENO), "stdout is not a tty. exiting...");
}

static void set_dimensions(void) {
	struct winsize ws;

	die_if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1,
			"unable to get terminal dimensions: %s", errno_string);

	tui_main_context.rows = ws.ws_row;
	tui_main_context.cols = ws.ws_col;
}

static void signal_handler(int sig) {
	switch (sig) {
	case SIGWINCH:	set_dimensions(); break;

	default:	break;
	}
}

static void set_signals(void) {
	struct sigaction act = { 0 };

	sigemptyset(&act.sa_mask);
	act.sa_handler = signal_handler;

	die_if(sigaction(SIGWINCH, &act, nullptr), 
			"unable to set signal handler for tui: %s", errno_string);
}

#define TUI_PROMPT	"]> "
#define TUI_TITLE	"(portal)"

void tui_display_prompt(void) {
	ansi_move(1, TUI_HEIGHT);
	ansi_clear_line();
	printf(TUI_PROMPT);
}

void tui_display_title(void) {
	ansi_move(1, 1);
	ansi_clear_line();
	printf(ANSI_BOLD TUI_TITLE ANSI_RESET);
}

void tui_enter(void) {
	tui_main_context = (tui_context_t){ 0 };

	assert_tty();

	die_if(tcgetattr(STDOUT_FILENO, &tui_main_context.prev_attr),
			"unable to get terminal attributes: %s", errno_string);

	cfmakeraw(&tui_main_context.attr);
	die_if(tcsetattr(STDOUT_FILENO, TCSANOW, &tui_main_context.attr),
			"unable to set terminal attributes: %s", errno_string);

	die_if(fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK), "unable to set stdin as nonblocking: %s", errno_string);

	set_signals();

	set_dimensions();

	ansi_cursor_visible(false);
	ansi_clear();

	tui_display_title();
	tui_display_prompt();
}

void tui_exit(void) {
	ansi_clear();
	ansi_cursor_visible(true);

	die_if(tcsetattr(STDOUT_FILENO, TCSANOW, &tui_main_context.prev_attr),
			"unable to set terminal attributes: %s", errno_string);

	tui_main_context = (tui_context_t){ 0 };
}


void tui_put_line(const int line, const char *text) {
	ansi_move(1, line);
	ansi_clear_line();

	fputs(text, stdout);
	fflush(stdout);
}


static void scroll_down(void) {
	// clear title
	ansi_move(0, 0);
	ansi_clear_line();

	// clear prompt
	ansi_move(0, TUI_HEIGHT);
	ansi_clear_line();

	ansi_scroll_down();

	tui_display_title();
	tui_display_prompt();
}

void tui_push_line(const char *text) {
	ansi_reset_video();

	static int curr_line = TUI_VIEW_START;

	if (curr_line <= TUI_VIEW_END) {
		tui_put_line(curr_line, text);
		curr_line++;
	} else {
		scroll_down();
		tui_put_line(TUI_VIEW_END, text);
	}

	ansi_reset_video();
	tui_display_prompt();
}

bool tui_read_line(char *buf, const size_t size) {
	static size_t len = 0;

	bool line_read = false;
	bool reading = true;

	while (reading) {
		if (len == size) {
			line_read = true;
			break;
		}

		const int c = getchar();

		switch (c) {
		case EOF: {
			reading = false;
		} break;

		case ANSI_ENTER: {
			if (len)
				line_read = true;

			reading = false;
		} break;

		case ANSI_BS: {
			if (!len)
				continue;

			ansi_move_left(1);
			ansi_clear_line();
			len--;
		} break;

		default: {
			putchar(c);
			buf[len++] = c;
		} break;
		}
	}

	if (line_read) {
		buf[len] = '\0';
		len = 0;
	}

	return line_read;
}

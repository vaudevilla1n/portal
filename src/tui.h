#pragma once

#include <stddef.h>
#include <termios.h>

typedef struct {
	struct termios prev_attr;
	struct termios attr;

	int rows;
	int cols;
} tui_context_t;

extern tui_context_t tui_main_context;

#define	TUI_HEIGHT	(tui_main_context.rows)	
#define	TUI_WIDTH	(tui_main_context.cols)

#define	TUI_VIEW_START	(2)	
#define	TUI_VIEW_END	(TUI_HEIGHT - 1)

#define TUI_LINE_MAX	4096

void tui_display_prompt(void);
void tui_display_title(void);

void tui_enter(void);
void tui_exit(void);

bool tui_read_line(char *buf, const size_t size);

void tui_put_line(const int line, const char *text);

void tui_push_line(const char *text);

#define tui_display_status(fmt)	tui_put_line(TUI_HEIGHT, fmt)

#define tui_warn(msg)	\
	do { printf(ANSI_BOLD ANSI_YELLOW); tui_push_line(msg); printf(ANSI_RESET); fflush(stdout); } while (0)
#define tui_info(msg)	\
	do { printf(ANSI_BOLD ANSI_BLUE); tui_push_line(msg); printf(ANSI_RESET); fflush(stdout); } while (0)


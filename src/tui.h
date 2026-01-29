#pragma once

#include <stddef.h>
#include <termios.h>

#define	TUI_HEIGHT	(tui_internal_context.rows)	
#define	TUI_WIDTH	(tui_internal_context.cols)

#define	TUI_VIEW_START	(2)	
#define	TUI_VIEW_END	(TUI_HEIGHT - 1)

#define TUI_LINE_MAX		4096
#define TUI_INPUT_MAX		1024
#define TUI_DISPLAY_MAX		2000
#define TUI_PROMPT_MAX		64


void tui_set_prompt(const int user_id);

void tui_enter(void);
void tui_exit(void);

void tui_draw(void);

void tui_puts(const char *fmt, ...);

#define tui_warn(fmt, ...)	\
	tui_puts(ANSI_BOLD "(portal) " ANSI_ORANGE fmt ANSI_RESET __VA_OPT__(,)__VA_ARGS__)
#define tui_info(fmt, ...)	\
	tui_puts(ANSI_BOLD "(portal) " fmt ANSI_RESET __VA_OPT__(,)__VA_ARGS__)

char *tui_repl(size_t *len);

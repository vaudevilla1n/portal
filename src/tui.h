#pragma once

#include <stddef.h>
#include <termios.h>

#define	TUI_HEIGHT	(tui_internal_context.rows)	
#define	TUI_WIDTH	(tui_internal_context.cols)

#define	TUI_VIEW_START	(2)	
#define	TUI_VIEW_END	(TUI_HEIGHT - 1)


#define TUI_INPUT_MAX		4096
#define TUI_DISPLAY_MAX		2000


typedef struct {
	char *lines[TUI_DISPLAY_MAX];
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

extern tui_context_t tui_internal_context;


void tui_enter(void);
void tui_exit(void);

void tui_draw(void);
const char *tui_prompt(void);
void tui_puts(const char *text);

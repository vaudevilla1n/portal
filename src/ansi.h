#pragma once

#define ANSI_ESC	'\033'
#define ANSI_BS		'\010'
#define ANSI_DEL	'\x7f'
#define ANSI_ENTER	'\r'
#define ANSI_PGDN	"[6~"
#define ANSI_PGUP	"[5~"
#define ANSI_HOME	"[H"
#define ANSI_END	"[F"

#define ANSI_BOLD	"\033[1m"

#define ANSI_RED	"\033[38;2;255;0;0m"
#define ANSI_GREEN	"\033[38;2;0;153;0m"
#define ANSI_ORANGE	"\033[38;2;255;128;0m"
#define ANSI_BLUE	"\033[38;2;21;129;192m"

#define ANSI_RESET	"\033[0m"


void ansi_move(const int x, const int y);
void ansi_move_left(const int x);

void ansi_clear(void);
void ansi_clear_line(void);

void ansi_cursor_visible(const bool visible);

void ansi_scroll_up(void);
void ansi_scroll_down(void);

void ansi_reset_video(void);

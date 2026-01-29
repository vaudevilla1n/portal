#include "ansi.h"

#include <stdio.h>

void ansi_move(const int x, const int y) {
	printf("\033[%d;%dH", y, x);
}

void ansi_move_left(const int x) {
	printf("\033[%dD", x);
}

void ansi_clear(void) {
	printf("\033[0;0H\033[2J");
}

void ansi_clear_line(void) {
	printf("\033[K");
}

void ansi_cursor_visible(const bool visible) {
	printf("\033[?25%c", visible ? 'h' : 'l');
}

void ansi_scroll_up(void) {
	printf("\033M");
}

void ansi_scroll_down(void) {
	printf("\033D");
}

void ansi_reset_video(void) {
	printf(ANSI_RESET);
}

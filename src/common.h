#pragma once

#include "ansi.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define errno_string	strerror(errno)

#define info(fmt, ...)	\
	do { printf(ANSI_BOLD ANSI_GREEN fmt ANSI_RESET __VA_OPT__(,)__VA_ARGS__ ); fputc('\n', stdout); } while (0)
#define warn(fmt, ...)	\
	do { fprintf(stderr, ANSI_BOLD ANSI_YELLOW fmt ANSI_RESET __VA_OPT__(,)__VA_ARGS__); fputc('\n', stderr); } while (0)
#define die(fmt, ...)	\
	do { fprintf(stderr, ANSI_BOLD ANSI_RED fmt ANSI_RESET __VA_OPT__(,)__VA_ARGS__); fputc('\n', stderr); exit(1); } while (0)
#define die_if(cond, fmt, ...)	\
	do { if (cond) { die(fmt __VA_OPT__(,)__VA_ARGS__); } } while (0)

#define __unreachable(msg)	\
	do { fprintf(stderr, "unreachable: %s\n", msg); abort(); } while (0)
#define todo(func)	\
	do { fprintf(stderr, "todo: %s\n", func); abort(); } while (0)
#define unused(x)	(void)(x)


#define STREQ(s, t)	(!strcmp((s), (t)))
#define lengthof(s)	(sizeof(s) - 1)

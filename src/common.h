#pragma once

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOLD	"\033[1m"
#define RED	"\033[38;2;255;0;0m"
#define GREEN	"\033[38;2;0;153;0m"
#define YELLOW	"\033[38;2;255;128;0m"
#define RESET	"\033[0m"

#define errno_string	strerror(errno)

#define info(fmt, ...)	\
	do { printf(BOLD GREEN fmt RESET __VA_OPT__(,)__VA_ARGS__ ); fputc('\n', stdout); } while (0)
#define warn(fmt, ...)	\
	do { fprintf(stderr, BOLD YELLOW fmt RESET __VA_OPT__(,)__VA_ARGS__); fputc('\n', stderr); } while (0)
#define die(fmt, ...)	\
	do { fprintf(stderr, BOLD RED fmt RESET __VA_OPT__(,)__VA_ARGS__); fputc('\n', stderr); exit(1); } while (0)

#define unreachable(msg)	\
	do { fprintf(stderr, "unreachable: %s\n", msg); abort(); } while (0)
#define todo(func)	\
	do { fprintf(stderr, "todo: %s\n", func); abort(); } while (0)
#define unused(x)	(void)(x)

#define STREQ(s, t)	(!strcmp((s), (t)))

#define ERROR_MAX	4096

typedef char *error_t;

error_t error_new(const char *fmt, ...);
error_t error_from_errno(const char *msg);

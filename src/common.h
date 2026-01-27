#pragma once

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

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

#define assert_ptr(p, func)	\
	((p) ? (p) : (perror(func), abort(), nullptr))

#define xmalloc(n, size)		assert_ptr(malloc((size)), "malloc")
#define xcalloc(n, size)		assert_ptr(calloc((n), (size)), "calloc")
#define xrealloc(p, size)		assert_ptr(realloc((p), (size)), "realloc")
#define xreallocarray(p, n, size)	assert_ptr(reallocarray((p), (n), (size)), "reallocarray")

#define STREQ(s, t)	(!strcmp((s), (t)))

#define lengthof(s)	(sizeof(s) - 1)
#define countof(s)	(sizeof(s) / sizeof((s)[0]))

#define	MIN(x, y)	((x) < (y) ? (x) : (y))
#define	MAX(x, y)	((x) > (y) ? (x) : (y))

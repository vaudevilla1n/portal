#include "common.h"
#include <stdarg.h>

error_t error_new(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	static char err[ERROR_MAX];
	vsnprintf(err, ERROR_MAX, fmt, args);

	va_end(args);

	return err;
}

error_t error_from_errno(const char *msg) {
	return error_new("%s: %s", msg, errno_string);
}

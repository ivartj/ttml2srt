#include "compat.h"
#include <stdio.h>

int snprintf(char *buf, size_t buflen, const char *fmt, ...)
{
	va_list ap;
	int n;

	va_start(ap, fmt);
	n = vsnprintf(buf, buflen, fmt, ap);
	va_end(ap);

	return n;
}

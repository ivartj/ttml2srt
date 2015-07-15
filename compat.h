#ifndef COMPAT_H
#define COMPAT_H

#include <time.h>
#include <stdarg.h>

#ifndef HAVE_STRCASECMP
int strcasecmp(const char *s1, const char *s2);
#endif

#ifndef HAVE_STRPTIME
const char *strptime(const char *str, const char *fmt, struct tm *time);
#endif

#ifndef HAVE_SNPRINTF
int snprintf(char *buf, size_t buflen, const char *fmt, ...);
#endif

#ifndef HAVE_VSNPRINTF_C99_RETURN
#ifdef vsnprintf
#undef vsnprintf
#endif
#define vsnprintf vsnprintf_compat
int vsnprintf(char *buf, size_t buflen, const char *fmt, va_list ap);
#endif

#endif

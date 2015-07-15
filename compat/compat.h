#ifndef COMPAT_H
#define COMPAT_H

#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>

#ifndef HAVE_SSIZE_T
typedef long long int ssize_t;
#endif

#ifndef HAVE_SNPRINTF
int snprintf(char *buf, size_t buflen, const char *fmt, ...);
#endif

#ifndef HAVE_VSNPRINTF_C99_RETURN
# ifdef vsnprintf
#  undef vsnprintf
# endif
# define vsnprintf vsnprintf_compat
int vsnprintf(char *buf, size_t buflen, const char *fmt, va_list ap);
#endif

#ifndef HAVE_STRCASECMP
int strcasecmp(const char *s1, const char *s2);
#endif


#ifndef HAVE_STRPTIME
const char *strptime(const char *str, const char *fmt, struct tm *tm);
#endif

#ifndef HAVE_GETLINE
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
#endif

#endif

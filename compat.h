#ifndef COMPAT_H
#define COMPAT_H

#include "config.h"
#include <time.h>

#if HAVE_DECL_STRCASECMP == 0
int strcasecmp(const char *s1, const char *s2);
#endif

#if HAVE_DECL_STRPTIME == 0
const char *strptime(const char *str, const char *fmt, struct tm *time);
#endif

#endif

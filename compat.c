#include "compat.h"
#include <ctype.h>

#ifndef HAVE_STRCASECMP

int strcasecmp(const char *s1, const char *s2)
{
	int i;
	int c1, c2;

	for(i = 0; ; i++) {
		c1 = tolower(s1[i]);
		c2 = tolower(s2[i]);

		if(c1 != c2)
			return c1 - c2;

		if(c1 == '\0')
			break;
	}

	return 0;
}

#endif /* HAVE_STRCASECMP */

#ifndef HAVE_STRPTIME

static const char *p60(const char *str, int *num);

const char *p60(const char *str, int *num)
{
	if(!(str[0] >= '0' && str[0] <= '5'))
		return NULL;
	if(!(str[1] >= '0' && str[1] <= '9'))
		return NULL;

	*num = (str[1] - '0') + (str[0] - '0') * 10;

	return str + 2;
}

const char *strptime(const char *str, const char *fmt, struct tm *time)
{
	int i;

	while(*fmt != '\0') {
		if(isspace(*fmt) || (isgraph(*fmt) && *fmt != '%')) {
			if(*fmt == *str) {
				fmt++; str++;
				continue;
			} else
				return NULL;
		}

		if(*fmt != '%')
			return NULL;

		fmt++;

		switch(*fmt) {
		case '%':
			if(*str == '%') {
				fmt++; str++;
				continue;
			}
			return NULL;
		case 'H':
			str = p60(str, &(time->tm_hour));
			if(str == NULL)
				return NULL;
			fmt++;
			break;
		case 'M':
			str = p60(str, &(time->tm_min));
			if(str == NULL)
				return NULL;
			fmt++;
			break;
		case 'S':
			str = p60(str, &(time->tm_sec));
			if(str == NULL)
				return NULL;
			fmt++;
			break;
		default:
			return NULL;
		}

	}

	return str;
}

#endif /* HAVE_STRPTIME */


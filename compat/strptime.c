#include "compat.h"
#include <time.h>
#include <ctype.h>

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

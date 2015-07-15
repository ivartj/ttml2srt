#include "compat.h"
#include <ctype.h>

int strcasecmp(const char *s1, const char *s2)
{
	int c1, c2;

	do {
		c1 = tolower(*s1); s1++;
		c2 = tolower(*s2); s2++;
	} while(c1 && c2 && c1 == c2);

	return c1 - c2;
}

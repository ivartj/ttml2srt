AC_INIT([ttml2srt], [0.1], [ivar.jarlsby@gmail.com])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC
AM_PROG_CC_C_O

AC_CHECK_DECLS([strptime, strcasecmp], [], [], [
[#include <strings.h>
#include <time.h>]
])
AC_CHECK_FUNCS([strptime, strcasecmp])

AC_SUBST([EXPAT_CFLAGS])
AC_SUBST([EXPAT_LIBS])

if pkg-config expat; then
	EXPAT_CFLAGS=`pkg-config --cflags expat`
	EXPAT_LIBS=`pkg-config --libs expat`
else
	AC_MSG_ERROR([Could not find expat XML parsing library])
fi

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

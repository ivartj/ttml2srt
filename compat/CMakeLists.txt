cmake_minimum_required(VERSION 2.8.11)

project(compat C)

include(CheckSymbolExists)
include(CheckIncludeFile)
include(CheckCSourceRuns)
include(CheckTypeSize)

set(COMPAT_SRC compat.c compat.h)
set(COMPAT_DEF "")


check_type_size(ssize_t SSIZE_T)
if(HAVE_SSIZE_T)
	set(COMPAT_DEF ${COMPAT_DEF} -DHAVE_SSIZE_T)
endif()


check_symbol_exists(snprintf stdio.h HAVE_SNPRINTF)
if(HAVE_SNPRINTF)
	set(COMPAT_DEF ${COMPAT_DEF} -DHAVE_SNPRINTF)
else()
	set(COMPAT_SRC ${COMPAT_SRC} snprintf.c)
endif()


check_symbol_exists(strcasecmp strings.h HAVE_STRCASECMP)
if(HAVE_STRCASECMP)
	set(COMPAT_DEF ${COMPAT_DEF} -DHAVE_STRCASECMP)
else()
	set(COMPAT_SRC ${COMPAT_SRC} strcasecmp.c)
endif()


check_symbol_exists(strptime time.h HAVE_STRPTIME)
if(HAVE_STRPTIME)
	set(COMPAT_DEF ${COMPAT_DEF} -DHAVE_STRPTIME)
else()
	set(COMPAT_SRC ${COMPAT_SRC} strptime.c)
endif()


check_symbol_exists(getline stdio.h HAVE_GETLINE)
if(HAVE_GETLINE)
	set(COMPAT_DEF ${COMPAT_DEF} -DHAVE_GETLINE)
else()
	set(COMPAT_SRC ${COMPAT_SRC} getline.c)
endif()


check_c_source_runs(
	"#include <stdio.h>
	#include <assert.h>
	#include <stdarg.h>

	int test(void *null, ...)
	{
		va_list ap;
		int n;
		char buf[] = \"AAAAAAAAAAAAA\";

		va_start(ap, null);
		n = vsnprintf(buf, 2, \"test\", ap);
		if(n != 4)
			return 1;
		va_end(ap);
		return 0;
	}
	
	int main(int argc, char *argv[])
	{
		return test(NULL);
	}
	"
	HAVE_VSNPRINTF_C99_RETURN
)

if(HAVE_VSNPRINTF_C99_RETURN)
	set(COMPAT_DEF ${COMPAT_DEF} -DHAVE_VSNPRINTF_C99_RETURN)
else()
	set(COMPAT_SRC ${COMPAT_SRC} vsnprintf.c)
endif()


add_library(compat STATIC ${COMPAT_SRC})
target_compile_definitions(compat PUBLIC ${COMPAT_DEF})

set(COMPAT_DEF ${COMPAT_DEF} PARENT_SCOPE)

#ifndef ARGS_H
#define ARGS_H

#include <stdio.h>

/* Declare an array of args_option, terminated by a zeroed args_option. This is
 * passed to args_parse. */

typedef struct args_option args_option;
struct args_option {
	int code;          /* Value returned by args_parse upon encountering
			    * option. Also short option if it is a alphanumeric
			    * ASCII character index. */

	char *longopt;     /* Long-option string for --option and
	                    * --option=param form options. */

	char *optarg_name; /* Specifies that the option has a parameter and the
			    * name of the parameter for use in help message. */

	char *help;        /* Help string for option. */
};


/* An initially zeroed args_context is passed to args_parse. Should not be
 * directly manipulated or accessed by user of the code.*/

typedef struct args_context args_context;
struct args_context {
	unsigned idx; /* current argument being parsed */

	unsigned off; /* offset when traversing a collection of short options
                       * (such as -abc) */

	unsigned no_more_options;
	char short_arg[3];
	args_option *opts;
};


/* Sets options to recognize. This is used initially and may also be used later
 * when you want to parse options for a sub-command. */

void args_set_options(args_context *ctx, args_option *opts);

/* Returns code specified in the encountered args_option or:
 *    -1 : Upon end of parsing.
 *   '_' : Upon encountering what does not appear to be an option (not starting
 *         with hyphen, "-", or after "--" option).
 *   '?' : Unrecognized option.
 *   ':' : Missing parameter to option.
 */

int args_parse(args_context *ctx, int argc, char **argv, char **optarg);


/* Prints formatted help for the given options. */

void args_print_option_usage(FILE *out, args_option *opts);


/* Retrieves what appears to be the command in the execution path. */
const char *args_get_command(const char *argv0);


/* Writes the given text with the given indentation and wraps it around 80 characters. */

void args_print_wrap_and_indent(FILE *out, unsigned indentation, unsigned initial_indentation, const char *text);

#endif

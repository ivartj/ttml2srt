#ifndef ARGS_H
#define ARGS_H

#include <stdio.h>

typedef struct args_option args_option;
typedef struct args_parse_state args_parse_state;

struct args_option {
	int code;
	char shortopt;
	char *longopt;
	char *arg_name;
	char *help;
};

struct args_parse_state {
	unsigned idx;
	unsigned off;
	char *arg;
	unsigned no_more_options;
	char short_arg[3];
};

int args_parse(args_parse_state *state, int argc, char **argv, args_option *opts);

const char *args_get_cmd(const char *argv0);
void args_usage(args_option *opts, FILE *out);

#endif

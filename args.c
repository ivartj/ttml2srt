#include "args.h"
#include "config.h"
#include <string.h>
#include <stdio.h>

args_option *getlongopt(args_option *opts, const char *name, const char **arg)
{
	int i;
	size_t namelen, optlen;

	namelen = strlen(name);

	for(i = 0; opts[i].code != 0; i++) {
		optlen = strlen(opts[i].longopt);
		if(optlen > namelen)
			continue;
		if(strncmp(name, opts[i].longopt, optlen) != 0)
			continue;
		if(namelen > optlen) {
			if(name[optlen] != '=')
				continue;
			*arg = name + optlen + 1;
		}
		return &(opts[i]);
	}
	return NULL;
}

args_option *getshortopt(args_option *opts, char c)
{
	int i;

	for(i = 0; opts[i].code != 0; i++) {
		if(opts[i].shortopt == c)
			return &(opts[i]);
	}
	return NULL;
}

int args_parse(args_parse_state *state, int argc, char **argv, args_option *opts)
{
	char *arg;
	const char *optarg;
	args_option *opt;
	int retval;

	if(state->idx == 0)
		state->idx++;

	if(state->idx >= argc)
		return -1;

	state->arg = arg = argv[state->idx];

	if(state->no_more_options || strlen(arg) <= 1 || arg[0] != '-') {
		state->idx++;
		return '_';
	}

	if(strcmp(arg, "--") == 0) {
		state->idx++;
		state->no_more_options = 1;
		return args_parse(state, argc, argv, opts);
	}

	if(strncmp(arg, "--", 2) == 0) {
		state->idx++;
		optarg = NULL;
		opt = getlongopt(opts, arg + 2, &optarg);
		if(opt == NULL)
			return '?';

		if(opt->arg_name == NULL)
			return opt->code;
		else {
			if(optarg != NULL)
				state->arg = (char *)optarg;
			else {
				if(state->idx >= argc)
					return ':';
				state->arg = argv[state->idx];
				state->idx++;
			}
			return opt->code;
		}
	}

	state->off++;
	snprintf(state->short_arg, 3, "-%c", arg[state->off]);
	state->arg = state->short_arg;
	opt = getshortopt(opts, arg[state->off]);
	if(opt == NULL) {
		retval = '?';
		goto ret;
	}

	if(opt->arg_name == NULL) {
		retval = opt->code;
		goto ret;
	}

	if(state->idx + 1 == argc || state->off + 1 != strlen(arg)) {
		retval = ':';
		goto ret;
	}

	state->idx++;
	arg = argv[state->idx];
	retval = opt->code;

ret:
	if(state->off + 1 == strlen(arg)) {
		state->off = 0;
		state->idx++;
	}
	return retval;
}

void args_usage(args_option *opts, FILE *out)
{
	unsigned maxoff = 0;
	unsigned off = 0;
	args_option *opt;
	int i, j;

	/* First find a common offset long enough for every option.
	 *
	 * Cases:
	 *   -h  |
	 *   -i FILENAME  |
	 *   -h, --help  |
	 *   --version  |
	 *   --format=FORMAT  |
	 *   -o, --output=FILENAME  |
	 *   
	 */
	for(i = 0; opts[i].code != 0; i++) {
		opt = &(opts[i]);

		switch(opt->shortopt) {
		case '-':
			switch(opt->longopt != NULL) {
			case 0:
				/* uninteresting case */
				break;
			default:
				switch(opt->arg_name != NULL) {
				case 0:
					off = snprintf(NULL, 0, "  --%s  ", opt->longopt);
					break;
				default:
					off = snprintf(NULL, 0, "  --%s=%s  ", opt->longopt, opt->arg_name);
					break;
				}
				break;
			}
			break;
		default:
			switch(opt->longopt != NULL) {
			case 0:
				switch(opt->arg_name != NULL) {
				case 0:
					off = snprintf(NULL, 0, "  -%c  ", opt->shortopt);
					break;
				default:
					off = snprintf(NULL, 0, "  -%c %s  ", opt->shortopt, opt->arg_name);
					break;
				}
				break;
			default:
				switch(opt->arg_name != NULL) {
				case 0:
					off = snprintf(NULL, 0, "  -%c, --%s  ", opt->shortopt, opt->longopt);
					break;
				default:
					off = snprintf(NULL, 0, "  -%c, --%s=%s  ", opt->shortopt, opt->longopt, opt->arg_name);
					break;
				}
				break;
			}
			break;
		}

		if(off > maxoff)
			maxoff = off;
	}

	for(i = 0; opts[i].code != 0; i++) {
		opt = &(opts[i]);

		switch(opt->shortopt) {
		case '-':
			switch(opt->longopt != NULL) {
			case 0:
				/* uninteresting case */
				break;
			default:
				switch(opt->arg_name != NULL) {
				case 0:
					off = fprintf(out, "  --%s  ", opt->longopt);
					break;
				default:
					off = fprintf(out, "  --%s=%s  ", opt->longopt, opt->arg_name);
					break;
				}
				break;
			}
			break;
		default:
			switch(opt->longopt != NULL) {
			case 0:
				switch(opt->arg_name != NULL) {
				case 0:
					off = fprintf(out, "  -%c  ", opt->shortopt);
					break;
				default:
					off = fprintf(out, "  -%c %s  ", opt->shortopt, opt->arg_name);
					break;
				}
				break;
			default:
				switch(opt->arg_name != NULL) {
				case 0:
					off = fprintf(out, "  -%c, --%s  ", opt->shortopt, opt->longopt);
					break;
				default:
					off = fprintf(out, "  -%c, --%s=%s  ", opt->shortopt, opt->longopt, opt->arg_name);
					break;
				}
				break;
			}
			break;
		}

		for(j = off; j < maxoff; j++)
			fprintf(out, " ");
		fprintf(out, "%s\n", opt->help);
	}
}

const char *args_get_cmd(const char *argv0)
{
	const char *sepcheck = NULL;

	if(argv0 == NULL)
		return PACKAGE_NAME;

	for(sepcheck = argv0; *sepcheck != '\0'; sepcheck++)
		if(*sepcheck == '/')
			argv0 = sepcheck + 1;

	return argv0;
}

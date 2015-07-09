#include "args.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#define WRAP 80

static args_option *getlongopt(args_option *opts, char *name, char **arg);
static args_option *getshortopt(args_option *opts, char c);
static const char *nextword(const char *(*text));
static int xprintf(FILE *, const char *fmt, ...);
static int print_option(FILE *, args_option *opt);
static int parselongopt(args_context *ctx, int argc, char **argv, char **optarg);
static int parseshortopt(args_context *ctx, int argc, char **argv, char **optarg);

args_option *getlongopt(args_option *opts, char *arg, char **optarg)
{
	int i;
	size_t arglen, optlen;

	if(strncmp(arg, "--", 2) != 0)
		return NULL;

	arg += 2;

	arglen = strlen(arg);

	for(i = 0; opts[i].code != 0; i++) {
		optlen = strlen(opts[i].longopt);
		if(optlen > arglen)
			continue;
		if(strncmp(arg, opts[i].longopt, optlen) != 0)
			continue;
		if(arglen > optlen) {
			if(arg[optlen] != '=')
				continue;
			*optarg = arg + optlen + 1;
		}
		return &(opts[i]);
	}
	return NULL;
}

// TODO: Enable function to handle someething like -o=optarg somehow
args_option *getshortopt(args_option *opts, char c)
{
	int i;

	for(i = 0; opts[i].code != 0; i++) {
		if(isalnum(opts[i].code))
		if(opts[i].code == c)
			return &(opts[i]);
	}
	return NULL;
}

void args_set_options(args_context *ctx, args_option *opts)
{
	ctx->opts = opts;
}

int args_parse(args_context *ctx, int argc, char **argv, char **optarg)
{
	char *arg;
	args_option *opt;
	char *optarg_new = NULL;
	int retval;
	args_option *opts;

	if(ctx->idx == 0)
		ctx->idx++;

	if(ctx->idx >= argc)
		return -1;

	*optarg = arg = argv[ctx->idx];

	/* Plain option */
	if(ctx->no_more_options || strlen(arg) <= 1 || arg[0] != '-') {
		ctx->idx++;
		return '_';
	}

	/* "--" argument, signifying end of any further options */
	if(strcmp(arg, "--") == 0) {
		ctx->idx++;
		ctx->no_more_options = 1;
		return args_parse(ctx, argc, argv, optarg);
	}

	/* Long options */
	if(strncmp(arg, "--", 2) == 0)
		return parselongopt(ctx, argc, argv, optarg);

	/* Short options */
	return parseshortopt(ctx, argc, argv, optarg);
}

int parselongopt(args_context *ctx, int argc, char **argv, char **optarg)
{
	args_option *opt;
	char *optarg_new = NULL;
	char *arg;

	arg = argv[ctx->idx];

	ctx->idx++;

	// TODO: Handle option argument to option that should not have argument.
	opt = getlongopt(ctx->opts, arg, &optarg_new);
	if(opt == NULL)
		return '?';

	if(opt->optarg_name == NULL)
		return opt->code;

	if(optarg_new != NULL)
		*optarg = optarg_new;
	else {
		if(ctx->idx >= argc)
			return ':';
		*optarg = argv[ctx->idx];
		ctx->idx++;
	}

	return opt->code;
}

int parseshortopt(args_context *ctx, int argc, char **argv, char **optarg)
{
	/* This code is complicated by its ability handle collections of short
	 * options in a single argument, such -abc, containing -a, -b, and -c.
	 * The parser will return once for each of the short options within
	 * such an argument. The current short option being processed within
	 * the argument is given by the ctx->off (offset) value.
	 */
	int retval;
	char *arg;
	args_option *opt;

	arg = argv[ctx->idx];

	/* Get to the next short option in the argument */
	ctx->off++;
	snprintf(ctx->short_arg, 3, "-%c", arg[ctx->off]);
	*optarg = ctx->short_arg;

	opt = getshortopt(ctx->opts, arg[ctx->off]);
	if(opt == NULL) {
		retval = '?';
		goto ret;
	}

	if(opt->optarg_name == NULL) {
		retval = opt->code;
		goto ret;
	}

	/* check if there are no more arguments or if there is another short
 	 * option in the current argument (such as 'c' after 'b' in -abc) */
	if(ctx->idx + 1 == argc || ctx->off + 1 != strlen(arg)) {
		retval = ':';
		goto ret;
	}

	ctx->idx++;
	*optarg = argv[ctx->idx];
	retval = opt->code;

ret:
	/* only go to the next argument if there are no more short options in
         * this one */
	if(ctx->off + 1 == strlen(arg)) {
		ctx->off = 0;
		ctx->idx++;
	}
	return retval;
}

int xprintf(FILE *file, const char *fmt, ...)
{
	va_list ap;
	int val;

	va_start(ap, fmt);
	if(file == NULL)
		val = vsnprintf(NULL, 0, fmt, ap);
	else
		val = vfprintf(file, fmt, ap);
	va_end(ap);

	return val;

}

int print_option(FILE *out, args_option *opt)
{
	/* Cases:
	 *   -h  |
	 *   -i FILENAME  |
	 *   -h, --help  |
	 *   --version  |
	 *   --format=FORMAT  |
	 *   -o, --output=FILENAME  |
	 */
	unsigned off = 0;

	 /* if it has a short option */
	if(isalnum(opt->code)) {
		switch(opt->longopt != NULL) {
		case 0:
			switch(opt->optarg_name != NULL) {
			case 0:
				off = xprintf(out, "  -%c  ", opt->code);
				break;
			default:
				off = xprintf(out, "  -%c %s  ", opt->code, opt->optarg_name);
				break;
			}
			break;
		default:
			switch(opt->optarg_name != NULL) {
			case 0:
				off = xprintf(out, "  -%c, --%s  ", opt->code, opt->longopt);
				break;
			default:
				off = xprintf(out, "  -%c, --%s=%s  ", opt->code, opt->longopt, opt->optarg_name);
				break;
			}
			break;
		}
	} else {
		switch(opt->longopt != NULL) {
		case 0:
			/* uninteresting case */
			break;
		default:
			switch(opt->optarg_name != NULL) {
			case 0:
				off = xprintf(out, "  --%s  ", opt->longopt);
				break;
			default:
				off = xprintf(out, "  --%s=%s  ", opt->longopt, opt->optarg_name);
				break;
			}
			break;
		}
	}

	return off;
}

void args_print_option_usage(FILE *out, args_option *opts)
{
	unsigned maxoff = 0;
	unsigned off = 0;
	args_option *opt;
	int i, j;

	/* First find a common offset long enough for every option.
	 */
	for(i = 0; opts[i].code != 0; i++) {
		opt = &(opts[i]);

		off = print_option(NULL, opt);

		if(off > maxoff)
			maxoff = off;
	}


	/* Write options with option help at given offset. */

	for(i = 0; opts[i].code != 0; i++) {
		opt = &(opts[i]);

		off = print_option(out, opt);

		args_print_wrap_and_indent(out, maxoff, off, opt->help);
	}
}

const char *args_get_command(const char *argv0)
{
	const char *sepcheck = NULL;

	for(sepcheck = argv0; *sepcheck != '\0'; sepcheck++)
		if(*sepcheck == '/')
			argv0 = sepcheck + 1;

	return argv0;
}

const char *nextword(const char *(*text))
{
	static char word[WRAP + 1] = { 0 };
	int i;

	while(**text && !isgraph(**text))
		(*text)++;

	if(**text == '\0')
		return NULL;

	for(i = 0; i < WRAP && isgraph(**text); i++) {
		word[i] = **text;
		(*text)++;
	}

	word[i] = '\0';


	return word;
}

void args_print_wrap_and_indent(FILE *out, unsigned ind, unsigned initind, const char *text)
{
	int i;
	int textoff = 0;
	int curoff;
	int maxoff = WRAP;
	const char *word;
	int wordlen;
	int newline;

	if(initind < ind) {
		for(i = 0; i < ind - initind; i++)
			fputc(' ', out);
		curoff = ind;
	} else
		curoff = initind;

	newline = 1;

	while((word = nextword(&text)) != NULL) {

		wordlen = strlen(word);
		if(curoff + wordlen > WRAP && !newline) {
			fputc('\n', out);
			for(i = 0; i < ind; i++)
				fputc(' ', out);
			curoff = ind;
			newline = 1;
		}

		if(!newline) {
			fputc(' ', out);
			curoff++;
		}

		curoff += fprintf(out, "%s", word);

		newline = 0;
	}

	fputc('\n', out);
}


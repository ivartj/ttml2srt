#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "args.h"
#include "ttml2srt.h"

static const char *main_name = NULL;
static const char *output_filename = NULL;
static const char *input_filename = NULL;
static FILE *input = NULL;
static FILE *output = NULL;

static void usage(FILE *out, args_option *opts);
static void parseargs(int argc, char *argv[]);
static void openfiles(void);

void usage(FILE *out, args_option *opts)
{
	fprintf(out, "Usage:\n");
	fprintf(out, "  %s [-o OUTPUT] [INPUT [OUTPUT]]\n", main_name);
	fprintf(out, "\n");
	fprintf(out, "Description:\n");
	fprintf(out, "  Convert TTML files to SRT files.\n");
	fprintf(out, "\n");
	fprintf(out, "Options:\n");
	args_print_option_usage(out, opts);
	fprintf(out, "\n");
}

void parseargs(int argc, char *argv[])
{
	args_context ctx = { 0 };
	static args_option opts[] = {
		{ 'h', "help", NULL, "Prints help message" },
		{ 301, "version", NULL, "Prints version" },
		{ 'o', "output", "FILENAME", "Specifies output file" },
		{ 0 },
	};
	int c;
	char *optarg = NULL;
	struct {
		int output;
		int input;
	} set = { 0 };

	args_set_options(&ctx, opts);

	while((c = args_parse(&ctx, argc, argv, &optarg)) != -1)
	switch(c) {
	case 'h':
		usage(stdout, opts);
		exit(EXIT_SUCCESS);
	case 301:
		printf("%s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
		exit(EXIT_SUCCESS);
	case 'o':
		if(set.output) {
			fprintf(stderr, "Output specified twice.\n");
			exit(EXIT_FAILURE);
		}
		output_filename = optarg;
		set.output = 1;
		break;
	case '_':
		if(set.input && !(set.output)) {
			output_filename = optarg;
			set.output = 1;
			break;
		} else if(set.output) {
			fprintf(stderr, "Output specified twice.\n");
			break;
		}
		input_filename = optarg;
		set.input = 1;
		break;
	case ':':
	case '?':
		usage(stderr, opts);
		exit(EXIT_FAILURE);
	}
}

void openfiles(void)
{
	if(input_filename == NULL)
		input = stdin;
	else {
		input = fopen(input_filename, "rb");
		if(input == NULL) {
			fprintf(stderr, "Failed to open '%s' for reading: %s.\n", input_filename, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	if(output_filename == NULL)
		output = stdout;
	else {
		output = fopen(output_filename, "w");
		if(output == NULL) {
			fprintf(stderr, "Failed to open '%s' for writing: %s.\n", output_filename, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char *argv[])
{
	ttml2srt_context *ctx;
	int err;
	int retval = 1;

	main_name = args_get_command(argv[0]);
	parseargs(argc, argv);
	openfiles();

	ctx = ttml2srt_create_context();
	if(ctx == NULL) {
		fprintf(stderr, "ttml2srt: Failed allocate context.\n");
		goto cleanup_files;
	}

	ttml2srt_set_input_file(ctx, input);
	ttml2srt_set_output_file(ctx, output);
	err = ttml2srt_process(ctx);
	if(err) {
		fprintf(stderr, "ttml2srt:\n%s.\n", ttml2srt_get_error(ctx));
		goto cleanup_context;
	}

	retval = 0; /* success */

cleanup_context:
	ttml2srt_destroy_context(ctx);

cleanup_files:
	fclose(input);
	fclose(output);

	exit(retval);
}

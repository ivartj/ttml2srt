#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "config.h"
#include "args.h"
#include "process.h"

const char *main_name = NULL;
const char *output_filename = NULL;
const char *input_filename = NULL;
FILE *input = NULL;
FILE *output = NULL;

void usage(FILE *out, args_option *opts)
{
	fprintf(out, "Usage: %s [-o OUTPUT] [INPUT]\n", main_name);
	fprintf(out, "Convert TTML files to SRT files.\n");
	fprintf(out, "\n");
	fprintf(out, "Options:\n");
	args_usage(opts, out);
}

void parseargs(int argc, char *argv[])
{
	args_parse_state st = { 0 };
	static args_option opts[] = {
		{ 'h', 'h', "help", NULL, "Prints help message" },
		{ 301, '-', "version", NULL, "Prints version" },
		{ 'o', 'o', "output", "FILENAME", "Specifies output file" },
		{ 0 },
	};
	int c;

	while((c = args_parse(&st, argc, argv, opts)) != -1)
	switch(c) {
	case 'h':
		usage(stdout, opts);
		exit(EXIT_SUCCESS);
	case 301:
		printf("%s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
		exit(EXIT_SUCCESS);
	case 'o':
		output_filename = st.arg;
		break;
	case '_':
		input_filename = st.arg;
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
	main_name = args_get_cmd(argv[0]);
	parseargs(argc, argv);
	openfiles();
	process(input, output);
	exit(EXIT_SUCCESS);
}

#include <expat.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <strings.h>
#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <time.h>
#include "ttml2srt.h"
#include "compat/compat.h"

/* TIMELEN = strlen("00:00:15,000") + 1 */
#define TIMELEN 13

#define TAG_UNRECOGNIZED	0
#define TAG_P			1
#define TAG_BR			2

#define INPUT_TYPE_FILE		1
#define INPUT_TYPE_CALLBACK	2

#define OUTPUT_TYPE_FILE	1
#define OUTPUT_TYPE_CALLBACK	2

typedef union input input;
typedef struct input_file input_file;
typedef struct input_callback input_callback;

typedef union output output;
typedef struct output_file output_file;
typedef struct output_callback output_callback;

struct input_file {
	int type;
	FILE *file;
};

struct input_callback {
	int type;
	ttml2srt_input_callback callback;
	void *handle;
};

union input {
	int type;
	input_file file;
	input_callback callback;
};

struct output_file {
	int type;
	FILE *file;
};

struct output_callback {
	int type;
	ttml2srt_output_callback callback;
	void *handle;
};

union output {
	int type;
	output_file file;
	output_callback callback;
};

struct ttml2srt_context {
	input in;
	output out;
	jmp_buf escape;
	char errmsg[1024];
	int sub_init;
	unsigned sub_num;
	char sub_begin[TIMELEN];
	char sub_end[TIMELEN];
	int sp;
};

static ssize_t xread(ttml2srt_context *ctx, void *buf, size_t buflen);
static ssize_t xwrite(ttml2srt_context *ctx, const void *buf, size_t buflen);
static void xputc(ttml2srt_context *ctx, int c);
static void xprintf(ttml2srt_context *ctx, const char *fmt, ...);
static void vxprintf(ttml2srt_context *ctx, const char *fmt, va_list ap);
static void xerror(ttml2srt_context *ctx, const char *fmt, ...);

ttml2srt_context *ttml2srt_create_context(void)
{
	ttml2srt_context *ctx;

	ctx = calloc(1, sizeof(ttml2srt_context));

	return ctx;
}

void ttml2srt_destroy_context(ttml2srt_context *ctx)
{
	free(ctx);
}

void ttml2srt_set_input_file(ttml2srt_context *ctx, FILE *file)
{
	memset(&(ctx->in), 0, sizeof(input));
	ctx->in.type = INPUT_TYPE_FILE;
	ctx->in.file.file = file;
}

void ttml2srt_set_input_callback(ttml2srt_context *ctx, ttml2srt_input_callback cb, void *handle)
{
	memset(&(ctx->in), 0, sizeof(input));
	ctx->in.type = INPUT_TYPE_CALLBACK;
	ctx->in.callback.callback = cb;
	ctx->in.callback.handle = handle;
}

void ttml2srt_set_output_file(ttml2srt_context *ctx, FILE *file)
{
	memset(&(ctx->out), 0, sizeof(output));
	ctx->out.type = INPUT_TYPE_FILE;
	ctx->out.file.file = file;
}

void ttml2srt_set_output_callback(ttml2srt_context *ctx, ttml2srt_output_callback cb, void *handle)
{
	memset(&(ctx->out), 0, sizeof(output));
	ctx->out.type = INPUT_TYPE_CALLBACK;
	ctx->out.callback.callback = cb;
	ctx->out.callback.handle = handle;
}

ssize_t xread(ttml2srt_context *ctx, void *buf, size_t buflen)
{
	ssize_t n;

	switch(ctx->in.type) {
	case INPUT_TYPE_FILE:
		n = fread(buf, 1, buflen, ctx->in.file.file);
		if(n == -1)
			xerror(ctx, "fread:\n%s", strerror(errno));
		return n;
	case INPUT_TYPE_CALLBACK:
		n = ctx->in.callback.callback(ctx->in.callback.handle, buf, buflen);
		if(n == -1)
			xerror(ctx, "input callback:\n%s", strerror(errno));
		return n;
	}

	xerror(ctx, "unrecognized input type");
}

ssize_t xwrite(ttml2srt_context *ctx, const void *buf, size_t buflen)
{
	ssize_t n;

	switch(ctx->out.type) {
	case OUTPUT_TYPE_FILE:
		n = fwrite(buf, 1, buflen, ctx->out.file.file);
		if(n == -1)
			xerror(ctx, "fwrite:\n%s", strerror(errno));
		return n;
	case OUTPUT_TYPE_CALLBACK:
		n = ctx->out.callback.callback(ctx->out.callback.handle, buf, buflen);
		if(n == -1)
			xerror(ctx, "output callback:\n%s", strerror(errno));
		return n;
	}

	xerror(ctx, "unrecognized output type");
}

void xputc(ttml2srt_context *ctx, int c)
{

	unsigned char byte;

	byte = c;

	xwrite(ctx, &byte, 1);
}

void vxprintf(ttml2srt_context *ctx, const char *fmt, va_list ap)
{
	// TODO: Do something about buffer overflow (currently just truncates)
	int n;
	char buf[1024];

	if(ctx->out.type == OUTPUT_TYPE_FILE) {
		n = vfprintf(ctx->out.file.file, fmt, ap);
		if(n < 0)
			xerror(ctx, "vfprintf:\n%s", strerror(errno));
		return;
	}

	n = vsnprintf(buf, sizeof(buf), fmt, ap);
	if(n < 0)
		xerror(ctx, "vfprintf:\n%s", strerror(errno));

	xwrite(ctx, buf, n);
}

void xprintf(ttml2srt_context *ctx, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vxprintf(ctx, fmt, ap);
	va_end(ap);
}

void xerror(ttml2srt_context *ctx, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(ctx->errmsg, sizeof(ctx->errmsg), fmt, ap);
	va_end(ap);
	longjmp(ctx->escape, -1);
}

const char *ttml2srt_get_error(ttml2srt_context *ctx)
{
	return ctx->errmsg;
}

int identify_tag(const char *name)
{
	if(!strcasecmp(name, "p"))
		return TAG_P;
	if(!strcasecmp(name, "br"))
		return TAG_BR;
	return 0;
}

const char *get_attr(const char **atts, const char *name)
{
	while(atts[0] != NULL) {
		if(!strcasecmp(name, atts[0]))
			return atts[1];
		atts = &(atts[2]);
	}
	return NULL;
}

void start_p_handler(ttml2srt_context *ctx, const char **atts)
{
	struct tm begintime = { 0 }, endtime = { 0 };
	const char *begin, *end, *dur, *rest;
	unsigned ms, ms_begin, len;

	if(ctx->sub_init)
		return;
	ctx->sub_init = 1;
	ctx->sub_num++;
	ctx->sp = 1;

	// TODO: Check that times and durations are in fact valid

	begin = get_attr(atts, "begin");
	if(begin != NULL) {
		rest = strptime(begin, "%H:%M:%S.", &begintime);
		sscanf(rest, "%u\n", &ms);
		ms_begin = ms;
		len = strftime(ctx->sub_begin, TIMELEN, "%H:%M:%S,", &begintime);
		snprintf(ctx->sub_begin + len, TIMELEN - len, "%03u", ms);
	}

	end = get_attr(atts, "end");
	if(end != NULL) {
		rest = strptime(end, "%H:%M:%S.", &endtime);
		sscanf(rest, "%u\n", &ms);
		len = strftime(ctx->sub_end, TIMELEN, "%H:%M:%S,", &endtime);
		snprintf(ctx->sub_end + len, TIMELEN - len, "%03u", ms);
	}

	dur = get_attr(atts, "dur");
	if(dur != NULL) {
		rest = strptime(dur, "%H:%M:%S.", &endtime);
		sscanf(rest, "%u\n", &ms);
		if(ms + ms_begin >= 1000)
			endtime.tm_sec++;
		ms = (ms + ms_begin) % 1000;
		endtime.tm_sec += begintime.tm_sec;
		endtime.tm_min += begintime.tm_min;
		endtime.tm_hour += begintime.tm_hour;
		mktime(&endtime);
		len = strftime(ctx->sub_end, TIMELEN, "%H:%M:%S,", &endtime);
		snprintf(ctx->sub_end + len, TIMELEN - len, "%03u", ms);
	}

	xprintf(ctx, "%u\n", ctx->sub_num);
	xprintf(ctx, "%s", ctx->sub_begin);
	if(strlen(ctx->sub_end) != 0)
		xprintf(ctx, " --> %s\n", ctx->sub_end);
	else
		xprintf(ctx, "\n");
}

void start_element_handler(void *udata, const char *name, const char **atts)
{
	ttml2srt_context *ctx;

	ctx = udata;
	switch(identify_tag(name)) {
	case TAG_P:
		start_p_handler(ctx, atts);
		break;
	case TAG_BR:
		xprintf(ctx, "\n");
		ctx->sp = 1;
		break;
	default:
		break;
	}
}

void end_element_handler(void *udata, const char *name)
{
	ttml2srt_context *ctx;

	ctx = udata;
	switch(identify_tag(name)) {
	case TAG_P:
		if(ctx->sub_init) {
			xprintf(ctx, "\n\n");
			ctx->sub_init = 0;
		}
		break;
	default:
		break;
	}
}

void text_handler(void *udata, const char *text, int len)
{
	ttml2srt_context *ctx;
	int i;
	char c;
	int sp;

	ctx = udata;

	if(!ctx->sub_init)
		return;


	for(i = 0; i < len; i++) {

		c = text[i];

		switch(c) {
		case '\n':
		case '\t':
		case '\r':
			continue;
		}

		sp = 0;

		switch(c) {
		case ' ':
			sp = 1;
			if(ctx->sp)
				break;
			else
				xputc(ctx, c);
			break;
		default:
			xputc(ctx, c);
		}
		ctx->sp = sp;
	}
}

int ttml2srt_process(ttml2srt_context *ctx)
{
	char buf[1024];
	size_t readlen;
	int isfinal;
	int ok;
	enum XML_Error errcode;
	XML_Parser expat; /* Really no asterisk */

	expat = XML_ParserCreate(NULL);
	XML_SetElementHandler(
		expat,
		(XML_StartElementHandler)start_element_handler,
		(XML_EndElementHandler)end_element_handler);
	XML_SetCharacterDataHandler(
		expat,
		(XML_CharacterDataHandler)text_handler
	);
	XML_SetUserData(expat, ctx);

	if(setjmp(ctx->escape))
		return -1;

	xprintf(ctx, "\xef\xbb\xbf"); /* Byte Order Mark */

	for(;;) {
		readlen = xread(ctx, buf, sizeof(buf));
		isfinal = readlen != sizeof(buf);
		ok = XML_Parse(expat, buf, readlen, isfinal);
		if(!ok) {
			errcode = XML_GetErrorCode(expat);
			xerror(ctx, "XML_Parse:\n%s", XML_ErrorString(errcode));
		}
		if(isfinal)
			break;
	}

	XML_ParserFree(expat);

	return 0;
}

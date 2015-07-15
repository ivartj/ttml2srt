#include "compat.h"
#include <assert.h>

#define FLAGS_ZERO_PADDING	1 << 0

enum state {
	st_text,
	st_eof,
	st_flags,
	st_width,
	st_prec,
	st_length,
	st_spec,
};

typedef struct context context;
struct context {
	char *buf;
	size_t len;
	int off;
	va_list ap;
	enum state state;
	unsigned flags;
	int width;
};

static const char *ptext(context *ctx, const char *fmt);
static const char *pflags(context *ctx, const char *fmt);
static const char *pwidth(context *ctx, const char *fmt);
static const char *pprec(context *ctx, const char *fmt);
static const char *plength(context *ctx, const char *fmt);
static const char *pspec(context *ctx, const char *fmt);
static void xputc(context *ctx, int c);
static void xputi(context *ctx, int num);
static void xputu(context *ctx, unsigned num);

void xputc(context *ctx, int c)
{
	/* + 1 because need space for null terminator */
	if(ctx->off + 1 >= ctx->len) {
		ctx->off++;
		return;
	}

	ctx->buf[ctx->off++] = c;
}

void xputi(context *ctx, int num)
{
	char buf[256];
	int i = 0;
	int j;
	int negative = 0;

	if(num < 0) {
		xputc(ctx, '-');
		num *= -1;
		negative = 1;
	}

	do {
		buf[i++] = (num % 10) + '0';
		num /= 10;
	} while(num);

	for(j = i + (negative ? 1 : 0); j < ctx->width; j++) {
		if(ctx->flags & FLAGS_ZERO_PADDING)
			xputc(ctx, '0');
		else
			xputc(ctx, ' ');
	}

	for(i--; i >= 0; i--)
		xputc(ctx, buf[i]);
}

void xputu(context *ctx, unsigned num)
{
	char buf[256];
	int i = 0;
	int j;

	do {
		buf[i++] = (num % 10) + '0';
		num /= 10;
	} while(num);

	for(j = i; j < ctx->width; j++) {
		if(ctx->flags & FLAGS_ZERO_PADDING)
			xputc(ctx, '0');
		else
			xputc(ctx, ' ');
	}

	for(i--; i >= 0; i--)
		xputc(ctx, buf[i]);
}

const char *ptext(context *ctx, const char *fmt)
{
	int c;

	for(;;) {
		c = *fmt; fmt++;

		switch(c) {
		case '\0':
			ctx->state = st_eof;
			return fmt;
		case '%':
			ctx->state = st_flags;
			return fmt;
		default:
			xputc(ctx, c);
			break;
		}
	}
}

const char *pflags(context *ctx, const char *fmt)
{
	int c;

	ctx->flags = 0;

	for(;;) {
		c = *fmt; fmt++;

		switch(c) {
		case '\0':
			ctx->state = st_eof;
			return fmt;
		case '0':
			ctx->flags |= FLAGS_ZERO_PADDING;
			break;
		default:
			ctx->state = st_width;
			return fmt - 1;
		}
	}
}

const char *pwidth(context *ctx, const char *fmt)
{
	int c;

	ctx->width = 0;

	for(;;) {
		c = *fmt; fmt++;

		switch(c) {
		case '\0':
			ctx->state = st_eof;
			return fmt;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			ctx->width *= 10;
			ctx->width += c - '0';
			break;
		default:
			ctx->state = st_prec;
			return fmt - 1;
		}
	}
}

const char *pprec(context *ctx, const char *fmt)
{
	int c;

	for(;;) {
		c = *fmt; fmt++;

		switch(c) {
		case '\0':
			ctx->state = st_eof;
			return fmt;
		default:
			ctx->state = st_length;
			return fmt - 1;
		}
	}

}

const char *plength(context *ctx, const char *fmt)
{
	int c;

	for(;;) {
		c = *fmt; fmt++;

		switch(c) {
		case '\0':
			ctx->state = st_eof;
			return fmt;
		default:
			ctx->state = st_spec;
			return fmt - 1;
		}
	}

}

const char *pspec(context *ctx, const char *fmt)
{
	int c;

	for(;;) {
		c = *fmt; fmt++;

		switch(c) {
		case '\0':
			ctx->state = st_eof;
			return fmt;
		case '%':
			xputc(ctx, '%');
			ctx->state = st_text;
			return fmt;
		case 'd':
		case 'i':
			xputi(ctx, va_arg(ctx->ap, int));
			ctx->state = st_text;
			return fmt;
		case 'u':
			xputu(ctx, va_arg(ctx->ap, unsigned));
			ctx->state = st_text;
			return fmt;
		}
	}
}

int vsnprintf(char *buf, size_t buflen, const char *fmt, va_list ap)
{
	context ctx = { 0 };

	ctx.buf = buf;
	ctx.len = buflen;
	ctx.state = st_text;
	va_copy(ctx.ap, ap);

	while(*fmt != '\0') {
		switch(ctx.state) {
		case st_text:
			fmt = ptext(&ctx, fmt);
			break;
		case st_flags:
			fmt = pflags(&ctx, fmt);
			break;
		case st_width:
			fmt = pwidth(&ctx, fmt);
			break;
		case st_prec:
			fmt = pprec(&ctx, fmt);
			break;
		case st_length:
			fmt = plength(&ctx, fmt);
			break;
		case st_spec:
			fmt = pspec(&ctx, fmt);
			break;
		case st_eof:
			goto ret;
		}
	}

ret:
	if(buflen != 0) {
		if(buflen - 1 < ctx.off)
			ctx.buf[buflen - 1] = '\0';
		else
			ctx.buf[ctx.off] = '\0';
	}

	va_end(ctx.ap);
	va_end(ap);

	return ctx.off;
}


#ifndef TTML2SRT_H
#define TTML2SRT_H

#include <stdio.h>
#include <sys/types.h>

typedef ssize_t (*ttml2srt_input_callback)(void *handle, void *buffer, size_t buffer_size);;
typedef ssize_t (*ttml2srt_output_callback)(void *handle, const void *buffer, size_t buffer_size);;

typedef struct ttml2srt_context ttml2srt_context;

ttml2srt_context *ttml2srt_create_context(void);

void ttml2srt_set_input_file(ttml2srt_context *ctx, FILE *file);
void ttml2srt_set_input_callback(ttml2srt_context *ctx, ttml2srt_input_callback cb, void *handle);

void ttml2srt_set_output_file(ttml2srt_context *ctx, FILE *file);
void ttml2srt_set_output_callback(ttml2srt_context *ctx, ttml2srt_output_callback cb, void *handle);

int ttml2srt_process(ttml2srt_context *ctx);
const char *ttml2srt_get_error(ttml2srt_context *ctx);

#endif

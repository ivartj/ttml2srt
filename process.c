#include "process.h"
#include <expat.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#define BUFLEN 1024

/* TIMELEN = strlen("00:00:15,000") + 1 */
#define TIMELEN 13

#define TAG_UNRECOGNIZED	0
#define TAG_P			1
#define TAG_BR			2

typedef struct process_state process_state;

struct process_state {
	FILE *out;
	int sub_init;
	unsigned sub_num;
	char sub_begin[TIMELEN];
	char sub_end[TIMELEN];
	int sp;
};

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

void start_p_handler(process_state *st, const char **atts)
{
	struct tm begintime, endtime;
	const char *begin, *end, *dur, *rest;
	unsigned ms, ms_begin, len;

	if(st->sub_init)
		return;
	st->sub_init = 1;
	st->sub_num++;
	st->sp = 1;

	// TODO: Check that times and durations are in fact valid

	begin = get_attr(atts, "begin");
	if(begin != NULL) {
		rest = strptime(begin, "%H:%M:%S.", &begintime);
		sscanf(rest, "%u\n", &ms);
		ms_begin = ms;
		len = strftime(st->sub_begin, TIMELEN, "%H:%M:%S,", &begintime);
		snprintf(st->sub_begin + len, TIMELEN - len, "%03u", ms);
	}

	end = get_attr(atts, "end");
	if(end != NULL) {
		rest = strptime(end, "%H:%M:%S.", &endtime);
		sscanf(rest, "%u\n", &ms);
		len = strftime(st->sub_end, TIMELEN, "%H:%M:%S,", &endtime);
		snprintf(st->sub_end + len, TIMELEN - len, "%03u", ms);
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
		len = strftime(st->sub_end, TIMELEN, "%H:%M:%S,", &endtime);
		snprintf(st->sub_end + len, TIMELEN - len, "%03u", ms);
	}

	fprintf(st->out, "%u\n", st->sub_num);
	fprintf(st->out, "%s", st->sub_begin);
	if(strlen(st->sub_end) != 0)
		fprintf(st->out, " --> %s\n", st->sub_end);
	else
		fprintf(st->out, "\n");
}

void start_element_handler(void *udata, const char *name, const char **atts)
{
	process_state *st;

	st = udata;
	switch(identify_tag(name)) {
	case TAG_P:
		start_p_handler(st, atts);
		break;
	case TAG_BR:
		fprintf(st->out, "\n");
		st->sp = 1;
		break;
	default:
		break;
	}
	fflush(st->out);
}

void end_element_handler(void *udata, const char *name)
{
	process_state *st;

	st = udata;
	switch(identify_tag(name)) {
	case TAG_P:
		if(st->sub_init) {
			fprintf(st->out, "\n\n");
			st->sub_init = 0;
		}
		break;
	default:
		break;
	}
	fflush(st->out);
}

void text_handler(void *udata, const char *text, int len)
{
	process_state *st;
	int i;
	char c;
	int sp;

	st = udata;

	if(!st->sub_init)
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
			if(st->sp)
				break;
			else
				fputc(c, st->out);
			break;
		default:
			fputc(c, st->out);
		}
		st->sp = sp;
	}
	fflush(st->out);
}

void process(FILE *in, FILE *out)
{
	char buf[BUFLEN];
	size_t readlen;
	int isfinal;
	int ok;
	enum XML_Error errcode;
	process_state st = { 0 };
	XML_Parser parser; /* Really no asterisk */

	st.out = out;

	parser = XML_ParserCreate(NULL);
	XML_SetElementHandler(parser,
		(XML_StartElementHandler)start_element_handler,
		(XML_EndElementHandler)end_element_handler);
	XML_SetCharacterDataHandler(parser,
		(XML_CharacterDataHandler)text_handler);
	XML_SetUserData(parser, &st);

	fprintf(st.out, "\xef\xbb\xbf"); /* Byte Order Mark */
	fflush(st.out);

	for(;;) {
		readlen = fread(buf, 1, BUFLEN, in);
		isfinal = readlen != BUFLEN;
		ok = XML_Parse(parser, buf, readlen, isfinal);
		if(!ok) {
			errcode = XML_GetErrorCode(parser);
			fprintf(stderr, "XML_Parse: %s.\n", XML_ErrorString(errcode));
			exit(EXIT_FAILURE);
		}
		if(isfinal)
			break;
	}

	XML_ParserFree(parser);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <execinfo.h>
#include "utility.h"

#define MAX_FRAMES 31

struct backtrace {
	unsigned int frame_nr;
	unsigned long frames[MAX_FRAMES];
};

static void capture_backtrace(struct backtrace *b)
{
	int i;
	void *frames[MAX_FRAMES];

	b->frame_nr = backtrace(frames, MAX_FRAMES);
	for (i = 0; i < b->frame_nr; i++)
		b->frames[i] = (unsigned long) frames[i];
}

static void out_of_memory()
{
	fatal("out of memory\n");
}

void *xmalloc(int size)
{
	char *p = malloc(size);
	if (p == NULL)
		out_of_memory();
	return p;
}

void xfree(void *ptr)
{
	free(ptr);
}

char *xstrndup(char *str, int n)
{
	char *p = xmalloc(n + 1);
	memcpy(p, str, n);
	*(p + n) = 0;
	return p;
}

char *xstrdup(char *str)
{
	return xstrndup(str, strlen(str));
}

void fatal(const char *format, ...)
{
	int i;
	struct backtrace b;
	va_list args;

	va_start(args, format);

	capture_backtrace(&b);

	fprintf(stderr, "backtrace:\n");
	for (i = 0; i < b.frame_nr; i++)
		fprintf(stderr, "[%02d]  0x%016lx\n", i, b.frames[i]);
	fprintf(stderr, "\n");

	exit(EXIT_FAILURE);
}

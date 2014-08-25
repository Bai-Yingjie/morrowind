#ifndef _UTILITY_H_
#define _UTILITY_H_

extern void fatal(const char *format, ...);
extern void *xmalloc(int size);
extern void xfree(void *ptr);
extern char *xstrndup(char *str, int n);
extern char *xstrdup(char *str);

#endif

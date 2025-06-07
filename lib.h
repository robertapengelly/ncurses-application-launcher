/******************************************************************************
 * @file            lib.h
 *****************************************************************************/
#ifndef     _LIB_H
#define     _LIB_H

char *trim_whitespace (char *__p);

char *xstrdup (const char *__p);
char *xstrndup (const char *__p, unsigned long __len);

void *xmalloc (unsigned long __size);
void *xrealloc (void *__ptr, unsigned long __size);

#endif      /* _LIB_H */

/******************************************************************************
 * @file            lib.c
 *****************************************************************************/
#include    <ctype.h>
#include    <stdlib.h>
#include    <string.h>

#include    "lib.h"

char *trim_whitespace (char *__p) {

	char *end;

    // Trim leading space
	while (*__p == ' ' || *__p== '\t') {
		__p++;
	}

    // Trim trailing space
	if (*__p == 0) {
		return __p; // All spaces?
	}

    end = __p + strlen (__p) - 1;

	while (end > __p && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
		end--;
	}

    *(end + 1) = '\0';
    return __p;

}



char *xstrdup (const char *__p) {

    char *p = xmalloc (strlen (__p) + 1);
    
    strcpy (p, __p);
    return p;

}

char *xstrndup (const char *__p, unsigned long __len) {

    char *p = xmalloc (__len + 1);
    
    memcpy (p, __p, __len);
    return p;

}



void *xmalloc (unsigned long __size) {

    void *ptr = malloc (__size);
    
    if (!ptr && __size) {
    
        //report_at (program_name, 0, REPORT_ERROR, "memory full (malloc)");
        exit (EXIT_FAILURE);
    
    }
    
    memset (ptr, 0, __size);
    return ptr;

}

void *xrealloc (void *__ptr, unsigned long __size) {

    void *ptr = realloc (__ptr, __size);
    
    if (!ptr && __size) {
    
        //report_at (program_name, 0, REPORT_ERROR, "memory full (realloc)");
        exit (EXIT_FAILURE);
    
    }
    
    return ptr;

}

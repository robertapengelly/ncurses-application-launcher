/******************************************************************************
 * @file            ll.h
 *****************************************************************************/
#ifndef     _LL_H
#define     _LL_H

#include    <stdio.h>
int load_line (char **line_p, char **line_end_p, FILE *ifp, void **load_line_internal_data_p);

void load_line_destroy_internal_data (void *load_line_internal_data);
void *load_line_create_internal_data (void);

#endif      /* _LL_H */

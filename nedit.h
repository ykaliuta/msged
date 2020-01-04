/*
 *  NEDIT.H
 *
 *  Written on 10-Jul-94 by John Dennis and released to the public domain.
 *
 *  This structure defines a LINE of text.
 */

#ifndef __NEDIT_H__
#define __NEDIT_H__

#include <stdlib.h>
/*
 *  Probably possible to have an extra field called block_len, containing
 *  the length of the block - this would allow rectangular blocks.
 */

typedef struct _line
{
    char *text;                 /* pointer to actual line text */
    unsigned int block  : 1;    /* this is in a block */
    unsigned int hide   : 1;    /* this is a hidden line */
    unsigned int quote  : 1;    /* this is a quoted line */
    unsigned int templt : 1;    /* was this a template line? */
    int cursor_position;        /* != 0 if line has the cursor */
    struct _line *prev;         /* previous line in BUFFER */
    struct _line *next;         /* next line in BUFFER */
}
LINE;

size_t line_len(LINE *);
size_t line_size(LINE *);
char *line_str(LINE *);
char *line_chr(LINE *, int);
long utf8_len(char *);
char *utf8_pos(char *, long);
/* number of utf8 chars between pointers */
long utf8_range(char *, char *);

#endif

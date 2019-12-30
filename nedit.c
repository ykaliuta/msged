
#include "nedit.h"
#include <stdlib.h>
#include <string.h>

struct line
{
    char *text;                 /* pointer to actual line text */
    unsigned int block  : 1;    /* this is in a block */
    unsigned int hide   : 1;    /* this is a hidden line */
    unsigned int quote  : 1;    /* this is a quoted line */
    unsigned int templt : 1;    /* was this a template line? */
    int cursor_position;        /* != 0 if line has the cursor */
    struct _line *prev;         /* previous line in BUFFER */
    struct _line *next;         /* next line in BUFFER */
};

/* calculate symbol length of the given line */
size_t line_len(struct _line *_l)
{
    struct line *l = (struct line *)_l;

    return strlen(l->text);
}

size_t line_size(struct _line *_l)
{
    struct line *l = (struct line *)_l;

    return strlen(l->text) + 1;
}

char *line_str(LINE *_l)
{
    struct line *l = (struct line *)_l;

    return l->text;
}

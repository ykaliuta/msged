
#include "nedit.h"
#include <errno.h>
#include <stdbool.h>
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

enum utf8_state {
    START_SEQ,
    PROCESS_SEQ,
    FINISH,
    ERR,
};

static enum utf8_state utf8_check_start(unsigned char c, size_t *n)
{
    size_t num;

    if ((c & 0x80) == 0)
	num = 1;
    else if (((c & 0xc0) == 0xc0) && ((c & 0x20) == 0))
	num = 2;
    else if (((c & 0xe0) == 0xe0) && ((c & 0x10) == 0))
	num = 3;
    else if (((c & 0xf0) == 0xf0) && ((c & 0x08) == 0))
	num = 4;
    else
	return ERR;

    *n = num;
    return PROCESS_SEQ;
}

static bool utf8_check_rest_byte(unsigned char c)
{
    return ((c & 0x80) == 0x80) && ((c & 0x40) == 0);
}

static bool utf8_check_rest_bytes(char *s, size_t i, size_t num)
{
    while (num--) {
	if (s[i] == '\0')
	    return false;
	if (!utf8_check_rest_byte(s[i]))
	    return false;
	i++;
    }
    return true;
}

static long utf8_process(char *s, long stop_position, char *stop_location,
			 char **stopped_position)
{
    enum utf8_state state = START_SEQ;
    size_t i;
    size_t num;
    long char_count;
    static const void *const states[] = {
	[START_SEQ] = &&START_SEQ,
	[PROCESS_SEQ] = &&PROCESS_SEQ,
	[FINISH] = &&FINISH,
	[ERR] = &&ERR,
    };

    i = 0;
    char_count = 0;
    goto START_SEQ;

START_SEQ:
    if ((s[i] == '\0')
	|| ((stop_position >= 0) && (char_count == stop_position))
	|| ((stop_location != NULL) && (&s[i]) >= stop_location))
	goto FINISH;
    state = utf8_check_start(s[i], &num);
    goto *states[state];

PROCESS_SEQ:
    i++;
    num--;
    if (!utf8_check_rest_bytes(s, i, num))
	goto ERR;
    i += num;
    char_count++;
    goto START_SEQ;

FINISH:
    if ((stop_position > 0) && (char_count != stop_position))
	return -E2BIG;

    if (stopped_position != NULL)
	*stopped_position = &s[i];
    return char_count;
ERR:
    return -EILSEQ;
}

long utf8_len(char *s)
{
    return utf8_process(s, -1, NULL, NULL);
}

char *utf8_pos(char *s, long pos)
{
    char *p;
    long rc;

    rc = utf8_process(s, pos, NULL, &p);
    if (rc < 0)
	return NULL;
    return p;
}

/* number of utf8 chars between pointers */
long utf8_range(char *s, char *end)
{
    return utf8_process(s, -1, end, NULL);
}

/* calculate symbol length of the given line */
size_t line_len(struct _line *_l)
{
    struct line *l = (struct line *)_l;

    return utf8_len(l->text);
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

char *line_chr(LINE *_l, int c)
{
    struct line *l = (struct line *)_l;

    return strchr(l->text, c);
}

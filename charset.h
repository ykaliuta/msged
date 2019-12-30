#ifndef __CHARSET_H__
#define __CHARSET_H__
#include <iconv.h>
#include <stdbool.h>

/*
 *  CHARSET.H
 *
 *  Written 1998 by Tobias Ernst. Released to the Public Domain.
 *
 *  A FSC-0054 compliant character set translation engine for MsgEd.
 */

typedef struct _lookuptable
{
    iconv_t cd;
} LOOKUPTABLE;

typedef struct _charsetalias
{
    char           from_charset[9];
    char           to_charset[9];
} CHARSETALIAS;

void read_charset_maps (char *, char *); /* initialize the charset engine */
void destroy_charset_maps (void);        /* destroy the charset engine    */

#define READ_DIRECTION 1
#define WRITE_DIRECTION 2

int          have_readtable      (const char *, int);
LOOKUPTABLE *get_readtable       (const char *, int);
LOOKUPTABLE *get_writetable      (const char *, int*);
void         put_table           (LOOKUPTABLE *);
void         charset_alias       (const char *, const char *);
int          get_codepage_number (const char *);

void         strip_control_chars (char *);
char        *translate_text      (const char *, LOOKUPTABLE *);

char *get_known_charset_table(int *n_elem, int *elem_size); /* free ptr! */
char *get_local_charset(void); /* don't free ptr! */
void  set_local_charset(char *);
bool  is_local_utf8(void);

#endif



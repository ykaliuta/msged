/*
 *  CHARSET.C
 *
 *  Written 1998 by Tobias Ernst. Released to the Public Domain.
 *
 *  A FSC-0054 / FSP-1013 compliant character set translation engine for MsgEd.
 */

#include <time.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "addr.h"
#include "memextra.h"
#include "strextra.h"
#include "nedit.h"
#include "charset.h"
#include "msged.h"
#include "config.h"


static CHARSETALIAS *aliases = NULL;
static int naliases = 0;

static char *local_charset;
static bool local_utf8;

/* register an alias name for a charset kludge (for backward compatibility with
   things like IBMPC, 7_FIDO or RUFIDO ... */

void charset_alias (const char *from, const char *to)
{
    if (!naliases)
    {
        aliases=xmalloc(sizeof(CHARSETALIAS));
    }
    else
    {
        aliases=realloc(aliases, sizeof(CHARSETALIAS) * (1+naliases));
    }
    naliases++;
    strncpy(aliases[naliases-1].from_charset, from, 9);
    aliases[naliases-1].from_charset[8] = '\0';
    strncpy(aliases[naliases-1].to_charset, to, 9);
    aliases[naliases-1].to_charset[8] = '\0';
}

static const char *findalias(const char *kludge)
{
    int i;

    for (i = 0; i < naliases; i++)
    {
        if (!strcmp(aliases[i].from_charset, kludge))
            return aliases[i].to_charset;
    }
    return kludge;
}

void read_charset_maps(char *readmap, char *writemap)
{
    /* empty for iconv */
}

void destroy_charset_maps(void)
{
    /* empty for iconv */
}

struct str_to_str {
    char *key;
    char *val;
};

struct str_to_str level1_map[] = {
    { "ASCII", "ASCII" },
    { "DUTCH", "ISO646-DK" },
    { "FINNISH", "ISO646-FI" },
    { "FRENCH", "ISO646-FR" },
    { "CANADIAN", "ISO646-CA" },
    { "GERMAN", "ISO646-DE" },
    { "ITALIAN", "ISO646-IT" },
    { "NORWEIG", "ISO646-NO" },
    { "PORTU", "ISO646-PT" },
    { "SPANISH", "ISO646-ES" },
    { "SWEDISH", "ISO646-SE" },
    { "SWISS", "ISO646-CN" },
    { "UK", "ISO646-GB" },
};

static const char *charset_level_to_iconv(const char *charset, int level)
{
    int i;

    if (level != 1)
	return charset;

    for (i = 0; i < sizeof(level1_map)/sizeof(level1_map[0]); i++)
	if (stricmp(level1_map[i].key, charset) == 0)
	    return level1_map[i].val;
    return charset;
}

static LOOKUPTABLE *get_table (const char *charset_name, int level,
			       int *allowed, bool to_local)
{
    iconv_t cd;
    LOOKUPTABLE *table;
    const char *to;
    const char *from;

    charset_name = findalias(charset_name);
    charset_name = charset_level_to_iconv(charset_name, level);

    if (!strcmp(local_charset, charset_name))
    {
	if (allowed != NULL)
	    *allowed=1;
	return NULL;  /* no translation necessary */
    }

    if (to_local) {
	to = local_charset;
	from = charset_name;
    } else {
	to = charset_name;
	from = local_charset;
    }

    cd = iconv_open(to, from);
    if (cd == (iconv_t)-1) {
	cd = iconv_open("ASCII//TRANSLIT", from);
	if (cd == (iconv_t)-1) {
	    return NULL;
	}
    }

    table = xmalloc(sizeof(*table));
    table->cd = cd;

    return table;
}


/* Find a lookup table. Note: NULL pointer means no translation has to be
   done. If you specify an unknown charset name, you will not get NULL
   pointer, but you will get the maskout table (which maps everything to
   a questionmark). If you do not want this, use have_readtable to test! */

LOOKUPTABLE *get_readtable (const char *charset_name, int level)
{
    bool to_local = true;

    return get_table(charset_name, level, NULL, to_local);
}

LOOKUPTABLE * get_writetable(const char *charset_name, int *allowed)
{
    bool from_local = false;

    return get_table(charset_name, 0, allowed, from_local);
}

void put_table(LOOKUPTABLE *table)
{
    if (table == NULL)
	return;

    iconv_close(table->cd);
    free(table);
}


/* Test if we have a read table for this charset */

int have_readtable (const char *charset_name, int level)
{
    return 1; /* TODO: check it with iconv */
}



/* this routine filters out control codes that could break vt100 */

void strip_control_chars (char *text)
{
    /* broken on utf */
    return;
#if defined(UNIX) || defined(SASC)

    unsigned char c;
    size_t dstidx, len;

    if (text == NULL) return;

    len  = strlen(text);

    for (dstidx = 0; dstidx < len; dstidx++)
    {
        c = *(unsigned char*)(text + dstidx);
        if ( (c < 32 && c != '\n' && c != '\r' && c != '\001') ||
            (c >= 128 && c < 160) )
        {
            text[dstidx] = '?';
        }
    }
#endif
}

char *translate_text (const char *text, LOOKUPTABLE *table)
{
    char * translated;
    char *d; /* current pointer for iconv */
    size_t slen;
    size_t dlen;
    size_t filled; /* in dst */
    size_t allocated;
    size_t inc;
    size_t rc;

    if (text == NULL)
    {
        return NULL;
    }

    slen = strlen(text);
    dlen = slen + 1;
    translated = xmalloc(dlen);
                 /* at first, we assume 1:1 translation */

    if (table == NULL) /* no translation necessary or possible */
    {
       if (dlen != 0)
       {
	   memcpy (translated, text, dlen); /* does final \0 */
       }
       return translated;
    }

    inc = slen;
    allocated = dlen;
    d = translated;
    slen++; /* translate also final \0 */

    while (slen) {
	rc = iconv(table->cd, &text, &slen, &d, &dlen);
	if (rc != (size_t)-1)
	    break;

	if (errno != E2BIG) {
	    text++;
	    slen--;
	    continue;
	}
	filled = allocated - dlen;
	translated = xrealloc(translated, allocated + inc);
	allocated += inc;
	d = translated + filled;
	dlen += inc;
    }

    translated[allocated - 1] = '\0';
    return translated;
}


int get_codepage_number(const char *kludge_name)
{
    kludge_name = findalias(kludge_name);

    if (kludge_name[0] == 'C' && kludge_name[1] == 'P')
        return atoi(kludge_name + 2);
    else
        return 0;
}

char *get_local_charset(void)
{
    static char buffer[20];

    sprintf (buffer, "%s 2", local_charset);
    return buffer;
}

void set_local_charset(char *charset)
{
    local_charset = charset;

    if (stricmp(local_charset, "utf-8") == 0)
	local_utf8 = true;
    else
	local_utf8 = false;
}

bool is_local_utf8(void)
{
    return local_utf8;
}

static int ct_comparator(const void *p1, const void *p2)
{
    return stricmp((const char *)p1, (const char *)p2);
}

/* This function gets a human readable list of character set for which we have
   read maps available. It can be used by the calling program to display a list
   of these character sets, e.g. when offering the user a possibility to
   override a character set kludge in the mail and to manually select the read
   map to use.

   nelem and elem_size must not be NULL; they will be filled in with the number
   of elements in the list and the size of each element (including a trailing
   \0), respectively.

   The pointer that is returned has to be free'ed by the program.
*/

char *get_known_charset_table(int *nelem, int *elem_size)
{
    return NULL; /* not implemented for iconv yet */

#if 0
    char *array;
    int i;

    if (nelem == NULL || elem_size == NULL || readmaps == NULL ||
        readmaps->tables == NULL || readmaps->n_tables <= 0)
    {
        return NULL;
    }

    *elem_size = 9 + 1 + 1; /* name, space, level */
    *nelem = readmaps->n_tables;
    array = malloc(((*nelem) + 1)* (*elem_size));

    for (i = 0; i < (*nelem); i++)
    {
        sprintf(array + i * (*elem_size), "%s %d",
                readmaps->tables[i].from_charset,
                readmaps->tables[i].level);
    }

    sprintf (array + (*nelem) * (*elem_size), "%s 2",
             readmaps->charset_name);
    (*nelem)++;

    qsort(array, *nelem, *elem_size, ct_comparator);

    /* filter out duplicates */

    for (i = 0; i < (*nelem) - 1; i++)
    {
        if (!stricmp(array + i * (*elem_size),
                     array + (i + 1) * (*elem_size)))
        {
            memmove(array + i * (*elem_size),
                    array + (i + 1) * (*elem_size),
                    ((*nelem) - i - 1) * (*elem_size));
            (*nelem)--;
        }
    }

    return array;
#endif
}

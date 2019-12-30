
#include "nedit.h"
#include <stdlib.h>
#include <string.h>

/* calculate symbol length of the given line */
size_t line_len(LINE *l)
{
    return strlen(l->text);
}

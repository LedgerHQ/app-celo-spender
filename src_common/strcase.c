#include "strcase.h"

#include <ctype.h>

int _strcasecmp(const char *s1, const char *s2) {
    const unsigned char *p1 = (const unsigned char *) s1;
    const unsigned char *p2 = (const unsigned char *) s2;
    int result = 0;
    if (p1 == p2) return 0;
    while ((result = toupper(*p1) - toupper(*p2++)) == 0)
        if (*p1++ == '\0') break;
    return result;
}

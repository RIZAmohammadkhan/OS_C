#include "util/strutil.h"

#include <ctype.h>
#include <string.h>

static int ci_cmp_char(char a, char b) {
    unsigned char ua = (unsigned char)a;
    unsigned char ub = (unsigned char)b;
    return tolower(ua) - tolower(ub);
}

bool str_eq_ci(const char* a, const char* b) {
    if (a == NULL || b == NULL) {
        return false;
    }
    while (*a && *b) {
        if (ci_cmp_char(*a, *b) != 0) {
            return false;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

bool str_starts_with_ci(const char* s, const char* prefix) {
    if (s == NULL || prefix == NULL) {
        return false;
    }
    while (*prefix) {
        if (*s == '\0') {
            return false;
        }
        if (ci_cmp_char(*s, *prefix) != 0) {
            return false;
        }
        s++;
        prefix++;
    }
    return true;
}

bool str_trim_inplace(char* s) {
    if (s == NULL) {
        return false;
    }
    size_t len = strlen(s);
    size_t start = 0;
    while (start < len && isspace((unsigned char)s[start])) {
        start++;
    }
    size_t end = len;
    while (end > start && isspace((unsigned char)s[end - 1])) {
        end--;
    }

    if (start > 0) {
        memmove(s, s + start, end - start);
    }
    s[end - start] = '\0';
    return true;
}

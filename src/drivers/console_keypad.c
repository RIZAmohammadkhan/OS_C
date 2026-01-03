#include "drivers/console_keypad.h"

#include <stdio.h>
#include <string.h>

static bool console_read_line(Keypad* self, char* out, size_t out_cap) {
    (void)self;
    if (out_cap == 0) {
        return false;
    }

    if (fgets(out, (int)out_cap, stdin) == NULL) {
        return false;
    }

    size_t n = strlen(out);
    while (n > 0 && (out[n - 1] == '\n' || out[n - 1] == '\r')) {
        out[n - 1] = '\0';
        n--;
    }
    return true;
}

Keypad console_keypad_create(void) {
    Keypad k;
    k.read_line = console_read_line;
    return k;
}

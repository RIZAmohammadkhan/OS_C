#include "drivers/console_display.h"

#include <stdio.h>

static void console_write(Display* self, const char* s) {
    (void)self;
    fputs(s, stdout);
    fflush(stdout);
}

static void console_write_line(Display* self, const char* s) {
    console_write(self, s);
    fputc('\n', stdout);
    fflush(stdout);
}

Display console_display_create(void) {
    Display d;
    d.write = console_write;
    d.write_line = console_write_line;
    return d;
}

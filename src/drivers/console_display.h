#pragma once

#include <stddef.h>

typedef struct Display Display;

typedef void (*DisplayWriteFn)(Display* self, const char* s);
typedef void (*DisplayWriteLineFn)(Display* self, const char* s);

struct Display {
    DisplayWriteFn write;
    DisplayWriteLineFn write_line;
};

Display console_display_create(void);

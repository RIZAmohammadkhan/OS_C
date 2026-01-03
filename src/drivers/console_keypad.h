#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct Keypad Keypad;

typedef bool (*KeypadReadLineFn)(Keypad* self, char* out, size_t out_cap);

struct Keypad {
    KeypadReadLineFn read_line;
};

Keypad console_keypad_create(void);

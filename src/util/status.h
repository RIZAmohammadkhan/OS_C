#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    bool ok;
    const char* msg;
} Status;

static inline Status status_ok(void) {
    Status s = { .ok = true, .msg = NULL };
    return s;
}

static inline Status status_err(const char* msg) {
    Status s = { .ok = false, .msg = msg };
    return s;
}

#pragma once

#include <stdbool.h>
#include <stddef.h>

bool str_eq_ci(const char* a, const char* b);
bool str_starts_with_ci(const char* s, const char* prefix);
bool str_trim_inplace(char* s);

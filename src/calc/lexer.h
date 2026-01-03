#pragma once

#include "util/status.h"
#include "calc/tokens.h"

#include <stddef.h>

Status lexer_tokenize(const char* input, Token* out, size_t out_cap, size_t* out_len);

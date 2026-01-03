#pragma once

#include <stddef.h>

typedef enum {
    TOK_END = 0,
    TOK_NUMBER,
    TOK_IDENT,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_CARET,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_COMMA,
} TokenKind;

typedef struct {
    TokenKind kind;
    const char* start;
    size_t len;
    double number;
} Token;

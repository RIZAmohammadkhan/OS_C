#include "calc/lexer.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static bool is_ident_start(char c) {
    return isalpha((unsigned char)c) || c == '_';
}

static bool is_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static Status push_token(Token* out, size_t out_cap, size_t* out_len, Token t) {
    if (*out_len >= out_cap) {
        return status_err("error: token buffer overflow");
    }
    out[*out_len] = t;
    (*out_len)++;
    return status_ok();
}

Status lexer_tokenize(const char* input, Token* out, size_t out_cap, size_t* out_len) {
    *out_len = 0;
    const char* p = input;

    while (*p) {
        while (*p && isspace((unsigned char)*p)) {
            p++;
        }
        if (*p == '\0') {
            break;
        }

        Token t;
        memset(&t, 0, sizeof(t));
        t.start = p;

        switch (*p) {
            case '+': t.kind = TOK_PLUS; t.len = 1; p++; break;
            case '-': t.kind = TOK_MINUS; t.len = 1; p++; break;
            case '*': t.kind = TOK_STAR; t.len = 1; p++; break;
            case '/': t.kind = TOK_SLASH; t.len = 1; p++; break;
            case '^': t.kind = TOK_CARET; t.len = 1; p++; break;
            case '(': t.kind = TOK_LPAREN; t.len = 1; p++; break;
            case ')': t.kind = TOK_RPAREN; t.len = 1; p++; break;
            case ',': t.kind = TOK_COMMA; t.len = 1; p++; break;
            default: {
                if (isdigit((unsigned char)*p) || *p == '.') {
                    errno = 0;
                    char* endptr = NULL;
                    double v = strtod(p, &endptr);
                    if (endptr == p) {
                        return status_err("error: invalid number");
                    }
                    if (errno == ERANGE) {
                        return status_err("error: number out of range");
                    }
                    t.kind = TOK_NUMBER;
                    t.number = v;
                    t.len = (size_t)(endptr - p);
                    p = endptr;
                    break;
                }
                if (is_ident_start(*p)) {
                    const char* start = p;
                    p++;
                    while (*p && is_ident_char(*p)) {
                        p++;
                    }
                    t.kind = TOK_IDENT;
                    t.start = start;
                    t.len = (size_t)(p - start);
                    break;
                }
                return status_err("error: unexpected character");
            }
        }

        Status st = push_token(out, out_cap, out_len, t);
        if (!st.ok) {
            return st;
        }
    }

    Token end;
    memset(&end, 0, sizeof(end));
    end.kind = TOK_END;
    end.start = p;
    end.len = 0;
    return push_token(out, out_cap, out_len, end);
}

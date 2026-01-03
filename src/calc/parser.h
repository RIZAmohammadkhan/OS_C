#pragma once

#include "util/status.h"
#include "calc/tokens.h"

#include <stddef.h>

typedef enum {
    AST_NODE_INVALID = -1,
} AstNodeId;

typedef enum {
    AST_NUM,
    AST_VAR,
    AST_UNARY,
    AST_BINARY,
    AST_CALL,
} AstKind;

typedef enum {
    UN_NEG,
    UN_POS,
} UnaryOp;

typedef enum {
    BIN_ADD,
    BIN_SUB,
    BIN_MUL,
    BIN_DIV,
    BIN_POW,
} BinaryOp;

typedef struct {
    const Token* tokens;
    size_t token_count;
    size_t pos;
} TokenStream;

typedef struct {
    AstKind kind;
    union {
        double num;
        struct { char name[16]; } var;
        struct { UnaryOp op; int child; } unary;
        struct { BinaryOp op; int lhs; int rhs; } binary;
        struct { char name[16]; int args[4]; size_t argc; } call;
    } as;
} AstNode;

typedef struct {
    AstNode* nodes;
    size_t node_cap;
    size_t node_len;
    int root;
} Ast;

Status parser_parse(const Token* tokens, size_t token_count, Ast* out);

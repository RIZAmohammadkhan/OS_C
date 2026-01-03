#include "calc/parser.h"

#include <ctype.h>
#include <string.h>

static const Token* ts_peek(const TokenStream* ts) {
    if (ts->pos >= ts->token_count) {
        return &ts->tokens[ts->token_count - 1];
    }
    return &ts->tokens[ts->pos];
}

static const Token* ts_prev(const TokenStream* ts) {
    if (ts->pos == 0) {
        return &ts->tokens[0];
    }
    return &ts->tokens[ts->pos - 1];
}

static const Token* ts_advance(TokenStream* ts) {
    if (ts->pos < ts->token_count) {
        ts->pos++;
    }
    return ts_prev(ts);
}

static bool ts_match(TokenStream* ts, TokenKind kind) {
    if (ts_peek(ts)->kind == kind) {
        ts_advance(ts);
        return true;
    }
    return false;
}

static Status ast_push(Ast* ast, AstNode node, int* out_id) {
    if (ast->node_len >= ast->node_cap) {
        return status_err("error: AST too large");
    }
    ast->nodes[ast->node_len] = node;
    *out_id = (int)ast->node_len;
    ast->node_len++;
    return status_ok();
}

static void token_to_ident16(const Token* t, char out[16]) {
    size_t n = t->len;
    if (n > 15) {
        n = 15;
    }
    for (size_t i = 0; i < n; i++) {
        char c = t->start[i];
        out[i] = (char)tolower((unsigned char)c);
    }
    out[n] = '\0';
}

/* Grammar (Pratt-ish precedence):
   expr        := add
   add         := mul (('+'|'-') mul)*
   mul         := pow (('*'|'/') pow)*
   pow         := unary ('^' pow)?   (right associative)
   unary       := ('+'|'-') unary | primary
   primary     := number | ident | call | '(' expr ')'
   call        := ident '(' [expr (',' expr)*] ')'
*/

static Status parse_expr(TokenStream* ts, Ast* ast, int* out);

static Status parse_primary(TokenStream* ts, Ast* ast, int* out) {
    const Token* t = ts_peek(ts);
    if (ts_match(ts, TOK_NUMBER)) {
        AstNode n;
        memset(&n, 0, sizeof(n));
        n.kind = AST_NUM;
        n.as.num = t->number;
        return ast_push(ast, n, out);
    }

    if (ts_match(ts, TOK_IDENT)) {
        Token ident = *t;
        if (ts_match(ts, TOK_LPAREN)) {
            AstNode call;
            memset(&call, 0, sizeof(call));
            call.kind = AST_CALL;
            token_to_ident16(&ident, call.as.call.name);
            call.as.call.argc = 0;

            if (!ts_match(ts, TOK_RPAREN)) {
                while (1) {
                    if (call.as.call.argc >= 4) {
                        return status_err("error: too many function args");
                    }
                    int arg_id = AST_NODE_INVALID;
                    Status st = parse_expr(ts, ast, &arg_id);
                    if (!st.ok) {
                        return st;
                    }
                    call.as.call.args[call.as.call.argc++] = arg_id;

                    if (ts_match(ts, TOK_COMMA)) {
                        continue;
                    }
                    if (ts_match(ts, TOK_RPAREN)) {
                        break;
                    }
                    return status_err("error: expected ',' or ')'");
                }
            }
            return ast_push(ast, call, out);
        }

        AstNode v;
        memset(&v, 0, sizeof(v));
        v.kind = AST_VAR;
        token_to_ident16(&ident, v.as.var.name);
        return ast_push(ast, v, out);
    }

    if (ts_match(ts, TOK_LPAREN)) {
        Status st = parse_expr(ts, ast, out);
        if (!st.ok) {
            return st;
        }
        if (!ts_match(ts, TOK_RPAREN)) {
            return status_err("error: expected ')'");
        }
        return status_ok();
    }

    return status_err("error: expected primary expression");
}

static Status parse_unary(TokenStream* ts, Ast* ast, int* out) {
    if (ts_match(ts, TOK_PLUS)) {
        int child = AST_NODE_INVALID;
        Status st = parse_unary(ts, ast, &child);
        if (!st.ok) {
            return st;
        }
        AstNode n;
        memset(&n, 0, sizeof(n));
        n.kind = AST_UNARY;
        n.as.unary.op = UN_POS;
        n.as.unary.child = child;
        return ast_push(ast, n, out);
    }
    if (ts_match(ts, TOK_MINUS)) {
        int child = AST_NODE_INVALID;
        Status st = parse_unary(ts, ast, &child);
        if (!st.ok) {
            return st;
        }
        AstNode n;
        memset(&n, 0, sizeof(n));
        n.kind = AST_UNARY;
        n.as.unary.op = UN_NEG;
        n.as.unary.child = child;
        return ast_push(ast, n, out);
    }
    return parse_primary(ts, ast, out);
}

static Status parse_pow(TokenStream* ts, Ast* ast, int* out) {
    int left = AST_NODE_INVALID;
    Status st = parse_unary(ts, ast, &left);
    if (!st.ok) {
        return st;
    }

    if (ts_match(ts, TOK_CARET)) {
        int right = AST_NODE_INVALID;
        st = parse_pow(ts, ast, &right); /* right associative */
        if (!st.ok) {
            return st;
        }
        AstNode n;
        memset(&n, 0, sizeof(n));
        n.kind = AST_BINARY;
        n.as.binary.op = BIN_POW;
        n.as.binary.lhs = left;
        n.as.binary.rhs = right;
        return ast_push(ast, n, out);
    }

    *out = left;
    return status_ok();
}

static Status parse_mul(TokenStream* ts, Ast* ast, int* out) {
    int expr = AST_NODE_INVALID;
    Status st = parse_pow(ts, ast, &expr);
    if (!st.ok) {
        return st;
    }

    while (1) {
        if (ts_match(ts, TOK_STAR)) {
            int rhs = AST_NODE_INVALID;
            st = parse_pow(ts, ast, &rhs);
            if (!st.ok) {
                return st;
            }
            AstNode n;
            memset(&n, 0, sizeof(n));
            n.kind = AST_BINARY;
            n.as.binary.op = BIN_MUL;
            n.as.binary.lhs = expr;
            n.as.binary.rhs = rhs;
            st = ast_push(ast, n, &expr);
            if (!st.ok) {
                return st;
            }
            continue;
        }
        if (ts_match(ts, TOK_SLASH)) {
            int rhs = AST_NODE_INVALID;
            st = parse_pow(ts, ast, &rhs);
            if (!st.ok) {
                return st;
            }
            AstNode n;
            memset(&n, 0, sizeof(n));
            n.kind = AST_BINARY;
            n.as.binary.op = BIN_DIV;
            n.as.binary.lhs = expr;
            n.as.binary.rhs = rhs;
            st = ast_push(ast, n, &expr);
            if (!st.ok) {
                return st;
            }
            continue;
        }
        break;
    }

    *out = expr;
    return status_ok();
}

static Status parse_add(TokenStream* ts, Ast* ast, int* out) {
    int expr = AST_NODE_INVALID;
    Status st = parse_mul(ts, ast, &expr);
    if (!st.ok) {
        return st;
    }

    while (1) {
        if (ts_match(ts, TOK_PLUS)) {
            int rhs = AST_NODE_INVALID;
            st = parse_mul(ts, ast, &rhs);
            if (!st.ok) {
                return st;
            }
            AstNode n;
            memset(&n, 0, sizeof(n));
            n.kind = AST_BINARY;
            n.as.binary.op = BIN_ADD;
            n.as.binary.lhs = expr;
            n.as.binary.rhs = rhs;
            st = ast_push(ast, n, &expr);
            if (!st.ok) {
                return st;
            }
            continue;
        }
        if (ts_match(ts, TOK_MINUS)) {
            int rhs = AST_NODE_INVALID;
            st = parse_mul(ts, ast, &rhs);
            if (!st.ok) {
                return st;
            }
            AstNode n;
            memset(&n, 0, sizeof(n));
            n.kind = AST_BINARY;
            n.as.binary.op = BIN_SUB;
            n.as.binary.lhs = expr;
            n.as.binary.rhs = rhs;
            st = ast_push(ast, n, &expr);
            if (!st.ok) {
                return st;
            }
            continue;
        }
        break;
    }

    *out = expr;
    return status_ok();
}

static Status parse_expr(TokenStream* ts, Ast* ast, int* out) {
    return parse_add(ts, ast, out);
}

Status parser_parse(const Token* tokens, size_t token_count, Ast* out) {
    out->node_len = 0;
    out->root = AST_NODE_INVALID;

    if (token_count == 0) {
        return status_err("error: empty input");
    }

    TokenStream ts = { .tokens = tokens, .token_count = token_count, .pos = 0 };

    int root = AST_NODE_INVALID;
    Status st = parse_expr(&ts, out, &root);
    if (!st.ok) {
        return st;
    }

    if (ts_peek(&ts)->kind != TOK_END) {
        return status_err("error: unexpected trailing tokens");
    }

    out->root = root;
    return status_ok();
}

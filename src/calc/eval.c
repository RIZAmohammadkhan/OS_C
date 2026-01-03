#include "calc/eval.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif

void eval_context_init(EvalContext* ctx) {
    ctx->angle_mode_deg = 1;
    ctx->ans = 0.0;
    ctx->mem = 0.0;
    ctx->mem_set = 0;
}

static double to_radians(const EvalContext* ctx, double x) {
    if (ctx->angle_mode_deg) {
        return x * (M_PI / 180.0);
    }
    return x;
}

static double from_radians(const EvalContext* ctx, double x) {
    if (ctx->angle_mode_deg) {
        return x * (180.0 / M_PI);
    }
    return x;
}

static bool isfinite_safe(double x) {
    return isfinite(x) != 0;
}

static Status eval_node(const Ast* ast, int id, const EvalContext* ctx, double* out);

static Status eval_var(const char* name, const EvalContext* ctx, double* out) {
    if (strcmp(name, "pi") == 0) {
        *out = M_PI;
        return status_ok();
    }
    if (strcmp(name, "e") == 0) {
        *out = M_E;
        return status_ok();
    }
    if (strcmp(name, "ans") == 0) {
        *out = ctx->ans;
        return status_ok();
    }
    if (strcmp(name, "mem") == 0) {
        if (!ctx->mem_set) {
            return status_err("error: mem is unset");
        }
        *out = ctx->mem;
        return status_ok();
    }
    return status_err("error: unknown variable");
}

static Status eval_call(const AstNode* n, const Ast* ast, const EvalContext* ctx, double* out) {
    const char* fn = n->as.call.name;

    double a0 = 0.0;
    if (n->as.call.argc >= 1) {
        Status st = eval_node(ast, n->as.call.args[0], ctx, &a0);
        if (!st.ok) {
            return st;
        }
    }

    if (strcmp(fn, "sin") == 0) {
        if (n->as.call.argc != 1) return status_err("error: sin(x) expects 1 arg");
        *out = sin(to_radians(ctx, a0));
        return status_ok();
    }
    if (strcmp(fn, "cos") == 0) {
        if (n->as.call.argc != 1) return status_err("error: cos(x) expects 1 arg");
        *out = cos(to_radians(ctx, a0));
        return status_ok();
    }
    if (strcmp(fn, "tan") == 0) {
        if (n->as.call.argc != 1) return status_err("error: tan(x) expects 1 arg");
        *out = tan(to_radians(ctx, a0));
        return status_ok();
    }
    if (strcmp(fn, "asin") == 0) {
        if (n->as.call.argc != 1) return status_err("error: asin(x) expects 1 arg");
        *out = from_radians(ctx, asin(a0));
        return status_ok();
    }
    if (strcmp(fn, "acos") == 0) {
        if (n->as.call.argc != 1) return status_err("error: acos(x) expects 1 arg");
        *out = from_radians(ctx, acos(a0));
        return status_ok();
    }
    if (strcmp(fn, "atan") == 0) {
        if (n->as.call.argc != 1) return status_err("error: atan(x) expects 1 arg");
        *out = from_radians(ctx, atan(a0));
        return status_ok();
    }
    if (strcmp(fn, "sqrt") == 0) {
        if (n->as.call.argc != 1) return status_err("error: sqrt(x) expects 1 arg");
        if (a0 < 0.0) return status_err("error: sqrt domain");
        *out = sqrt(a0);
        return status_ok();
    }
    if (strcmp(fn, "abs") == 0) {
        if (n->as.call.argc != 1) return status_err("error: abs(x) expects 1 arg");
        *out = fabs(a0);
        return status_ok();
    }
    if (strcmp(fn, "ln") == 0) {
        if (n->as.call.argc != 1) return status_err("error: ln(x) expects 1 arg");
        if (a0 <= 0.0) return status_err("error: ln domain");
        *out = log(a0);
        return status_ok();
    }
    if (strcmp(fn, "log") == 0) {
        if (n->as.call.argc != 1) return status_err("error: log(x) expects 1 arg");
        if (a0 <= 0.0) return status_err("error: log domain");
        *out = log10(a0);
        return status_ok();
    }

    return status_err("error: unknown function");
}

static Status eval_node(const Ast* ast, int id, const EvalContext* ctx, double* out) {
    if (id < 0 || (size_t)id >= ast->node_len) {
        return status_err("error: invalid AST node");
    }
    const AstNode* n = &ast->nodes[id];

    switch (n->kind) {
        case AST_NUM:
            *out = n->as.num;
            return status_ok();
        case AST_VAR:
            return eval_var(n->as.var.name, ctx, out);
        case AST_UNARY: {
            double v = 0.0;
            Status st = eval_node(ast, n->as.unary.child, ctx, &v);
            if (!st.ok) {
                return st;
            }
            if (n->as.unary.op == UN_NEG) {
                *out = -v;
            } else {
                *out = v;
            }
            return status_ok();
        }
        case AST_BINARY: {
            double a = 0.0, b = 0.0;
            Status st = eval_node(ast, n->as.binary.lhs, ctx, &a);
            if (!st.ok) return st;
            st = eval_node(ast, n->as.binary.rhs, ctx, &b);
            if (!st.ok) return st;

            switch (n->as.binary.op) {
                case BIN_ADD: *out = a + b; break;
                case BIN_SUB: *out = a - b; break;
                case BIN_MUL: *out = a * b; break;
                case BIN_DIV:
                    if (b == 0.0) return status_err("error: division by zero");
                    *out = a / b;
                    break;
                case BIN_POW:
                    *out = pow(a, b);
                    break;
            }
            if (!isfinite_safe(*out)) {
                return status_err("error: result is not finite");
            }
            return status_ok();
        }
        case AST_CALL:
            return eval_call(n, ast, ctx, out);
        default:
            return status_err("error: unknown AST kind");
    }
}

Status eval_ast(const Ast* ast, int node_id, const EvalContext* ctx, double* out) {
    Status st = eval_node(ast, node_id, ctx, out);
    if (!st.ok) {
        return st;
    }
    if (!isfinite_safe(*out)) {
        return status_err("error: non-finite result");
    }
    return status_ok();
}

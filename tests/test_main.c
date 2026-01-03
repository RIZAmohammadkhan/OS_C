#include "calc/lexer.h"
#include "calc/parser.h"
#include "calc/eval.h"

#include <stdio.h>
#include <string.h>

static int fails = 0;

static void expect_ok(Status st, const char* msg) {
    if (!st.ok) {
        fprintf(stderr, "FAIL: %s: %s\n", msg, st.msg ? st.msg : "(no msg)");
        fails++;
    }
}

static void expect_near(double a, double b, double eps, const char* msg) {
    double d = a - b;
    if (d < 0) d = -d;
    if (d > eps) {
        fprintf(stderr, "FAIL: %s: got %.17g expected %.17g\n", msg, a, b);
        fails++;
    }
}

static Status eval_expr(const char* expr, int deg, double ans, double mem, int mem_set, double* out) {
    Token tokens[256];
    size_t tok_count = 0;
    Status st = lexer_tokenize(expr, tokens, 256, &tok_count);
    if (!st.ok) return st;

    AstNode nodes[256];
    Ast ast = { .nodes = nodes, .node_cap = 256, .node_len = 0, .root = AST_NODE_INVALID };
    st = parser_parse(tokens, tok_count, &ast);
    if (!st.ok) return st;

    EvalContext ctx;
    eval_context_init(&ctx);
    ctx.angle_mode_deg = deg;
    ctx.ans = ans;
    ctx.mem = mem;
    ctx.mem_set = mem_set;
    return eval_ast(&ast, ast.root, &ctx, out);
}

int main(void) {
    {
        double v = 0.0;
        Status st = eval_expr("2+2*3", 1, 0, 0, 0, &v);
        expect_ok(st, "basic precedence");
        expect_near(v, 8.0, 1e-12, "2+2*3");
    }
    {
        double v = 0.0;
        Status st = eval_expr("(2+2)*3", 1, 0, 0, 0, &v);
        expect_ok(st, "paren grouping");
        expect_near(v, 12.0, 1e-12, "(2+2)*3");
    }
    {
        double v = 0.0;
        Status st = eval_expr("2^3^2", 1, 0, 0, 0, &v);
        expect_ok(st, "pow right assoc");
        expect_near(v, 512.0, 1e-9, "2^(3^2)");
    }
    {
        double v = 0.0;
        Status st = eval_expr("sin(30)", 1, 0, 0, 0, &v);
        expect_ok(st, "sin deg");
        expect_near(v, 0.5, 1e-12, "sin(30deg)");
    }
    {
        double v = 0.0;
        Status st = eval_expr("sin(pi/2)", 0, 0, 0, 0, &v);
        expect_ok(st, "sin rad");
        expect_near(v, 1.0, 1e-12, "sin(pi/2)");
    }
    {
        double v = 0.0;
        Status st = eval_expr("ans+1", 1, 41.0, 0, 0, &v);
        expect_ok(st, "ans variable");
        expect_near(v, 42.0, 1e-12, "ans+1");
    }
    {
        double v = 0.0;
        Status st = eval_expr("mem+1", 1, 0, 0, 0, &v);
        if (st.ok) {
            fprintf(stderr, "FAIL: mem should be unset\n");
            fails++;
        }
    }

    if (fails == 0) {
        printf("OK\n");
        return 0;
    }
    printf("FAIL (%d)\n", fails);
    return 1;
}

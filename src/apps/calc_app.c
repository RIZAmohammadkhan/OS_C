#include "apps/calc_app.h"

#include "calc/eval.h"
#include "calc/format.h"
#include "calc/parser.h"
#include "calc/lexer.h"
#include "util/strutil.h"

#include <stdio.h>
#include <string.h>

static void write_prompt(Display* d, const CalcApp* app) {
    (void)app;
    d->write(d, "calc-os> ");
}

static void write_help(Display* d) {
    d->write_line(d, "Commands:");
    d->write_line(d, "  help");
    d->write_line(d, "  mode deg | mode rad");
    d->write_line(d, "  mem               (show)");
    d->write_line(d, "  mem set <expr>");
    d->write_line(d, "  mem clear");
    d->write_line(d, "  exit");
    d->write_line(d, "Expressions:");
    d->write_line(d, "  operators: + - * / ^");
    d->write_line(d, "  functions: sin cos tan asin acos atan ln log sqrt abs");
    d->write_line(d, "  constants: pi e");
    d->write_line(d, "  variables: ans mem");
}

void calc_app_init(CalcApp* app, Kernel* kernel, Display* display, Keypad* keypad) {
    app->kernel = kernel;
    app->display = display;
    app->keypad = keypad;
    app->angle_mode_deg = 1;
    app->ans = 0.0;
    app->mem = 0.0;
    app->mem_set = 0;
    app->initialized = 0;
    app->should_exit = 0;
}

void calc_app_deinit(CalcApp* app) {
    (void)app;
}

static Status eval_and_print(CalcApp* app, const char* expr) {
    Token tokens[256];
    size_t tok_count = 0;

    Status st = lexer_tokenize(expr, tokens, sizeof(tokens)/sizeof(tokens[0]), &tok_count);
    if (!st.ok) {
        return st;
    }

    AstNode ast_nodes[256];
    Ast ast = { .nodes = ast_nodes, .node_cap = sizeof(ast_nodes)/sizeof(ast_nodes[0]), .node_len = 0, .root = AST_NODE_INVALID };

    st = parser_parse(tokens, tok_count, &ast);
    if (!st.ok) {
        return st;
    }

    EvalContext ctx;
    eval_context_init(&ctx);
    ctx.angle_mode_deg = app->angle_mode_deg;
    ctx.ans = app->ans;
    ctx.mem = app->mem_set ? app->mem : 0.0;
    ctx.mem_set = app->mem_set;

    double out = 0.0;
    st = eval_ast(&ast, ast.root, &ctx, &out);
    if (!st.ok) {
        return st;
    }

    app->ans = out;

    char buf[128];
    format_double(out, buf, sizeof(buf));

    char line[160];
    snprintf(line, sizeof(line), "= %s", buf);
    app->display->write_line(app->display, line);
    return status_ok();
}

static void handle_line(CalcApp* app, char* line) {
    str_trim_inplace(line);
    if (line[0] == '\0') {
        return;
    }

    if (str_eq_ci(line, "exit") || str_eq_ci(line, "quit")) {
        app->should_exit = 1;
        return;
    }

    if (str_eq_ci(line, "help")) {
        write_help(app->display);
        return;
    }

    if (str_starts_with_ci(line, "mode ")) {
        char* arg = line + 5;
        str_trim_inplace(arg);
        if (str_eq_ci(arg, "deg")) {
            app->angle_mode_deg = 1;
            app->display->write_line(app->display, "mode: degrees");
            return;
        }
        if (str_eq_ci(arg, "rad")) {
            app->angle_mode_deg = 0;
            app->display->write_line(app->display, "mode: radians");
            return;
        }
        app->display->write_line(app->display, "error: expected 'deg' or 'rad'");
        return;
    }

    if (str_eq_ci(line, "mem")) {
        if (!app->mem_set) {
            app->display->write_line(app->display, "mem: (unset)");
            return;
        }
        char buf[128];
        format_double(app->mem, buf, sizeof(buf));
        char out[160];
        snprintf(out, sizeof(out), "mem: %s", buf);
        app->display->write_line(app->display, out);
        return;
    }

    if (str_eq_ci(line, "mem clear")) {
        app->mem_set = 0;
        app->mem = 0.0;
        app->display->write_line(app->display, "mem: cleared");
        return;
    }

    if (str_starts_with_ci(line, "mem set ")) {
        const char* expr = line + 8;
        Status st = eval_and_print(app, expr);
        if (!st.ok) {
            app->display->write_line(app->display, st.msg ? st.msg : "error");
            return;
        }
        app->mem = app->ans;
        app->mem_set = 1;
        app->display->write_line(app->display, "mem: set");
        return;
    }

    Status st = eval_and_print(app, line);
    if (!st.ok) {
        app->display->write_line(app->display, st.msg ? st.msg : "error");
    }
}

void calc_app_task(void* ctx) {
    CalcApp* app = (CalcApp*)ctx;

    if (!app->initialized) {
        app->display->write_line(app->display, "Calculator OS (sim) - type 'help' for commands");
        app->initialized = 1;
    }

    if (app->should_exit) {
        app->display->write_line(app->display, "bye");
        kernel_stop(app->kernel);
        return;
    }

    char line[256];
    write_prompt(app->display, app);
    if (!app->keypad->read_line(app->keypad, line, sizeof(line))) {
        app->should_exit = 1;
        app->display->write_line(app->display, "bye");
        kernel_stop(app->kernel);
        return;
    }
    handle_line(app, line);

    if (app->should_exit) {
        app->display->write_line(app->display, "bye");
        kernel_stop(app->kernel);
    }
}

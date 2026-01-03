#pragma once

#include "util/status.h"
#include "calc/parser.h"

typedef struct {
    int angle_mode_deg;
    double ans;
    double mem;
    int mem_set;
} EvalContext;

void eval_context_init(EvalContext* ctx);
Status eval_ast(const Ast* ast, int node_id, const EvalContext* ctx, double* out);

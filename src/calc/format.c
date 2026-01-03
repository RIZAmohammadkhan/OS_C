#include "calc/format.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

void format_double(double v, char* out, size_t out_cap) {
    if (out_cap == 0) {
        return;
    }

    if (isnan(v)) {
        snprintf(out, out_cap, "nan");
        return;
    }
    if (isinf(v)) {
        snprintf(out, out_cap, (v > 0) ? "inf" : "-inf");
        return;
    }

    /* 15 sig digits ~= double precision without noisy tails */
    snprintf(out, out_cap, "%.15g", v);

    /* normalize -0 */
    if (strcmp(out, "-0") == 0 || strcmp(out, "-0.0") == 0) {
        snprintf(out, out_cap, "0");
    }
}

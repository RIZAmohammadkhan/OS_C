#pragma once

#include "kernel/kernel.h"
#include "drivers/console_display.h"
#include "drivers/console_keypad.h"

typedef struct {
    Kernel* kernel;
    Display* display;
    Keypad* keypad;

    int angle_mode_deg; /* 1=deg, 0=rad */
    double ans;
    double mem;
    int mem_set;

    int initialized;
    int should_exit;
} CalcApp;

void calc_app_init(CalcApp* app, Kernel* kernel, Display* display, Keypad* keypad);
void calc_app_deinit(CalcApp* app);
void calc_app_task(void* ctx);

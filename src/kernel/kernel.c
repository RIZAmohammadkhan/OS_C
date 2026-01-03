#include "kernel/kernel.h"

#include <stdio.h>

void kernel_init(Kernel* k) {
    k->task_count = 0;
    k->tick = 0;
    k->running = false;
    for (size_t i = 0; i < sizeof(k->tasks) / sizeof(k->tasks[0]); i++) {
        k->tasks[i].fn = NULL;
        k->tasks[i].ctx = NULL;
        k->tasks[i].name = NULL;
        k->tasks[i].active = false;
    }
}

bool kernel_add_task(Kernel* k, KernelTaskFn fn, void* ctx, const char* name) {
    if (k->task_count >= (sizeof(k->tasks) / sizeof(k->tasks[0]))) {
        return false;
    }
    k->tasks[k->task_count].fn = fn;
    k->tasks[k->task_count].ctx = ctx;
    k->tasks[k->task_count].name = name;
    k->tasks[k->task_count].active = true;
    k->task_count++;
    return true;
}

void kernel_stop(Kernel* k) {
    k->running = false;
}

uint64_t kernel_tick(const Kernel* k) {
    return k->tick;
}

void kernel_run(Kernel* k) {
    k->running = true;
    while (k->running) {
        for (size_t i = 0; i < k->task_count; i++) {
            if (!k->tasks[i].active || k->tasks[i].fn == NULL) {
                continue;
            }
            k->tasks[i].fn(k->tasks[i].ctx);
            if (!k->running) {
                break;
            }
        }
        k->tick++;
        if (k->tick == UINT64_MAX) {
            fprintf(stderr, "kernel: tick overflow, stopping\n");
            k->running = false;
        }
    }
}

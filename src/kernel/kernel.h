#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef void (*KernelTaskFn)(void* ctx);

typedef struct {
    KernelTaskFn fn;
    void* ctx;
    const char* name;
    bool active;
} KernelTask;

typedef struct {
    KernelTask tasks[16];
    size_t task_count;
    uint64_t tick;
    bool running;
} Kernel;

void kernel_init(Kernel* k);
bool kernel_add_task(Kernel* k, KernelTaskFn fn, void* ctx, const char* name);
void kernel_stop(Kernel* k);
void kernel_run(Kernel* k);
uint64_t kernel_tick(const Kernel* k);

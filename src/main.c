#include "kernel/kernel.h"
#include "drivers/console_display.h"
#include "drivers/console_keypad.h"
#include "apps/calc_app.h"
#include "platform/linux_poweroff.h"

int main(void) {
    Display display = console_display_create();
    Keypad keypad = console_keypad_create();

    Kernel kernel;
    kernel_init(&kernel);

    CalcApp app;
    calc_app_init(&app, &kernel, &display, &keypad);

    kernel_add_task(&kernel, calc_app_task, &app, "calc_app");

    kernel_run(&kernel);

    calc_app_deinit(&app);

    /* If booted as an initramfs PID 1 under QEMU, exiting would panic.
       Attempt a clean poweroff in that case. */
    (void)linux_poweroff_if_pid1();
    return 0;
}

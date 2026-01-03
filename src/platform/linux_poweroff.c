#include "platform/linux_poweroff.h"

#include <unistd.h>

#if defined(__linux__)
#include <linux/reboot.h>
#include <sys/reboot.h>
#endif

int linux_poweroff_if_pid1(void) {
    if (getpid() != 1) {
        return -1;
    }

#if defined(__linux__)
    /* If this fails, we'll just return and PID 1 will exit (which typically panics).
       In QEMU this call should normally succeed. */
    (void)reboot(LINUX_REBOOT_CMD_POWER_OFF);
    return 0;
#else
    return 0;
#endif
}

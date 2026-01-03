#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void ensure_dir(const char *path) {
    if (mkdir(path, 0755) != 0) {
        if (errno != EEXIST) {
            (void)fprintf(stderr, "init: mkdir(%s) failed: %s\n", path, strerror(errno));
        }
    }
}

static void try_mount(const char *source, const char *target, const char *fstype, unsigned long flags) {
    if (mount(source, target, fstype, flags, NULL) != 0) {
        (void)fprintf(stderr,
                     "init: mount(%s -> %s, %s) failed: %s\n",
                     source ? source : "(null)",
                     target,
                     fstype ? fstype : "(null)",
                     strerror(errno));
    }
}

int main(void) {
    ensure_dir("/proc");
    ensure_dir("/sys");
    ensure_dir("/dev");

    // Best-effort mounts; on some kernels, devtmpfs might not be enabled.
    try_mount("proc", "/proc", "proc", 0);
    try_mount("sysfs", "/sys", "sysfs", 0);
    try_mount("devtmpfs", "/dev", "devtmpfs", 0);

    char *const argv[] = {(char *)"/calc_os", NULL};
    execv("/calc_os", argv);

    (void)fprintf(stderr, "init: execv(/calc_os) failed: %s\n", strerror(errno));

    // Avoid exiting PID 1; sleep forever.
    for (;;) {
        (void)sleep(3600);
    }
}

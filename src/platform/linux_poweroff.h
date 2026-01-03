#pragma once

/* Attempts to power off the machine if running as PID 1.
   Safe on normal Linux usage because PID!=1.

   Returns 0 if a shutdown was attempted (even if it fails),
   returns -1 if not PID 1 (no attempt made).
*/
int linux_poweroff_if_pid1(void);

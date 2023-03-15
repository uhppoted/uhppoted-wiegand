#pragma once

extern void sys_start();
extern void sys_ok();
extern bool is_valid(datetime_t);
extern bool is_valid(datetime_t);

extern void sys_settime(char *);
extern bool watchdog(repeating_timer_t *);
extern bool syscheck(repeating_timer_t *rt);
extern int64_t startup(alarm_id_t, void *);

extern void setup_uart();
extern void setup_usb();

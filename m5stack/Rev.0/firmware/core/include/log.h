#pragma once

extern void log_init();

extern void debugf(const char *tag, const char *fmt, ...);
extern void infof(const char *tag, const char *fmt, ...);
extern void warnf(const char *tag, const char *fmt, ...);
extern void errorf(const char *tag, const char *fmt, ...);

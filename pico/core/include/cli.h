#pragma once

typedef void (*txrx)(void *, const char *);

// CLI functions
void reboot();
void cli_set_time(char *, txrx, void *);
void keypad(char *, txrx, void *);
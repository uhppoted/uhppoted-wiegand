#pragma once

typedef void (*txrx)(void *, const char *);

// CLI functions
void cli_reboot(txrx, void *);
void cli_set_time(char *, txrx, void *);
void cli_blink(txrx, void *);
void cli_unlock_door(txrx, void *);
void keypad(char *, txrx, void *);
void set_passcodes(char *, txrx, void *);
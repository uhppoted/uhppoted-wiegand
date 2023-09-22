#pragma once

typedef void (*txrx)(void *, const char *);

// CLI functions
void reboot();
void keypad(char *cmd, txrx, void *);
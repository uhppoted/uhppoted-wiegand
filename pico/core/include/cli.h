#pragma once

typedef void (*txrx)(void *, const char *);

// CLI functions
void cli_reboot(txrx, void *);
void cli_set_time(char *, txrx, void *);
void cli_blink(txrx, void *);
void cli_unlock_door(txrx, void *);
void cli_list_acl(txrx, void *);
void cli_clear_acl(txrx, void *);
void cli_write_acl(txrx, void *);
void cli_set_passcodes(char *, txrx, void *);
void keypad(char *, txrx, void *);

#pragma once

typedef void (*txrx)(void *, const char *);

void cli_reboot(txrx, void *);
void cli_set_time(char *, txrx, void *);
void cli_blink(txrx, void *);
void cli_unlock_door(txrx, void *);
void cli_on_door_open(txrx, void *);
void cli_on_door_close(txrx, void *);
void cli_acl_list(txrx, void *);
void cli_acl_clear(txrx, void *);
void cli_acl_grant(char *, txrx, void *);
void cli_acl_write(txrx, void *);
void cli_set_passcodes(char *, txrx, void *);

void cli_swipe(char *, txrx f, void *);
void keypad(char *, txrx, void *);

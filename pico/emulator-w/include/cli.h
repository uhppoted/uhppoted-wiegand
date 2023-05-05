#pragma once

typedef void (*txrx)(void *, const char *);

extern void exec(char *);
extern void execw(char *, txrx, void *);

#pragma once

typedef void (*txrx)(void *, const char *);

extern void clear_screen();
extern void set_scroll_area();
extern void rx(char *);
extern void tx(char *);

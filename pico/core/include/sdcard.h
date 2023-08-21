#pragma once

#include "wiegand.h"

void sdcard_initialise(enum MODE, bool);

int sdcard_format();
int sdcard_mount();
int sdcard_unmount();
int sdcard_read_acl(CARD[], int *);
int sdcard_write_acl(CARD[], int);

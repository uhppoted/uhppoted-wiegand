#pragma once

#include "wiegand.h"

void sdcard_initialise(enum MODE mode);

int sdcard_format();
int sdcard_mount();
int sdcard_unmount();

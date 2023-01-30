#pragma once

#include <stdbool.h>
#include <stdint.h>

extern void acl_initialise(uint32_t cards[], int N);
extern bool acl_grant(uint32_t, uint32_t);
extern bool acl_revoke(uint32_t, uint32_t);
extern bool acl_allowed(uint32_t, uint32_t);
extern int acl_list(uint32_t *[]);

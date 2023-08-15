#pragma once

#include <stdbool.h>
#include <stdint.h>

extern void acl_initialise();
extern void acl_load(char *, int);
extern void acl_save(char *, int);
extern bool acl_grant(uint32_t, uint32_t);
extern bool acl_revoke(uint32_t, uint32_t);
extern bool acl_allowed(uint32_t, uint32_t);
extern int acl_list(uint32_t *[]);

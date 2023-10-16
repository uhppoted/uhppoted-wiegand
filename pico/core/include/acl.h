#pragma once

#include <stdbool.h>
#include <stdint.h>

extern void acl_initialise();
extern int acl_load();
extern int acl_save();

extern int acl_list(uint32_t *[]);
extern bool acl_clear();
extern bool acl_grant(uint32_t, uint32_t, const char *);
extern bool acl_revoke(uint32_t, uint32_t);
extern enum ACCESS acl_allowed(uint32_t, uint32_t, const char *);

extern bool acl_set_passcodes(uint32_t, uint32_t, uint32_t, uint32_t);
extern bool acl_passcode(const char *);

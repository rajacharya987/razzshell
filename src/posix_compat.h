#ifndef POSIX_COMPAT_H
#define POSIX_COMPAT_H

#include "shell_config.h"

// POSIX command alias structure
typedef struct {
    const char *posix_name;
    const char *razz_name;
} PosixAlias;

// Function prototypes
const char* posix_translate_command(const char *cmd);
void posix_init_aliases(void);
int posix_is_standard_command(const char *cmd);

#endif // POSIX_COMPAT_H

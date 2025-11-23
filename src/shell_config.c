#include "shell_config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Global shell configuration instance
ShellConfig shell_config;

// Initialize shell configuration with defaults
void shell_config_init(void) {
    shell_config.mode = MODE_RAZZSHELL;
    shell_config.strict_posix = 0;
    shell_config.errexit = 0;
    shell_config.pipefail = 0;
    shell_config.nounset = 0;
    shell_config.verbose = 0;
    shell_config.xtrace = 0;
}

// Set shell execution mode
void shell_set_mode(ShellMode mode) {
    shell_config.mode = mode;
    
    // Apply mode-specific defaults
    switch (mode) {
        case MODE_POSIX:
            shell_config.strict_posix = 1;
            printf("Switched to POSIX mode\n");
            break;
        case MODE_BASH:
            shell_config.strict_posix = 0;
            printf("Switched to Bash compatibility mode\n");
            break;
        case MODE_RAZZSHELL:
        default:
            shell_config.strict_posix = 0;
            printf("Switched to RazzShell native mode\n");
            break;
    }
}

// Get current shell mode
ShellMode shell_get_mode(void) {
    return shell_config.mode;
}

// Apply set options (set -e, set -o pipefail, etc.)
void shell_apply_set_options(const char *options) {
    if (!options) return;
    
    // Handle set -e (errexit)
    if (strchr(options, 'e')) {
        shell_config.errexit = 1;
        printf("errexit enabled: shell will exit on command failure\n");
    }
    
    // Handle set -u (nounset)
    if (strchr(options, 'u')) {
        shell_config.nounset = 1;
        printf("nounset enabled: error on unset variables\n");
    }
    
    // Handle set -v (verbose)
    if (strchr(options, 'v')) {
        shell_config.verbose = 1;
        printf("verbose mode enabled\n");
    }
    
    // Handle set -x (xtrace)
    if (strchr(options, 'x')) {
        shell_config.xtrace = 1;
        printf("xtrace enabled: commands will be printed before execution\n");
    }
    
    // Handle set +e (disable errexit)
    if (strstr(options, "+e")) {
        shell_config.errexit = 0;
        printf("errexit disabled\n");
    }
    
    // Handle set +u (disable nounset)
    if (strstr(options, "+u")) {
        shell_config.nounset = 0;
        printf("nounset disabled\n");
    }
}

// Get mode name as string
const char* shell_mode_name(ShellMode mode) {
    switch (mode) {
        case MODE_POSIX:
            return "POSIX";
        case MODE_BASH:
            return "Bash";
        case MODE_RAZZSHELL:
        default:
            return "RazzShell";
    }
}

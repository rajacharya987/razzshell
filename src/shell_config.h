#ifndef SHELL_CONFIG_H
#define SHELL_CONFIG_H

// Shell execution modes
typedef enum {
    MODE_RAZZSHELL,  // Native RazzShell mode (default)
    MODE_POSIX,      // POSIX-compliant mode
    MODE_BASH        // Bash-compatible mode
} ShellMode;

// Shell configuration structure
typedef struct {
    ShellMode mode;
    int strict_posix;    // Strict POSIX compliance
    int errexit;         // set -e: exit on error
    int pipefail;        // set -o pipefail: pipeline failure detection
    int nounset;         // set -u: error on unset variables
    int verbose;         // set -v: verbose mode
    int xtrace;          // set -x: trace execution
} ShellConfig;

// Global shell configuration
extern ShellConfig shell_config;

// Function prototypes
void shell_config_init(void);
void shell_set_mode(ShellMode mode);
ShellMode shell_get_mode(void);
void shell_apply_set_options(const char *options);
const char* shell_mode_name(ShellMode mode);

#endif // SHELL_CONFIG_H

#include "posix_compat.h"
#include <string.h>
#include <stdio.h>

// POSIX to RazzShell command mapping
static PosixAlias posix_aliases[] = {
    {"cd", "change"},
    {"ls", "list"},
    {"echo", "say"},
    {"pwd", "where"},
    {"cat", "readfile"},
    {"cp", "copy"},
    {"mv", "move"},
    {"rm", "delete"},
    {"mkdir", "makedir"},
    {"rmdir", "removedir"},
    {"chmod", "setperm"},
    {"chown", "setowner"},
    {"grep", "searchtext"},
    {"find", "searchfile"},
    {"touch", "create"},
    {"ps", "showprocesses"},
    {"whoami", "whome"},
    {"ping", "pinghost"},
    {"curl", "fetchurl"},
    {"df", "diskfree"},
    {"du", "diskuse"},
    {"uname", "systemname"},
    {"head", "headfile"},
    {"tail", "tailfile"},
    {"wc", "wordcount"},
    {"date", "today"},
    {"cal", "calendar"},
    {"clear", "clear"},
    {"history", "commands"},
    {"alias", "makealias"},
    {"unalias", "removealias"},
    {"export", "setenv"},
    {"unset", "unsetenv"},
    {"printenv", "printenv"},
    {"env", "printenv"},
    {"exit", "quit"},
    {"jobs", "viewjobs"},
    {"fg", "bringtofront"},
    {"bg", "sendtoback"},
    {"kill", "terminate"},
    {NULL, NULL}  // Sentinel
};

// Initialize POSIX aliases
void posix_init_aliases(void) {
    // Currently just a placeholder
    // In the future, this could load additional aliases from config
}

// Translate POSIX command to RazzShell command
const char* posix_translate_command(const char *cmd) {
    // Only translate in POSIX or Bash mode
    ShellMode mode = shell_get_mode();
    if (mode != MODE_POSIX && mode != MODE_BASH) {
        return cmd;  // No translation in native mode
    }
    
    // Search for POSIX alias
    for (int i = 0; posix_aliases[i].posix_name != NULL; i++) {
        if (strcmp(cmd, posix_aliases[i].posix_name) == 0) {
            return posix_aliases[i].razz_name;
        }
    }
    
    // No translation found, return original
    return cmd;
}

// Check if command is a standard POSIX command
int posix_is_standard_command(const char *cmd) {
    for (int i = 0; posix_aliases[i].posix_name != NULL; i++) {
        if (strcmp(cmd, posix_aliases[i].posix_name) == 0) {
            return 1;
        }
    }
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pwd.h>
#include <time.h>
#include <sys/utsname.h>
#include <grp.h>
#include <termios.h>
#include <dlfcn.h>
#include <linux/limits.h> // For PATH_MAX
#include <sys/select.h>
#include <sys/time.h>
#include <strings.h>
#include <ctype.h>

// RazzShell modernization includes
#include "src/shell_config.h"
#include "src/posix_compat.h"

#define MAX_ARGS 128
#define MAX_JOBS 100
#define MAX_HISTORY 1000
#define MAX_BOOKMARKS 100
#define MAX_ALIASES 100

// Color codes
#define RESET_COLOR   "\x1b[0m"
#define GREEN_COLOR   "\x1b[38;5;46m"    // Bright neon green
#define BLUE_COLOR    "\x1b[38;5;33m"    // Bright cyan-blue
#define CYAN_COLOR    "\x1b[38;5;51m"    // Electric cyan
#define RED_COLOR     "\x1b[38;5;196m"   // Bright red
#define YELLOW_COLOR  "\x1b[38;5;226m"   // Bright yellow
#define MAGENTA_COLOR "\x1b[38;5;201m"   // Hot pink
#define PURPLE_COLOR  "\x1b[38;5;93m"    // Deep purple
#define ORANGE_COLOR  "\x1b[38;5;208m"   // Bright orange

// Style codes
#define BOLD_TEXT      "\x1b[1m"
#define UNDERLINE_TEXT "\x1b[4m"
#define BLINK_TEXT     "\x1b[5m"
#define DIM_TEXT       "\x1b[2m"
#define ITALIC_TEXT    "\x1b[3m"

// Background colors
#define BG_BLACK   "\x1b[40m"
#define BG_BLUE    "\x1b[44m"
#define BG_CYBER   "\x1b[48;5;17m"  // Dark blue background

// Shell styling
#define PROMPT_STYLE    BOLD_TEXT CYAN_COLOR
#define ERROR_STYLE     BOLD_TEXT RED_COLOR
#define SUCCESS_STYLE   BOLD_TEXT GREEN_COLOR
#define WARNING_STYLE   BOLD_TEXT YELLOW_COLOR
#define INFO_STYLE      BOLD_TEXT BLUE_COLOR
#define CYBER_STYLE     BOLD_TEXT PURPLE_COLOR BG_CYBER

// ASCII art frames
#define TOP_BORDER    "╔════════════════════════════════════╗"
#define BOTTOM_BORDER "╚════════════════════════════════════╝"
#define SIDE_BORDER   "║"

// Enhanced color gradients for cyber theme
#define NEON_CYAN    "\x1b[38;5;51m"
#define NEON_BLUE    "\x1b[38;5;33m"
#define NEON_GREEN   "\x1b[38;5;46m"
#define NEON_PINK    "\x1b[38;5;198m"
#define NEON_PURPLE  "\x1b[38;5;141m"
#define NEON_YELLOW  "\x1b[38;5;226m"
#define NEON_ORANGE  "\x1b[38;5;214m"
#define NEON_RED     "\x1b[38;5;196m"

// Gradient backgrounds
#define BG_DARK_BLUE  "\x1b[48;5;17m"
#define BG_CYBER_ALT  "\x1b[48;5;23m"
#define BG_CYBER_DIM  "\x1b[48;5;16m"

// Job structure to manage background jobs
typedef struct {
    int id;
    pid_t pid;
    char command[256];
    int is_background;
} Job;

// Plugin structure
typedef struct {
    char *name;
    void *handle;
    int (*command_func)(char **args);
} Plugin;

Plugin plugins[MAX_ALIASES]; // Reuse the MAX_ALIASES constant for simplicity
int plugin_count = 0;
static char *ai_api_key = NULL;
static char *ai_model = NULL;

// Alias structure
typedef struct {
    char *alias_name;
    char *command;
} Alias;

Job jobs[MAX_JOBS];
int job_count = 0;
char *history[MAX_HISTORY];
int history_count = 0;
char *bookmarks[MAX_BOOKMARKS];
int bookmark_count = 0;
Alias aliases[MAX_ALIASES];
int alias_count = 0;

static char *safe_strdup(const char *value) {
    if (!value) {
        return NULL;
    }

    char *copy = strdup(value);
    if (!copy) {
        fprintf(stderr, ERROR_STYLE "Memory allocation failed\n" RESET_COLOR);
    }
    return copy;
}

static char *escape_json_string(const char *input) {
    if (!input) {
        return safe_strdup("");
    }

    size_t length = 0;
    for (const unsigned char *ptr = (const unsigned char *)input; *ptr; ptr++) {
        switch (*ptr) {
            case '\\':
            case '"':
                length += 2;
                break;
            case '\n':
            case '\r':
            case '\t':
                length += 2;
                break;
            default:
                if (iscntrl(*ptr)) {
                    length += 6;
                } else {
                    length += 1;
                }
        }
    }

    char *escaped = malloc(length + 1);
    if (!escaped) {
        fprintf(stderr, ERROR_STYLE "Memory allocation failed\n" RESET_COLOR);
        return NULL;
    }

    char *out = escaped;
    for (const unsigned char *ptr = (const unsigned char *)input; *ptr; ptr++) {
        switch (*ptr) {
            case '\\':
                *out++ = '\\';
                *out++ = '\\';
                break;
            case '"':
                *out++ = '\\';
                *out++ = '"';
                break;
            case '\n':
                *out++ = '\\';
                *out++ = 'n';
                break;
            case '\r':
                *out++ = '\\';
                *out++ = 'r';
                break;
            case '\t':
                *out++ = '\\';
                *out++ = 't';
                break;
            default:
                if (iscntrl(*ptr)) {
                    snprintf(out, 7, "\\u%04x", *ptr);
                    out += 6;
                } else {
                    *out++ = (char)*ptr;
                }
        }
    }
    *out = '\0';
    return escaped;
}

static char *join_args(char **args, int start_index) {
    size_t total = 0;
    int count = 0;
    for (int i = start_index; args[i] != NULL; i++) {
        total += strlen(args[i]) + 1;
        count++;
    }
    if (count == 0) {
        return safe_strdup("");
    }
    char *result = malloc(total);
    if (!result) {
        fprintf(stderr, ERROR_STYLE "Memory allocation failed\n" RESET_COLOR);
        return NULL;
    }
    result[0] = '\0';
    for (int i = start_index; args[i] != NULL; i++) {
        strcat(result, args[i]);
        if (args[i + 1] != NULL) {
            strcat(result, " ");
        }
    }
    return result;
}

// Shell environment setup
void setup_shell_env() {
    setenv("SHELL", "/usr/local/bin/razzshell", 1);
    setenv("RAZZSHELL_VERSION", "2.0.0", 1);
    setenv("RAZZSHELL_MODE", shell_mode_name(shell_get_mode()), 1);
}

// Forward declarations
char **razzshell_completion(const char *text, int start, int end);
char *command_generator(const char *text, int state);
char *get_command_name(int index);
void initialize_readline();
char *read_input_line();
char *get_prompt();

// Signal handling variables
struct termios shell_tmodes;
pid_t shell_pgid;

// Function prototypes for commands
int razz_change(char **args);       // cd
int razz_quit(char **args);         // exit
int razz_say(char **args);          // echo
int razz_where(char **args);        // pwd
int razz_viewjobs(char **args);     // jobs
int razz_bringtofront(char **args); // fg
int razz_sendtoback(char **args);   // bg
int razz_terminate(char **args);    // kill
int razz_list(char **args);         // ls
int razz_copy(char **args);         // cp
int razz_move(char **args);         // mv
int razz_delete(char **args);       // rm
int razz_searchfile(char **args);   // find
int razz_readfile(char **args);     // cat
int razz_searchtext(char **args);   // grep
int razz_commands(char **args);     // history
int razz_create(char **args);       // touch
int razz_makedir(char **args);      // mkdir
int razz_removedir(char **args);    // rmdir
int razz_setperm(char **args);      // chmod
int razz_setowner(char **args);     // chown
int razz_showprocesses(char **args); // ps
int razz_whome(char **args);        // whoami
int razz_pinghost(char **args);     // ping
int razz_fetchurl(char **args);     // curl
int razz_sudo(char **args);         // sudo
int razz_sudo_su(char **args);      // sudo su
int razz_save(char **args);         // save session
int razz_load(char **args);         // load session
int razz_bookmark(char **args);     // bookmark
int razz_listbookmarks(char **args);// list bookmarks
int razz_visualize(char **args);    // visualize command flow
int razz_sysinfo(char **args);      // system information
int razz_diskusage(char **args);    // disk usage
int razz_cpuusage(char **args);     // cpu usage
int razz_memusage(char **args);     // memory usage
int razz_howto(char **args);        // help
int razz_makealias(char **args);    // alias
int razz_removealias(char **args);  // unalias
int razz_setenv(char **args);       // set environment variable
int razz_printenv(char **args);     // print environment variables
int razz_clear(char **args);        // clear screen
int razz_today(char **args);        // date
int razz_calendar(char **args);     // cal
int razz_diskfree(char **args);     // df
int razz_diskuse(char **args);      // du
int razz_systemname(char **args);   // uname
int razz_headfile(char **args);     // head
int razz_tailfile(char **args);     // tail
int razz_wordcount(char **args);    // wc
int razz_aliases(char **args);      // list aliases
int razz_unsetenv(char **args);     // unset environment variable
int razz_repeat(char **args);       // repeat command
int razz_history_clear(char **args);// clear history
int razz_mkcd(char **args);         // mkdir + cd
int razz_which(char **args);        // which
int razz_setapi_ai(char **args);    // set AI API key and model
int razz_ai(char **args);           // AI command
int razz_loadplugin(char **args);   // Load a plugin
int razz_unloadplugin(char **args); // Unload a plugin
int razz_monitor(char **args);      // System resource monitor
int razz_matrix(char **args);       // Matrix-style text effect
int razz_sysart(char **args);       // System information with ASCII art
int razz_clock(char **args);        // Digital clock
int razz_fetch(char **args);        // RazzFetch (custom neofetch-style system info)
int razz_history_search(char **args); // Enhanced history search
int razz_mode(char **args);         // Switch shell mode
int razz_set(char **args);          // Set shell options (set -e, etc.)

// Command-to-function mapping
typedef struct {
    char *command_name;
    int (*command_func)(char **args);
    char *description;
} CommandMap;

CommandMap command_list[] = {
    {"change", razz_change, "Change directory"},                      // cd
    {"loadplugin", razz_loadplugin, "Load a plugin"},                 // loadplugin
    {"unloadplugin", razz_unloadplugin, "Unload a plugin"},           // unloadplugin
    {"quit", razz_quit, "Exit the shell"},                            // exit
    {"say", razz_say, "Display a line of text"},                      // echo
    {"where", razz_where, "Print working directory"},                 // pwd
    {"viewjobs", razz_viewjobs, "List active background jobs"},       // jobs
    {"bringtofront", razz_bringtofront, "Bring job to foreground"},   // fg
    {"sendtoback", razz_sendtoback, "Send job to background"},        // bg
    {"terminate", razz_terminate, "Terminate a process"},             // kill
    {"list", razz_list, "List directory contents"},                   // ls
    {"copy", razz_copy, "Copy files"},                                // cp
    {"move", razz_move, "Move/rename files"},                         // mv
    {"delete", razz_delete, "Delete files"},                          // rm
    {"searchfile", razz_searchfile, "Search for files"},              // find
    {"readfile", razz_readfile, "Display file contents"},             // cat
    {"searchtext", razz_searchtext, "Search text in files"},          // grep
    {"commands", razz_commands, "Show command history"},              // history
    {"create", razz_create, "Create a file"},                         // touch
    {"makedir", razz_makedir, "Create directory"},                    // mkdir
    {"mkcd", razz_mkcd, "Create a directory and switch into it"},
    {"removedir", razz_removedir, "Remove directory"},                // rmdir
    {"setperm", razz_setperm, "Change file permissions"},             // chmod
    {"setowner", razz_setowner, "Change file owner and group"},       // chown
    {"showprocesses", razz_showprocesses, "Show running processes"},  // ps
    {"whome", razz_whome, "Show current user"},                       // whoami
    {"pinghost", razz_pinghost, "Ping a host"},                       // ping
    {"fetchurl", razz_fetchurl, "Fetch URL"},                         // curl
    {"sudo", razz_sudo, "Run command as root"},                       // sudo
    {"sudo_su", razz_sudo_su, "Switch to root shell within razzshell"}, // sudo su
    {"save", razz_save, "Save current session"},                      // save session
    {"load", razz_load, "Load saved session"},                        // load session
    {"bookmark", razz_bookmark, "Bookmark a command"},                // bookmark
    {"listbookmarks", razz_listbookmarks, "List all bookmarks"},      // list bookmarks
    {"visualize", razz_visualize, "Visualize command flow"},          // visualize command flow
    {"sysinfo", razz_sysinfo, "Display system information"},          // system information
    {"diskusage", razz_diskusage, "Display disk usage"},              // disk usage
    {"cpuusage", razz_cpuusage, "Display CPU usage"},                 // cpu usage
    {"memusage", razz_memusage, "Display memory usage"},              // memory usage
    {"howto", razz_howto, "Show help for commands"},                  // help
    {"makealias", razz_makealias, "Create a command alias"},          // alias
    {"removealias", razz_removealias, "Remove a command alias"},      // unalias
    {"setenv", razz_setenv, "Set an environment variable"},           // set environment variable
    {"printenv", razz_printenv, "Print environment variables"},       // print environment variables
    {"clear", razz_clear, "Clear the terminal screen"},
    {"today", razz_today, "Display current date and time"},           // date
    {"calendar", razz_calendar, "Display calendar"},                  // cal
    {"diskfree", razz_diskfree, "Display free disk space"},           // df
    {"diskuse", razz_diskuse, "Estimate file space usage"},           // du
    {"systemname", razz_systemname, "Print system information"},      // uname
    {"headfile", razz_headfile, "Display first lines of a file"},     // head
    {"tailfile", razz_tailfile, "Display last lines of a file"},      // tail
    {"wordcount", razz_wordcount, "Count words in a file"},           // wc
    {"aliases", razz_aliases, "List all aliases"},                    // list aliases
    {"unsetenv", razz_unsetenv, "Unset an environment variable"},     // unset environment variable
    {"which", razz_which, "Locate a command in PATH or built-ins"},
    {"setapi-ai", razz_setapi_ai, "Set AI API key and model"},
    {"ai", razz_ai, "Query the configured AI model"},
    {"repeat", razz_repeat, "Repeat a command multiple times"},       // repeat command
    {"history_clear", razz_history_clear, "Clear command history"},   // clear history
    {"monitor", razz_monitor, "Show system resource monitor"},
    {"matrix", razz_matrix, "Display Matrix-style animation"},
    {"sysart", razz_sysart, "Show system information with ASCII art"},
    {"clock", razz_clock, "Show digital clock"},
    {"razzfetch", razz_fetch, "Display system information in RazzShell style"},
    {"hsearch", razz_history_search, "Search command history with highlighting"},
    {"mode", razz_mode, "Switch shell execution mode"},
    {"set", razz_set, "Set shell options (set -e, set -o pipefail, etc.)"},
    // Add additional commands here as per your list
};

// File type labels for list command
#define ICON_DIRECTORY "[DIR]"
#define ICON_FILE "[FILE]"
#define ICON_EXECUTABLE "[EXE]"
#define ICON_IMAGE "[IMG]"
#define ICON_VIDEO "[VID]"
#define ICON_AUDIO "[AUD]"
#define ICON_ARCHIVE "[ARC]"
#define ICON_TEXT "[TXT]"
#define ICON_PDF "[PDF]"
#define ICON_CONFIG "[CFG]"
#define ICON_LINK "[LNK]"

// Function to get file icon based on extension and permissions
const char* get_file_icon(const char* name, mode_t mode) {
    if (S_ISDIR(mode)) return ICON_DIRECTORY;
    if (S_ISLNK(mode)) return ICON_LINK;
    if (mode & S_IXUSR) return ICON_EXECUTABLE;

    // Get file extension
    const char* ext = strrchr(name, '.');
    if (!ext) return ICON_FILE;
    ext++; // Skip the dot

    // Common file extensions
    if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "png") == 0 || 
        strcasecmp(ext, "gif") == 0 || strcasecmp(ext, "bmp") == 0)
        return ICON_IMAGE;
    if (strcasecmp(ext, "mp4") == 0 || strcasecmp(ext, "avi") == 0 || 
        strcasecmp(ext, "mkv") == 0)
        return ICON_VIDEO;
    if (strcasecmp(ext, "mp3") == 0 || strcasecmp(ext, "wav") == 0 || 
        strcasecmp(ext, "flac") == 0)
        return ICON_AUDIO;
    if (strcasecmp(ext, "zip") == 0 || strcasecmp(ext, "tar") == 0 || 
        strcasecmp(ext, "gz") == 0)
        return ICON_ARCHIVE;
    if (strcasecmp(ext, "txt") == 0 || strcasecmp(ext, "md") == 0 || 
        strcasecmp(ext, "c") == 0 || strcasecmp(ext, "cpp") == 0 || 
        strcasecmp(ext, "py") == 0 || strcasecmp(ext, "js") == 0)
        return ICON_TEXT;
    if (strcasecmp(ext, "pdf") == 0)
        return ICON_PDF;
    if (strcasecmp(ext, "conf") == 0 || strcasecmp(ext, "config") == 0 || 
        strcasecmp(ext, "ini") == 0)
        return ICON_CONFIG;

    return ICON_FILE;
}

// Format size in human-readable format
char* format_size(off_t size) {
    static char buf[32];
    const char* units[] = {"B", "K", "M", "G", "T"};
    int unit = 0;
    double size_d = size;

    while (size_d >= 1024 && unit < 4) {
        size_d /= 1024;
        unit++;
    }

    if (unit == 0)
        sprintf(buf, "%ld%s", (long)size_d, units[unit]);
    else
        sprintf(buf, "%.1f%s", size_d, units[unit]);

    return buf;
}

// Format permissions in a readable format
char* format_permissions(mode_t mode) {
    static char perms[11];
    strcpy(perms, "----------");

    // File type
    if (S_ISDIR(mode)) perms[0] = 'd';
    else if (S_ISLNK(mode)) perms[0] = 'l';

    // User permissions
    if (mode & S_IRUSR) perms[1] = 'r';
    if (mode & S_IWUSR) perms[2] = 'w';
    if (mode & S_IXUSR) perms[3] = 'x';

    // Group permissions
    if (mode & S_IRGRP) perms[4] = 'r';
    if (mode & S_IWGRP) perms[5] = 'w';
    if (mode & S_IXGRP) perms[6] = 'x';

    // Others permissions
    if (mode & S_IROTH) perms[7] = 'r';
    if (mode & S_IWOTH) perms[8] = 'w';
    if (mode & S_IXOTH) perms[9] = 'x';

    return perms;
}

// Improved list command with better organization and visuals
int razz_list(char **args) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char path[PATH_MAX];
    char datestr[256];
    struct passwd *pw;
    struct group *gr;
    
    // Get target directory
    const char *target_dir = args[1] ? args[1] : ".";
    
    dir = opendir(target_dir);
    if (!dir) {
        printf(ERROR_STYLE "Error: Could not open directory %s\n" RESET_COLOR, target_dir);
        return 1;
    }

    // Print header with fancy border
    printf(CYBER_STYLE "\n╭──────────────────────────────────────────────────────────────╮\n");
    printf("│ " BOLD_TEXT "Directory Listing: %-43s" RESET_COLOR CYBER_STYLE "│\n", target_dir);
    printf("├──────────────────────────────────────────────────────────────┤\n" RESET_COLOR);
    
    printf(BOLD_TEXT "%-2s %-10s %-8s %-8s %-6s %-19s %s\n" RESET_COLOR,
           "", "Perms", "Owner", "Group", "Size", "Modified", "Name");
    printf(DIM_TEXT "%-2s %-10s %-8s %-8s %-6s %-19s %s\n" RESET_COLOR,
           "", "----------", "--------", "--------", "------", "-------------------", "--------------------");

    // Store entries for sorting
    typedef struct file_entry {
        char name[256];
        char perms[11];
        char owner[32];
        char group[32];
        char size[32];
        char date[64];
        char icon[8];
        int is_dir;
    } FileEntry;

    FileEntry entries[1024];
    int entry_count = 0;

    // Read directory entries
    while ((entry = readdir(dir)) != NULL && entry_count < 1024) {
        snprintf(path, sizeof(path), "%s/%s", target_dir, entry->d_name);
        
        if (lstat(path, &file_stat) < 0)
            continue;

        // Skip hidden files unless -a flag is present
        if (entry->d_name[0] == '.' && (!args[1] || strcmp(args[1], "-a") != 0))
            continue;

        // Get owner and group names
        pw = getpwuid(file_stat.st_uid);
        gr = getgrgid(file_stat.st_gid);

        // Format modification time
        strftime(datestr, sizeof(datestr), "%Y-%m-%d %H:%M", localtime(&file_stat.st_mtime));

        // Store entry information
        FileEntry *current = &entries[entry_count];
        strncpy(current->name, entry->d_name, sizeof(current->name) - 1);
        strncpy(current->perms, format_permissions(file_stat.st_mode), sizeof(current->perms) - 1);
        strncpy(current->owner, pw ? pw->pw_name : "unknown", sizeof(current->owner) - 1);
        strncpy(current->group, gr ? gr->gr_name : "unknown", sizeof(current->group) - 1);
        strncpy(current->size, format_size(file_stat.st_size), sizeof(current->size) - 1);
        strncpy(current->date, datestr, sizeof(current->date) - 1);
        strncpy(current->icon, get_file_icon(entry->d_name, file_stat.st_mode), sizeof(current->icon) - 1);
        current->is_dir = S_ISDIR(file_stat.st_mode);

        entry_count++;
    }

    // Sort entries (directories first, then files alphabetically)
    for (int i = 0; i < entry_count - 1; i++) {
        for (int j = 0; j < entry_count - i - 1; j++) {
            if ((entries[j].is_dir < entries[j + 1].is_dir) ||
                (entries[j].is_dir == entries[j + 1].is_dir && 
                 strcasecmp(entries[j].name, entries[j + 1].name) > 0)) {
                FileEntry temp = entries[j];
                entries[j] = entries[j + 1];
                entries[j + 1] = temp;
            }
        }
    }

    // Print entries with alternating background colors for better readability
    for (int i = 0; i < entry_count; i++) {
        const char *bg_color = (i % 2 == 0) ? "" : BG_CYBER;
        if (entries[i].is_dir) {
            printf("%s%-2s " BLUE_COLOR "%-10s %-8s %-8s %-6s %-19s %s%s" RESET_COLOR "\n",
                   bg_color, entries[i].icon, entries[i].perms, entries[i].owner,
                   entries[i].group, entries[i].size, entries[i].date, 
                   entries[i].name, S_ISLNK(file_stat.st_mode) ? " -> " : "");
        } else {
            printf("%s%-2s %-10s %-8s %-8s %-6s %-19s %s%s" RESET_COLOR "\n",
                   bg_color, entries[i].icon, entries[i].perms, entries[i].owner,
                   entries[i].group, entries[i].size, entries[i].date, 
                   entries[i].name, S_ISLNK(file_stat.st_mode) ? " -> " : "");
        }
    }

    // Print footer with summary
    printf(CYBER_STYLE "├──────────────────────────────────────────────────────────────┤\n");
    printf("│ " BOLD_TEXT "Total: %d items" RESET_COLOR CYBER_STYLE "%-43s│\n", entry_count, "");
    printf("╰──────────────────────────────────────────────────────────────╯\n" RESET_COLOR);
    
    closedir(dir);
    return 1;
}

// Enhanced RazzFetch with better visuals
int razz_fetch(char **args) {
    struct utsname sys_info;
    if (uname(&sys_info) == -1) {
        printf(ERROR_STYLE "Error getting system information\n" RESET_COLOR);
        return 1;
    }

    // Get username and hostname
    char hostname[1024];
    struct passwd *pw = getpwuid(getuid());
    gethostname(hostname, sizeof(hostname));

    // Get memory information
    long total_mem = 0, available_mem = 0;
    FILE *meminfo = fopen("/proc/meminfo", "r");
    if (meminfo) {
        char line[256];
        while (fgets(line, sizeof(line), meminfo)) {
            if (strncmp(line, "MemTotal:", 9) == 0)
                sscanf(line, "MemTotal: %ld", &total_mem);
            else if (strncmp(line, "MemAvailable:", 12) == 0)
                sscanf(line, "MemAvailable: %ld", &available_mem);
        }
        fclose(meminfo);
    }

    // Get CPU info
    char cpu_model[256] = "Unknown";
    FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
    if (cpuinfo) {
        char line[256];
        while (fgets(line, sizeof(line), cpuinfo)) {
            if (strncmp(line, "model name", 10) == 0) {
                char *colon = strchr(line, ':');
                if (colon) {
                    strncpy(cpu_model, colon + 2, sizeof(cpu_model) - 1);
                    char *newline = strchr(cpu_model, '\n');
                    if (newline) *newline = '\0';
                }
                break;
            }
        }
        fclose(cpuinfo);
    }

    // Get package count (pacman)
    int pacman_count = 0;
    FILE *pacman = popen("pacman -Q | wc -l", "r");
    if (pacman) {
        fscanf(pacman, "%d", &pacman_count);
        pclose(pacman);
    }

    // Get desktop environment
    char *desktop_env = getenv("XDG_CURRENT_DESKTOP");
    if (!desktop_env) desktop_env = "Unknown";

    // Get GPU info
    char gpu_info[256] = "Unknown";
    FILE *lspci = popen("lspci | grep -i vga | head -n1 | cut -d ':' -f3", "r");
    if (lspci) {
        fgets(gpu_info, sizeof(gpu_info), lspci);
        char *newline = strchr(gpu_info, '\n');
        if (newline) *newline = '\0';
        pclose(lspci);
    }

    // Get uptime
    long uptime = 0;
    FILE *uptime_file = fopen("/proc/uptime", "r");
    if (uptime_file) {
        fscanf(uptime_file, "%ld", &uptime);
        fclose(uptime_file);
    }
    int days = uptime / 86400;
    int hours = (uptime % 86400) / 3600;
    int minutes = (uptime % 3600) / 60;

    // Clear screen and move cursor to top
    printf("\033[2J\033[H");

    // Print RazzShell logo with gradient effect
    printf(NEON_CYAN "    ╭─────────────╮\n");
    printf("    │ " NEON_PINK "R" NEON_PURPLE "A" NEON_BLUE "Z" NEON_CYAN "Z" 
           NEON_GREEN "S" NEON_YELLOW "H" NEON_ORANGE "E" NEON_PINK "L" 
           NEON_PURPLE "L" NEON_CYAN " │\n");
    printf("    ╰─────────────╯\n\n");

    // Print system information in a fancy box
    printf(NEON_CYAN "╭────────────────────── " NEON_PINK "System Information" 
           NEON_CYAN " ──────────────────────╮\n");

    // User@Host with custom art
    printf("│ " NEON_BLUE "%s" NEON_PINK "@" NEON_BLUE "%s" NEON_CYAN "%*s│\n",
           pw->pw_name, hostname, 
           (int)(50 - strlen(pw->pw_name) - strlen(hostname) - 1), "");

    // Separator
    printf("├──────────────────────────────────────────────────────────────┤\n");

    // System information with custom styling and progress bars
    printf("│ " NEON_GREEN "OS        " RESET_COLOR "%-48s" NEON_CYAN "│\n", sys_info.sysname);
    printf("│ " NEON_BLUE "Shell     " RESET_COLOR "RazzShell v1.0.2%35s" NEON_CYAN "│\n", "");
    printf("│ " NEON_YELLOW "Kernel    " RESET_COLOR "%-48s" NEON_CYAN "│\n", sys_info.release);
    printf("│ " NEON_PINK "Packages  " RESET_COLOR "%d (pacman)%39s" NEON_CYAN "│\n", 
           pacman_count, "");
    printf("│ " NEON_PURPLE "DE        " RESET_COLOR "%-48s" NEON_CYAN "│\n", desktop_env);

    // Hardware information
    printf("├──────────────────────────────────────────────────────────────┤\n");
    printf("│ " NEON_ORANGE "CPU       " RESET_COLOR "%-48s" NEON_CYAN "│\n", cpu_model);
    printf("│ " NEON_GREEN "GPU       " RESET_COLOR "%-48s" NEON_CYAN "│\n", gpu_info);

    // Memory usage with progress bar
    int mem_percent = ((total_mem - available_mem) * 100) / total_mem;
    printf("│ " NEON_BLUE "Memory    " RESET_COLOR "[");
    for (int i = 0; i < 20; i++) {
        if (i < mem_percent / 5)
            printf(NEON_GREEN "█");
        else
            printf(DIM_TEXT "░" RESET_COLOR);
    }
    printf("] %.1f/%.1fG%*s" NEON_CYAN "│\n",
           (total_mem - available_mem) / 1048576.0,
           total_mem / 1048576.0,
           (int)(13 - (mem_percent >= 100 ? 3 : mem_percent >= 10 ? 2 : 1)), "");

    // Uptime
    printf("│ " NEON_PINK "Uptime    " RESET_COLOR "%d days, %d hours, %d mins%*s" NEON_CYAN "│\n",
           days, hours, minutes,
           (int)(27 - (days >= 100 ? 3 : days >= 10 ? 2 : 1) -
                 (hours >= 10 ? 2 : 1) - (minutes >= 10 ? 2 : 1)), "");

    // Color blocks
    printf("├──────────────────────────────────────────────────────────────┤\n");
    printf("│ ");
    for (int i = 0; i < 8; i++) printf(NEON_CYAN "█" NEON_BLUE "█" NEON_GREEN "█ ");
    printf(NEON_CYAN "│\n│ ");
    for (int i = 0; i < 8; i++) printf(NEON_PINK "█" NEON_PURPLE "█" NEON_YELLOW "█ ");
    printf(NEON_CYAN "│\n");

    printf("╰──────────────────────────────────────────────────────────────╯\n");
    printf(RESET_COLOR);

    return 1;
}

// Function to check for aliases
char* check_alias(char *cmd) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(cmd, aliases[i].alias_name) == 0) {
            return aliases[i].command;
        }
    }
    return cmd;
}

// Load a plugin
int razz_loadplugin(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: loadplugin [plugin_path]\n");
        return 1;
    }

    if (plugin_count >= MAX_ALIASES) {
        fprintf(stderr, "Plugin limit reached.\n");
        return 1;
    }

    void *handle = dlopen(args[1], RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error loading plugin: %s\n", dlerror());
        return 1;
    }

    int (*command_func)(char **args) = dlsym(handle, "plugin_command");
    char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "Error finding symbol: %s\n", error);
        dlclose(handle);
        return 1;
    }

    plugins[plugin_count].name = safe_strdup(args[1]);
    if (!plugins[plugin_count].name) {
        dlclose(handle);
        return 1;
    }
    plugins[plugin_count].handle = handle;
    plugins[plugin_count].command_func = command_func;
    plugin_count++;

    printf("Plugin '%s' loaded.\n", args[1]);
    return 1;
}

// Unload a plugin
int razz_unloadplugin(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: unloadplugin [plugin_name]\n");
        return 1;
    }

    for (int i = 0; i < plugin_count; i++) {
        if (strcmp(args[1], plugins[i].name) == 0) {
            dlclose(plugins[i].handle);
            free(plugins[i].name);
            for (int j = i; j < plugin_count - 1; j++) {
                plugins[j] = plugins[j + 1];
            }
            plugin_count--;
            printf("Plugin '%s' unloaded.\n", args[1]);
            return 1;
        }
    }

    fprintf(stderr, "Plugin '%s' not found.\n", args[1]);
    return 1;
}

// Generate the shell prompt
char* get_prompt() {
    char cwd[PATH_MAX];
    const char *fallback_dir = "?";
    if (!getcwd(cwd, sizeof(cwd))) {
        strncpy(cwd, fallback_dir, sizeof(cwd) - 1);
        cwd[sizeof(cwd) - 1] = '\0';
    }
    char *dir_name = strrchr(cwd, '/');
    if (dir_name != NULL) {
        dir_name++; // Move past '/'
    } else {
        dir_name = cwd; // Root directory
    }

    char *prompt = malloc(256);
    if (!prompt) {
        return NULL;
    }
    if (geteuid() == 0) {
        snprintf(prompt, 256, CYBER_STYLE TOP_BORDER "\n" SIDE_BORDER " razzshell-# [%s]> " RESET_COLOR, dir_name);
    } else {
        snprintf(prompt, 256, CYBER_STYLE TOP_BORDER "\n" SIDE_BORDER " razzshell-$ [%s]> " RESET_COLOR, dir_name);
    }
    return prompt;
}

// Helper function to remove a job from the jobs list
void remove_job(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            for (int j = i; j < job_count - 1; j++) {
                jobs[j] = jobs[j + 1];
            }
            job_count--;
            break;
        }
    }
}

// Signal handlers
void sigint_handler(int signo) {
    // Reset Readline state
    rl_replace_line("", 0);
    rl_on_new_line();
    printf("\n");
    rl_redisplay();
}

void sigtstp_handler(int sig) {
    // Ignore SIGTSTP
}

// Initialize Readline
void initialize_readline() {
    rl_readline_name = "razzshell";
    rl_attempted_completion_function = razzshell_completion;
}

// Command generator for completion
char *get_command_name(int index) {
    int built_in_count = sizeof(command_list) / sizeof(CommandMap);
    if (index < built_in_count) {
        return command_list[index].command_name;
    }
    index -= built_in_count;
    if (index < alias_count) {
        return aliases[index].alias_name;
    }
    index -= alias_count;
    if (index < plugin_count) {
        return plugins[index].name;
    }
    return NULL;
}

char *command_generator(const char *text, int state) {
    static int list_index, len;
    char *command_name;

    if (state == 0) {
        list_index = 0;
        len = strlen(text);
    }

    while ((command_name = get_command_name(list_index)) != NULL) {
        list_index++;
        if (strncmp(command_name, text, len) == 0) {
            return safe_strdup(command_name);
        }
    }
    return NULL;
}

char *read_input_line() {
    char *prompt = get_prompt();
    char *line = readline(prompt);
    free(prompt);
    if (line && *line) {
        add_history(line);
    }
    return line;
}

char *history_generator(const char *text, int state) {
    static int history_index;
    HIST_ENTRY **hist_list = history_list();

    if (!hist_list) {
        return NULL;
    }

    if (state == 0) {
        history_index = history_length;
    }

    while (--history_index >= 0) {
        char *cmd = hist_list[history_index]->line;
        if (strncmp(cmd, text, strlen(text)) == 0) {
            return safe_strdup(cmd);
        }
    }
    return NULL;
}

// Completion function
char **razzshell_completion(const char *text, int start, int end) {
    char **matches = NULL;

    // If this is the first word, offer command completions
    if (start == 0) {
        matches = rl_completion_matches(text, command_generator);
    } else {
        // For other words, perform default file completion
        rl_attempted_completion_over = 0;
        matches = NULL;
    }
    return matches;
}

// Function prototype for highlighting commands
void highlight_command(const char *cmd, const char *highlight);

// Command implementations
int razz_change(char **args) {
    if (args[1] == NULL) {
        chdir(getenv("HOME"));
    } else {
        if (chdir(args[1]) != 0) {
            perror("change");
        }
    }
    return 1;
}

int razz_quit(char **args) {
    return 0;  // Exit the shell
}

int razz_say(char **args) {
    for (int i = 1; args[i] != NULL; i++) {
        if (args[i][0] == '$') {
            char *env_var = getenv(args[i] + 1);
            if (env_var) {
                printf("%s ", env_var);
            }
        } else {
            printf("%s ", args[i]);
        }
    }
    printf("\n");
    return 1;
}

int razz_where(char **args) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("where");
    }
    return 1;
}

int razz_viewjobs(char **args) {
    for (int i = 0; i < job_count; i++) {
        printf("[%d] %d %s\n", jobs[i].id, jobs[i].pid, jobs[i].command);
    }
    return 1;
}

int razz_bringtofront(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: bringtofront [job id]\n");
    } else {
        int job_id = atoi(args[1]);
        for (int i = 0; i < job_count; i++) {
            if (jobs[i].id == job_id) {
                int status;
                tcsetpgrp(STDIN_FILENO, jobs[i].pid);
                kill(jobs[i].pid, SIGCONT);
                waitpid(jobs[i].pid, &status, WUNTRACED);
                tcsetpgrp(STDIN_FILENO, shell_pgid);
                remove_job(jobs[i].pid);
                return 1;
            }
        }
        fprintf(stderr, "bringtofront: no such job\n");
    }
    return 1;
}

int razz_sendtoback(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: sendtoback [job id]\n");
    } else {
        int job_id = atoi(args[1]);
        for (int i = 0; i < job_count; i++) {
            if (jobs[i].id == job_id) {
                kill(jobs[i].pid, SIGCONT);
                printf("Job [%d] %d sent to background\n", jobs[i].id, jobs[i].pid);
                return 1;
            }
        }
        fprintf(stderr, "sendtoback: no such job\n");
    }
    return 1;
}

int razz_terminate(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: terminate [process id]\n");
    } else {
        pid_t pid = atoi(args[1]);
        if (kill(pid, SIGTERM) == -1) {
            perror("terminate");
        } else {
            printf("Process %d terminated.\n", pid);
        }
    }
    return 1;
}

int razz_copy(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, "Usage: copy [source] [destination]\n");
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            execlp("cp", "cp", args[1], args[2], NULL);
            perror("copy");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork");
        }
    }
    return 1;
}

int razz_move(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, "Usage: move [source] [destination]\n");
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            execlp("mv", "mv", args[1], args[2], NULL);
            perror("move");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork");
        }
    }
    return 1;
}

int razz_delete(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: delete [file]\n");
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            execvp("rm", args);
            perror("delete");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork");
        }
    }
    return 1;
}

int razz_searchfile(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: searchfile [filename]\n");
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            execvp("find", args);
            perror("searchfile");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork");
        }
    }
    return 1;
}

int razz_readfile(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: readfile [filename]\n");
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            execvp("cat", args);
            perror("readfile");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork");
        }
    }
    return 1;
}

int razz_searchtext(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, "Usage: searchtext [pattern] [file]\n");
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            execvp("grep", args);
            perror("searchtext");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork");
        }
    }
    return 1;
}

int razz_commands(char **args) {
    for (int i = 0; i < history_count; i++) {
        printf("%d %s\n", i + 1, history[i]);
    }
    return 1;
}

int razz_create(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: create [filename]\n");
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            execvp("touch", args);
            perror("create");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork");
        }
    }
    return 1;
}

int razz_makedir(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: makedir [directory]\n");
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            execvp("mkdir", args);
            perror("makedir");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork");
        }
    }
    return 1;
}

int razz_mkcd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: mkcd [directory]\n");
        return 1;
    }

    if (mkdir(args[1], 0755) != 0) {
        if (errno != EEXIST) {
            perror("mkcd");
            return 1;
        }

        struct stat st;
        if (stat(args[1], &st) != 0 || !S_ISDIR(st.st_mode)) {
            fprintf(stderr, "mkcd: %s exists and is not a directory\n", args[1]);
            return 1;
        }
    }

    if (chdir(args[1]) != 0) {
        perror("mkcd");
        return 1;
    }

    return 1;
}

int razz_removedir(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: removedir [directory]\n");
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            execvp("rmdir", args);
            perror("removedir");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork");
        }
    }
    return 1;
}

int razz_which(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: which [-a] [command...]\n");
        return 1;
    }

    int show_all = 0;
    int start_index = 1;
    if (strcmp(args[1], "-a") == 0) {
        show_all = 1;
        start_index = 2;
    }

    if (args[start_index] == NULL) {
        fprintf(stderr, "Usage: which [-a] [command...]\n");
        return 1;
    }

    for (int arg_index = start_index; args[arg_index] != NULL; arg_index++) {
        const char *name = args[arg_index];
        int found = 0;

        for (int i = 0; i < (int)(sizeof(command_list) / sizeof(CommandMap)); i++) {
            if (strcmp(command_list[i].command_name, name) == 0) {
                printf("%s: built-in\n", name);
                found = 1;
                if (!show_all) {
                    break;
                }
            }
        }

        if (show_all || !found) {
            for (int i = 0; i < alias_count; i++) {
                if (strcmp(aliases[i].alias_name, name) == 0) {
                    printf("%s: alias -> %s\n", name, aliases[i].command);
                    found = 1;
                    if (!show_all) {
                        break;
                    }
                }
            }
        }

        if (show_all || !found) {
            for (int i = 0; i < plugin_count; i++) {
                if (strcmp(plugins[i].name, name) == 0) {
                    printf("%s: plugin\n", name);
                    found = 1;
                    if (!show_all) {
                        break;
                    }
                }
            }
        }

        if (show_all || !found) {
            const char *path_env = getenv("PATH");
            if (path_env) {
                char *path_copy = safe_strdup(path_env);
                if (!path_copy) {
                    return 1;
                }
                char *saveptr = NULL;
                char *dir = strtok_r(path_copy, ":", &saveptr);
                while (dir) {
                    char candidate[PATH_MAX];
                    snprintf(candidate, sizeof(candidate), "%s/%s", dir, name);
                    if (access(candidate, X_OK) == 0) {
                        printf("%s\n", candidate);
                        found = 1;
                        if (!show_all) {
                            break;
                        }
                    }
                    dir = strtok_r(NULL, ":", &saveptr);
                }
                free(path_copy);
            }
        }

        if (!found) {
            fprintf(stderr, "%s not found\n", name);
        }
    }

    return 1;
}

static int ai_model_is_valid(const char *model) {
    if (!model || *model == '\0') {
        return 0;
    }
    for (const char *ptr = model; *ptr; ptr++) {
        if (!(isalnum((unsigned char)*ptr) || *ptr == '-' || *ptr == '_' || *ptr == '.')) {
            return 0;
        }
    }
    return 1;
}

static const char *ai_default_model() {
    return "gemini-2.5-flash";
}

int razz_setapi_ai(char **args) {
    if (args[1] == NULL) {
        const char *model = ai_model ? ai_model : ai_default_model();
        if (ai_api_key) {
            size_t key_len = strlen(ai_api_key);
            const char *tail = key_len > 4 ? ai_api_key + key_len - 4 : ai_api_key;
            printf("AI key: ****%s\n", tail);
        } else {
            printf("AI key: (not set)\n");
        }
        printf("AI model: %s\n", model);
        printf("Usage: setapi-ai [api-key] [model]\n");
        printf("Example: setapi-ai sk-xxxx %s\n", ai_default_model());
        return 1;
    }

    if (strcmp(args[1], "--clear") == 0) {
        free(ai_api_key);
        ai_api_key = NULL;
        free(ai_model);
        ai_model = NULL;
        unsetenv("RAZZSHELL_AI_KEY");
        unsetenv("RAZZSHELL_AI_MODEL");
        printf("AI configuration cleared.\n");
        return 1;
    }

    if (args[2] && !ai_model_is_valid(args[2])) {
        fprintf(stderr, "Invalid model name. Use letters, numbers, '.', '-', '_'.\n");
        return 1;
    }

    free(ai_api_key);
    ai_api_key = safe_strdup(args[1]);
    if (!ai_api_key) {
        return 1;
    }

    free(ai_model);
    if (args[2]) {
        ai_model = safe_strdup(args[2]);
        if (!ai_model) {
            return 1;
        }
    } else {
        ai_model = safe_strdup(ai_default_model());
        if (!ai_model) {
            return 1;
        }
    }

    setenv("RAZZSHELL_AI_KEY", ai_api_key, 1);
    setenv("RAZZSHELL_AI_MODEL", ai_model, 1);
    printf("AI configuration updated. Model: %s\n", ai_model);
    return 1;
}

int razz_ai(char **args) {
    if (!ai_api_key) {
        const char *env_key = getenv("RAZZSHELL_AI_KEY");
        if (env_key) {
            ai_api_key = safe_strdup(env_key);
        }
    }

    if (!ai_model) {
        const char *env_model = getenv("RAZZSHELL_AI_MODEL");
        if (env_model && ai_model_is_valid(env_model)) {
            ai_model = safe_strdup(env_model);
        } else {
            ai_model = safe_strdup(ai_default_model());
        }
    }

    if (!ai_api_key) {
        fprintf(stderr, "AI key not set. Use: setapi-ai [api-key] [model]\n");
        return 1;
    }

    char *prompt = join_args(args, 1);
    if (!prompt) {
        return 1;
    }
    if (prompt[0] == '\0') {
        fprintf(stderr, "Usage: ai [prompt]\n");
        free(prompt);
        return 1;
    }

    char *escaped = escape_json_string(prompt);
    free(prompt);
    if (!escaped) {
        return 1;
    }

    char json_payload[8192];
    snprintf(json_payload, sizeof(json_payload),
             "{\"contents\":[{\"parts\":[{\"text\":\"%s\"}]}]}",
             escaped);
    free(escaped);

    char tmp_template[] = "/tmp/razzshell_ai_XXXXXX";
    int tmp_fd = mkstemp(tmp_template);
    if (tmp_fd < 0) {
        perror("mkstemp");
        return 1;
    }

    FILE *tmp_file = fdopen(tmp_fd, "w");
    if (!tmp_file) {
        perror("fdopen");
        close(tmp_fd);
        unlink(tmp_template);
        return 1;
    }

    fprintf(tmp_file, "%s", json_payload);
    fclose(tmp_file);

    char url[512];
    snprintf(url, sizeof(url),
             "https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s",
             ai_model, ai_api_key);

    char command[1024];
    snprintf(command, sizeof(command),
             "curl -s -H \"Content-Type: application/json\" -d @%s \"%s\"",
             tmp_template, url);

    FILE *pipe = popen(command, "r");
    if (!pipe) {
        perror("popen");
        unlink(tmp_template);
        return 1;
    }

    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        fputs(buffer, stdout);
    }

    int status = pclose(pipe);
    if (status == -1) {
        perror("pclose");
    }
    unlink(tmp_template);
    printf("\n");
    return 1;
}

int razz_setperm(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, "Usage: setperm [permissions] [file]\n");
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            execvp("chmod", args);
            perror("setperm");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork");
        }
    }
    return 1;
}

int razz_setowner(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, "Usage: setowner [owner] [file]\n");
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            execvp("chown", args);
            perror("setowner");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork");
        }
    }
    return 1;
}

int razz_showprocesses(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        execlp("ps", "ps", "-ef", NULL);
        perror("showprocesses");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
    return 1;
}

int razz_whome(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        execlp("whoami", "whoami", NULL);
        perror("whome");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
    return 1;
}

int razz_pinghost(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: pinghost [hostname]\n");
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            execvp("ping", args);
            perror("pinghost");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork");
        }
    }
    return 1;
}

int razz_fetchurl(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: fetchurl [URL]\n");
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            execvp("curl", args);
            perror("fetchurl");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork");
        }
    }
    return 1;
}

// Command: sudo (run command as root)
int razz_sudo(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: sudo [command]\n");
        return 1;
    }
    // Build the command array
    char *sudo_args[MAX_ARGS];
    sudo_args[0] = "sudo";
    int i;
    for (i = 1; args[i] != NULL && i < MAX_ARGS - 1; i++) {
        sudo_args[i] = args[i];
    }
    sudo_args[i] = NULL;

    pid_t pid = fork();
    if (pid == 0) {
        execvp("sudo", sudo_args);
        perror("sudo");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
    return 1;
}

// Command: sudo su (switch to root user shell within razzshell)
int razz_sudo_su(char **args) {
    if (geteuid() == 0) {
        printf("Already running as root.\n");
        return 1;
    }
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        char exe_path[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path)-1);
        if (len == -1) {
            perror("readlink");
            exit(EXIT_FAILURE);
        }
        exe_path[len] = '\0';
        execlp("sudo", "sudo", exe_path, NULL);
        perror("sudo_su");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Parent process
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
    return 1;
}

int razz_save(char **args) {
    FILE *file = fopen("session.save", "w");
    if (!file) {
        perror("save");
        return 1;
    }

    for (int i = 0; i < history_count; i++) {
        fprintf(file, "%s\n", history[i]);
    }

    fclose(file);
    printf("Session saved.\n");
    return 1;
}

int razz_load(char **args) {
    FILE *file = fopen("session.save", "r");
    if (!file) {
        perror("load");
        return 1;
    }

    char *line = NULL;
    size_t len = 0;

    printf("Session commands:\n");
    while (getline(&line, &len, file) != -1) {
        printf("%s", line);
    }

    fclose(file);
    if (line) free(line);
    printf("Session loaded.\n");
    return 1;
}

int razz_bookmark(char **args) {
    if (args[1] == NULL) {
        printf("Usage: bookmark [command]\n");
        return 1;
    }

    if (bookmark_count >= MAX_BOOKMARKS) {
        printf("Bookmark limit reached.\n");
        return 1;
    }

    char *command = safe_strdup(args[1]);
    if (!command) {
        return 1;
    }
    bookmarks[bookmark_count++] = command;
    printf("Command '%s' bookmarked!\n", command);
    return 1;
}

int razz_listbookmarks(char **args) {
    printf("Bookmarked Commands:\n");
    for (int i = 0; i < bookmark_count; i++) {
        printf("%d: %s\n", i + 1, bookmarks[i]);
    }
    return 1;
}

int razz_visualize(char **args) {
    if (args[1] == NULL) {
        printf("Usage: visualize [command]\n");
        return 1;
    }

    printf("Visualizing command flow for '%s':\n", args[1]);
    printf("|\n");
    printf("|---> Parse Command\n");
    printf("|\n");
    printf("|---> Check Syntax\n");
    printf("|\n");
    printf("|---> Execute Command\n");
    printf("|\n");
    printf("|---> Return Output\n");

    return 1;
}

int razz_sysinfo(char **args) {
    struct utsname sysinfo;
    if (uname(&sysinfo) == -1) {
        perror("sysinfo");
        return 1;
    }

    printf("System Information:\n");
    printf("  Sysname: %s\n", sysinfo.sysname);
    printf("  Nodename: %s\n", sysinfo.nodename);
    printf("  Release: %s\n", sysinfo.release);
    printf("  Version: %s\n", sysinfo.version);
    printf("  Machine: %s\n", sysinfo.machine);
    return 1;
}

int razz_diskusage(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        execlp("df", "df", "-h", NULL);
        perror("diskusage");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
    return 1;
}

int razz_cpuusage(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        execlp("top", "top", "-bn1", NULL);
        perror("cpuusage");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
    return 1;
}

int razz_memusage(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        execlp("free", "free", "-h", NULL);
        perror("memusage");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
    return 1;
}

int razz_howto(char **args) {
    printf("Available Commands:\n");
    for (int i = 0; i < sizeof(command_list) / sizeof(CommandMap); i++) {
        printf("%s - %s\n", command_list[i].command_name, command_list[i].description);
    }
    return 1;
}

int razz_makealias(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        printf("Usage: makealias [alias_name] [command]\n");
        return 1;
    }
    if (alias_count >= MAX_ALIASES) {
        printf("Alias limit reached.\n");
        return 1;
    }
    aliases[alias_count].alias_name = safe_strdup(args[1]);
    if (!aliases[alias_count].alias_name) {
        return 1;
    }
    aliases[alias_count].command = safe_strdup(args[2]);
    if (!aliases[alias_count].command) {
        free(aliases[alias_count].alias_name);
        aliases[alias_count].alias_name = NULL;
        return 1;
    }
    alias_count++;
    printf("Alias '%s' created for command '%s'.\n", args[1], args[2]);
    return 1;
}

int razz_removealias(char **args) {
    if (args[1] == NULL) {
        printf("Usage: removealias [alias_name]\n");
        return 1;
    }
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(args[1], aliases[i].alias_name) == 0) {
            free(aliases[i].alias_name);
            free(aliases[i].command);
            for (int j = i; j < alias_count - 1; j++) {
                aliases[j] = aliases[j + 1];
            }
            alias_count--;
            printf("Alias '%s' removed.\n", args[1]);
            return 1;
        }
    }
    printf("Alias '%s' not found.\n", args[1]);
    return 1;
}

int razz_aliases(char **args) {
    if (alias_count == 0) {
        printf("No aliases defined.\n");
        return 1;
    }
    printf("Current Aliases:\n");
    for (int i = 0; i < alias_count; i++) {
        printf("%s='%s'\n", aliases[i].alias_name, aliases[i].command);
    }
    return 1;
}

int razz_setenv(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        printf("Usage: setenv [VAR] [VALUE]\n");
        return 1;
    }
    setenv(args[1], args[2], 1);
    return 1;
}

int razz_unsetenv(char **args) {
    if (args[1] == NULL) {
        printf("Usage: unsetenv [VAR]\n");
        return 1;
    }
    unsetenv(args[1]);
    return 1;
}

int razz_printenv(char **args) {
    extern char **environ;
    for (char **env = environ; *env != 0; env++) {
        printf("%s\n", *env);
    }
    return 1;
}

int razz_clear(char **args) {
    // Clear screen and move cursor to home position
    printf("\033[2J\033[H");
    
    // Clear scrollback buffer (if terminal supports it)
    printf("\033[3J");
    
    // Force a complete refresh
    printf("\033[!p");
    fflush(stdout);
    
    return 1;
}

int razz_today(char **args) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("Current Date and Time: %s", asctime(&tm));
    return 1;
}

int razz_calendar(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        execlp("cal", "cal", NULL);
        perror("calendar");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
    return 1;
}

int razz_diskfree(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        execlp("df", "df", "-h", NULL);
        perror("diskfree");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
    return 1;
}

int razz_diskuse(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        execvp("du", args);
        perror("diskuse");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
    return 1;
}

int razz_systemname(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        execvp("uname", args);
        perror("systemname");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
    return 1;
}

int razz_headfile(char **args) {
    if (args[1] == NULL) {
        printf("Usage: headfile [filename]\n");
        return 1;
    }
    pid_t pid = fork();
    if (pid == 0) {
        execvp("head", args);
        perror("headfile");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
    return 1;
}

int razz_tailfile(char **args) {
    if (args[1] == NULL) {
        printf("Usage: tailfile [filename]\n");
        return 1;
    }
    pid_t pid = fork();
    if (pid == 0) {
        execvp("tail", args);
        perror("tailfile");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
    return 1;
}

int razz_wordcount(char **args) {
    if (args[1] == NULL) {
        printf("Usage: wordcount [filename]\n");
        return 1;
    }
    pid_t pid = fork();
    if (pid == 0) {
        execvp("wc", args);
        perror("wordcount");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
    return 1;
}

int razz_repeat(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        printf("Usage: repeat [count] [command]\n");
        return 1;
    }
    int count = atoi(args[1]);
    if (count <= 0) {
        printf("Invalid count.\n");
        return 1;
    }
    for (int i = 0; i < count; i++) {
        // Build command
        char *command = args[2];
        char *cmd_args[MAX_ARGS];
        cmd_args[0] = command;
        int idx = 1;
        for (int j = 3; args[j] != NULL && idx < MAX_ARGS - 1; j++) {
            cmd_args[idx++] = args[j];
        }
        cmd_args[idx] = NULL;

        // Execute command
        int found = 0;

        // Special handling for 'sudo su'
        if (strcmp(cmd_args[0], "sudo") == 0 && cmd_args[1] != NULL && strcmp(cmd_args[1], "su") == 0) {
            int status = razz_sudo_su(cmd_args);
            found = 1;
        } else {
            for (int k = 0; k < sizeof(command_list) / sizeof(CommandMap); k++) {
                if (strcmp(cmd_args[0], command_list[k].command_name) == 0) {
                    command_list[k].command_func(cmd_args);
                    found = 1;
                    break;
                }
            }
        }

        if (!found) {
            // Check for plugin commands
            for (int i = 0; i < plugin_count; i++) {
                if (strcmp(cmd_args[0], plugins[i].name) == 0) {
                    plugins[i].command_func(cmd_args);
                    found = 1;
                    break;
                }
            }
        }
        if (!found) {
            // Execute external command
            pid_t pid = fork();
            if (pid == 0) {
                // Put the child in its own process group
                setpgid(0, 0);

                // Take control of the terminal
                tcsetpgrp(STDIN_FILENO, getpid());

                // Restore default signal handlers
                signal(SIGINT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);

                execvp(cmd_args[0], cmd_args);
                printf(RED_COLOR "%s: command not found\n" RESET_COLOR, cmd_args[0]);
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                // Wait for the child process
                int child_status;
                waitpid(pid, &child_status, WUNTRACED);

                // Give control back to the shell
                tcsetpgrp(STDIN_FILENO, shell_pgid);
            } else {
                perror("fork");
            }
        }
    }
    return 1;
}

int razz_history_clear(char **args) {
    clear_history();
    history_count = 0;
    printf("Command history cleared.\n");
    return 1;
}

// New feature: System resource monitor
int razz_monitor(char **args) {
    int interval = 2; // Default update interval in seconds
    if (args[1]) {
        interval = atoi(args[1]);
        if (interval < 1) interval = 1;
    }

    printf(NEON_CYAN "╭───────────────────── " NEON_PINK "Process Monitor" NEON_CYAN " ─────────────────────╮\n" RESET_COLOR);
    
    while (1) {
        // Clear previous output
        printf("\033[H\033[J");
        
        // Get CPU usage
        double cpu_usage = 0.0;
        FILE *stat = fopen("/proc/stat", "r");
        if (stat) {
            unsigned long long user, nice, system, idle, iowait, irq, softirq;
            fscanf(stat, "cpu %llu %llu %llu %llu %llu %llu %llu",
                   &user, &nice, &system, &idle, &iowait, &irq, &softirq);
            fclose(stat);
            
            static unsigned long long prev_total = 0, prev_idle = 0;
            unsigned long long total = user + nice + system + idle + iowait + irq + softirq;
            unsigned long long diff_total = total - prev_total;
            unsigned long long diff_idle = idle - prev_idle;
            
            if (diff_total > 0)
                cpu_usage = 100.0 * (1.0 - (double)diff_idle / diff_total);
            
            prev_total = total;
            prev_idle = idle;
        }

        // Get memory usage
        unsigned long total_mem = 0, available_mem = 0;
        FILE *meminfo = fopen("/proc/meminfo", "r");
        if (meminfo) {
            char line[256];
            while (fgets(line, sizeof(line), meminfo)) {
                if (strncmp(line, "MemTotal:", 9) == 0)
                    sscanf(line, "MemTotal: %lu", &total_mem);
                else if (strncmp(line, "MemAvailable:", 12) == 0)
                    sscanf(line, "MemAvailable: %lu", &available_mem);
            }
            fclose(meminfo);
        }
        
        // Print header
        time_t now = time(NULL);
        char timestr[64];
        strftime(timestr, sizeof(timestr), "%H:%M:%S", localtime(&now));
        
        printf(NEON_CYAN "│ " NEON_YELLOW "System Monitor" NEON_CYAN " - Updated at: " 
               NEON_GREEN "%s" NEON_CYAN " │\n", timestr);
        printf("├────────────────────────────────────────────────────────┤\n" RESET_COLOR);

        // CPU Usage Bar
        printf(NEON_CYAN "│ " NEON_BLUE "CPU Usage  " RESET_COLOR "[");
        int cpu_bars = (int)(cpu_usage / 2.5);
        for (int i = 0; i < 40; i++) {
            if (i < cpu_bars)
                printf(NEON_GREEN "█");
            else
                printf(DIM_TEXT "░" RESET_COLOR);
        }
        printf("] %5.1f%%" NEON_CYAN " │\n", cpu_usage);

        // Memory Usage Bar
        double mem_used = (total_mem - available_mem) / 1024.0;
        double mem_total = total_mem / 1024.0;
        double mem_percent = 0.0;
        if (mem_total > 0.0) {
            mem_percent = (mem_used / mem_total) * 100;
        }
        
        printf(NEON_CYAN "│ " NEON_PINK "Memory    " RESET_COLOR "[");
        int mem_bars = (int)(mem_percent / 2.5);
        for (int i = 0; i < 40; i++) {
            if (i < mem_bars)
                printf(NEON_ORANGE "█");
            else
                printf(DIM_TEXT "░" RESET_COLOR);
        }
        printf("] %5.1f%%" NEON_CYAN " │\n", mem_percent);
        printf("│ " RESET_COLOR "          %.1f/%.1f GB Used" NEON_CYAN "%27s│\n", 
               mem_used/1024.0, mem_total/1024.0, "");

        // Top processes
        printf("├────────────────────────────────────────────────────────┤\n");
        printf("│ " NEON_GREEN "Top Processes by CPU Usage" NEON_CYAN "%29s│\n", "");
        printf("├────────────────────────────────────────────────────────┤\n");
        printf("│ " NEON_BLUE "PID   CPU%%   MEM%%   COMMAND" NEON_CYAN "%35s│\n", "");

        // Get top processes using ps command
        FILE *ps = popen("ps -eo pid,pcpu,pmem,comm --sort=-pcpu | head -n 6", "r");
        if (ps) {
            char line[256];
            int count = 0;
            // Skip header
            fgets(line, sizeof(line), ps);
            while (fgets(line, sizeof(line), ps) && count < 5) {
                int pid;
                float cpu, mem;
                char cmd[32];
                sscanf(line, "%d %f %f %s", &pid, &cpu, &mem, cmd);
                printf(NEON_CYAN "│ " RESET_COLOR "%5d %5.1f %6.1f   %-30s" NEON_CYAN "│\n",
                       pid, cpu, mem, cmd);
                count++;
            }
            pclose(ps);
        }

        printf("╰────────────────────────────────────────────────────────╯\n" RESET_COLOR);
        
        fflush(stdout);
        sleep(interval);

        // Check for input to exit
        struct timeval tv = {0, 0};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
            char c;
            read(STDIN_FILENO, &c, 1);
            if (c == 'q' || c == 'Q')
                break;
        }
    }

    // Clear screen when exiting
    printf("\033[2J\033[H");
    return 1;
}

// Enhanced history search with syntax highlighting
int razz_history_search(char **args) {
    HIST_ENTRY **history = history_list();
    if (!history) {
        printf(ERROR_STYLE "No command history available\n" RESET_COLOR);
        return 1;
    }

    char *query = args[1];
    int found = 0;

    printf(NEON_CYAN "╭─────────────────── " NEON_PINK "Command History Search" 
           NEON_CYAN " ───────────────────╮\n");

    if (query) {
        // Search mode
        printf("│ " NEON_YELLOW "Searching for: " RESET_COLOR "%-43s" NEON_CYAN "│\n", query);
        printf("├────────────────────────────────────────────────────────┤\n");
        
        for (int i = 0; history[i]; i++) {
            if (strcasestr(history[i]->line, query)) {
                highlight_command(history[i]->line, query);
                found++;
            }
        }
        
        if (!found) {
            printf("%s│ %sNo matches found for: %s%s%*s│\n", 
                   NEON_CYAN, NEON_RED, query, NEON_CYAN,
                   (int)(35 - strlen(query)), "");
        }
    } else {
        // Show recent history with line numbers
        printf("│ " NEON_YELLOW "Recent Commands" NEON_CYAN "%41s│\n", "");
        printf("├────────────────────────────────────────────────────────┤\n");
        
        int start = history_length - 10;
        if (start < 0) start = 0;
        
        for (int i = start; history[i]; i++) {
            printf("│ " NEON_GREEN "%3d" RESET_COLOR " │ ", i + 1);
            highlight_command(history[i]->line, NULL);
            found++;
        }
    }

    printf("╰────────────────────────────────────────────────────────╯\n");
    printf(RESET_COLOR "Type '!<number>' to execute a command or 'q' to quit\n");
    
    return 1;
}

// Syntax highlighting for commands
void highlight_command(const char *cmd, const char *highlight) {
    char *cmd_copy = safe_strdup(cmd);
    if (!cmd_copy) {
        int padding = 48 - (int)strlen(cmd);
        printf("%s", cmd);
        if (padding > 0) {
            printf("%*s", padding, "");
        }
        printf(NEON_CYAN "│\n");
        return;
    }
    char *token = strtok(cmd_copy, " ");
    int first = 1;
    int pos = 0;

    while (token) {
        // Add spacing
        if (!first) {
            printf(" ");
            pos++;
        }

        // Highlight matching text if searching
        if (highlight && strcasestr(token, highlight)) {
            printf(NEON_PINK "%s" RESET_COLOR, token);
        }
        // Highlight command name
        else if (first) {
            printf(NEON_BLUE "%s" RESET_COLOR, token);
        }
        // Highlight options
        else if (token[0] == '-') {
            printf(NEON_GREEN "%s" RESET_COLOR, token);
        }
        // Highlight paths
        else if (strchr(token, '/')) {
            printf(NEON_YELLOW "%s" RESET_COLOR, token);
        }
        // Regular text
        else {
            printf("%s", token);
        }

        pos += strlen(token);
        first = 0;
        token = strtok(NULL, " ");
    }

    // Pad to full width
    int padding = 48 - pos;
    if (padding > 0) {
        printf("%*s", padding, "");
    }
    printf(NEON_CYAN "│\n");

    free(cmd_copy);
}

// Command: mode (switch shell mode)
int razz_mode(char **args) {
    if (args[1] == NULL) {
        printf("Current mode: %s\n", shell_mode_name(shell_get_mode()));
        printf("\nAvailable modes:\n");
        printf("  razzshell  - Native RazzShell mode (custom commands)\n");
        printf("  posix      - POSIX-compliant mode (standard commands)\n");
        printf("  bash       - Bash-compatible mode\n");
        printf("\nUsage: mode [razzshell|posix|bash]\n");
        return 1;
    }
    
    if (strcmp(args[1], "razzshell") == 0) {
        shell_set_mode(MODE_RAZZSHELL);
    } else if (strcmp(args[1], "posix") == 0) {
        shell_set_mode(MODE_POSIX);
    } else if (strcmp(args[1], "bash") == 0) {
        shell_set_mode(MODE_BASH);
    } else {
        printf(ERROR_STYLE "Unknown mode: %s\n" RESET_COLOR, args[1]);
        printf("Valid modes: razzshell, posix, bash\n");
        return 1;
    }
    
    return 1;
}

// Command: set (set shell options)
int razz_set(char **args) {
    if (args[1] == NULL) {
        printf("Shell options:\n");
        printf("  errexit:   %s\n", shell_config.errexit ? "on" : "off");
        printf("  pipefail:  %s\n", shell_config.pipefail ? "on" : "off");
        printf("  nounset:   %s\n", shell_config.nounset ? "on" : "off");
        printf("  verbose:   %s\n", shell_config.verbose ? "on" : "off");
        printf("  xtrace:    %s\n", shell_config.xtrace ? "on" : "off");
        printf("\nUsage: set [-e] [-u] [-v] [-x] [-o pipefail]\n");
        return 1;
    }
    
    // Handle set -o pipefail
    if (strcmp(args[1], "-o") == 0 && args[2] != NULL) {
        if (strcmp(args[2], "pipefail") == 0) {
            shell_config.pipefail = 1;
            printf("pipefail enabled\n");
        } else {
            printf(ERROR_STYLE "Unknown option: %s\n" RESET_COLOR, args[2]);
        }
        return 1;
    }
    
    // Handle set +o pipefail
    if (strcmp(args[1], "+o") == 0 && args[2] != NULL) {
        if (strcmp(args[2], "pipefail") == 0) {
            shell_config.pipefail = 0;
            printf("pipefail disabled\n");
        }
        return 1;
    }
    
    // Handle other options
    shell_apply_set_options(args[1]);
    return 1;
}

// Initialize shell
void init_shell() {
    // Set up shell environment
    setup_shell_env();
    
    // Ignore interactive and job-control signals
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);
    signal(SIGTTOU, SIG_IGN);
    
    // Initialize job control
    shell_pgid = getpid();
    if (setpgid(shell_pgid, shell_pgid) < 0) {
        perror("Couldn't put the shell in its own process group");
        exit(1);
    }
    
    // Take control of the terminal
    tcsetpgrp(STDIN_FILENO, shell_pgid);
    tcgetattr(STDIN_FILENO, &shell_tmodes);
    
    // Initialize readline
    initialize_readline();
    
    // Clear screen and show welcome message
    printf("\033[2J\033[H"); // Clear screen and move cursor to top
    printf(NEON_CYAN
           "╔════════════════════════════════════════════════════════════════╗\n"
           "║                    Welcome to RazzShell v2.0.0                 ║\n"
           "║                    Mode: %-37s ║\n"
           "║                                                                ║\n"
           "║  Type 'howto' for help or 'razzfetch' for system information  ║\n"
           "╚════════════════════════════════════════════════════════════════╝\n"
           RESET_COLOR "\n", shell_mode_name(shell_get_mode()));
}

// Main shell loop
void razzshell_loop() {
    char *input;
    char **args;
    int status = 1;

    do {
        char *prompt = get_prompt();
        input = readline(prompt);
        free(prompt);

        if (!input) {
            // Handle EOF (Ctrl+D)
            printf("\n");
            break;
        }

        if (strlen(input) == 0) {
            free(input);
            continue;
        }

        // Add input to history
        add_history(input);
        if (history_count < MAX_HISTORY) {
            char *entry = safe_strdup(input);
            if (entry) {
                history[history_count++] = entry;
            }
        }

        // Tokenize input
        args = malloc(MAX_ARGS * sizeof(char *));
        int position = 0;
        char *token = strtok(input, " \t\r\n");

        while (token != NULL && position < MAX_ARGS - 1) {
            // First check for POSIX translation (if in POSIX/Bash mode)
            const char *translated = posix_translate_command(token);
            // Then check for user-defined aliases
            char *alias_cmd = check_alias((char*)translated);
            args[position++] = alias_cmd;
            token = strtok(NULL, " \t\r\n");
        }
        args[position] = NULL;

        if (args[0] != NULL) {
            int found = 0;

            // Special handling for 'sudo su'
            if (strcmp(args[0], "sudo") == 0 && args[1] != NULL && strcmp(args[1], "su") == 0) {
                status = razz_sudo_su(args);
                found = 1;
            } else {
                for (int i = 0; i < sizeof(command_list) / sizeof(CommandMap); i++) {
                    if (strcmp(args[0], command_list[i].command_name) == 0) {
                        status = command_list[i].command_func(args);
                        found = 1;
                        break;
                    }
                }
            }

            if (!found) {
                // Check for plugin commands
                for (int i = 0; i < plugin_count; i++) {
                    if (strcmp(args[0], plugins[i].name) == 0) {
                        status = plugins[i].command_func(args);
                        found = 1;
                        break;
                    }
                }
            }
            if (!found) {
                // Execute external command
                pid_t pid = fork();
                if (pid == 0) {
                    // Put the child in its own process group
                    setpgid(0, 0);

                    // Take control of the terminal
                    tcsetpgrp(STDIN_FILENO, getpid());

                    // Restore default signal handlers
                    signal(SIGINT, SIG_DFL);
                    signal(SIGTSTP, SIG_DFL);

                    execvp(args[0], args);
                    printf(RED_COLOR "%s: command not found\n" RESET_COLOR, args[0]);
                    exit(EXIT_FAILURE);
                } else if (pid > 0) {
                    // Wait for the child process
                    int child_status;
                    waitpid(pid, &child_status, WUNTRACED);

                    // Give control back to the shell
                    tcsetpgrp(STDIN_FILENO, shell_pgid);
                } else {
                    perror("fork");
                }
            }
        }

        free(input);
        free(args);
    } while (status);
}

int main(int argc, char **argv) {
    // Initialize shell configuration
    shell_config_init();
    posix_init_aliases();
    
    // Parse command-line arguments for mode selection
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--posix") == 0) {
            shell_set_mode(MODE_POSIX);
        } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--bash") == 0) {
            shell_set_mode(MODE_BASH);
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("RazzShell v2.0.0 - Modern Unix Shell\n\n");
            printf("Usage: razzshell [OPTIONS]\n\n");
            printf("Options:\n");
            printf("  --posix          Run in POSIX-compliant mode\n");
            printf("  -b, --bash       Run in Bash-compatible mode\n");
            printf("  -h, --help       Show this help message\n");
            printf("  --version        Show version information\n\n");
            printf("Default mode: RazzShell native mode\n");
            return EXIT_SUCCESS;
        } else if (strcmp(argv[i], "--version") == 0) {
            printf("RazzShell version 2.0.0\n");
            printf("Mode: %s\n", shell_mode_name(shell_get_mode()));
            return EXIT_SUCCESS;
        }
    }
    
    // Initialize shell
    init_shell();
    initialize_readline();
    rl_bind_key('\t', rl_complete);
    using_history();

    razzshell_loop();
    return EXIT_SUCCESS;
}

// Matrix-style text effect
int razz_matrix(char **args) {
    printf(CYBER_STYLE "\n%s\n" SIDE_BORDER " Matrix Effect\n%s\n" RESET_COLOR, TOP_BORDER, BOTTOM_BORDER);
    
    char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789@#$%^&*";
    int width = 80;
    int height = 20;
    
    // Clear screen
    printf("\033[2J\033[H");
    
    for (int frame = 0; frame < 100; frame++) {
        printf("\033[H"); // Move cursor to top
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (rand() % 20 == 0) {
                    printf(GREEN_COLOR "%c" RESET_COLOR, chars[rand() % strlen(chars)]);
                } else {
                    printf(" ");
                }
            }
            printf("\n");
        }
        usleep(50000); // Sleep for 50ms
    }
    
    return 1;
}

// System information with ASCII art
int razz_sysart(char **args) {
    struct utsname sys_info;
    if (uname(&sys_info) == -1) {
        printf(ERROR_STYLE "Error getting system information\n" RESET_COLOR);
        return 1;
    }

    printf(CYBER_STYLE
           "\n    ╭─────────────────────╮    "
           "\n    │  SYSTEM INFO       │    "
           "\n    ├─────────────────────┤    "
           "\n    │  OS: %-13s │    "
           "\n    │  HOST: %-11s │    "
           "\n    │  KERNEL: %-9s │    "
           "\n    │  ARCH: %-11s │    "
           "\n    ╰─────────────────────╯    "
           "\n" RESET_COLOR,
           sys_info.sysname,
           sys_info.nodename,
           sys_info.release,
           sys_info.machine);
    
    return 1;
}

// Clock with ASCII art
int razz_clock(char **args) {
    time_t now;
    struct tm *time_info;
    char time_str[9];
    
    time(&now);
    time_info = localtime(&now);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", time_info);
    
    printf(CYBER_STYLE
           "\n    ╭───────────────╮    "
           "\n    │   %s   │    "
           "\n    ╰───────────────╯    "
           "\n" RESET_COLOR,
           time_str);
    
    return 1;
}

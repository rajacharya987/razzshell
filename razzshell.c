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

#define MAX_ARGS 128
#define MAX_JOBS 100
#define MAX_HISTORY 1000
#define MAX_BOOKMARKS 100
#define MAX_ALIASES 100

// Color codes
#define RESET_COLOR   "\x1b[0m"
#define GREEN_COLOR   "\x1b[32m"
#define BLUE_COLOR    "\x1b[34m"
#define CYAN_COLOR    "\x1b[36m"
#define RED_COLOR     "\x1b[31m"
#define YELLOW_COLOR  "\x1b[33m"
#define MAGENTA_COLOR "\x1b[35m"

#define BOLD_TEXT      "\x1b[1m"
#define UNDERLINE_TEXT "\x1b[4m"

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
int razz_loadplugin(char **args);   // Load a plugin
int razz_unloadplugin(char **args); // Unload a plugin

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
    {"clear", razz_clear, "Clear the screen"},                        // clear screen
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
    {"repeat", razz_repeat, "Repeat a command multiple times"},       // repeat command
    {"history_clear", razz_history_clear, "Clear command history"},   // clear history
    // Add additional commands here as per your list
};

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

    plugins[plugin_count].name = strdup(args[1]);
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
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    char *dir_name = strrchr(cwd, '/');
    if (dir_name != NULL) {
        dir_name++; // Move past '/'
    } else {
        dir_name = cwd; // Root directory
    }

    char *prompt = malloc(256);
    if (geteuid() == 0) {
        snprintf(prompt, 256, "razzshell-# [%s]> ", dir_name);
    } else {
        snprintf(prompt, 256, "razzshell-$ [%s]> ", dir_name);
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
            return strdup(command_name);
        }
    }
    return NULL;
}
char *read_input_line() {
    char *line = readline(get_prompt());
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
            return strdup(cmd);
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

// Custom 'list' command with colorful output
int razz_list(char **args) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char path[1024];
    char *dirname;
    int show_all = 0; // Flag to show hidden files

    // Check for '-a' option
    if (args[1] != NULL && strcmp(args[1], "-a") == 0) {
        show_all = 1;
        dirname = (args[2] != NULL) ? args[2] : ".";
    } else {
        dirname = (args[1] != NULL) ? args[1] : ".";
    }

    dir = opendir(dirname);
    if (dir == NULL) {
        perror("list");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Skip hidden files if '-a' is not specified
        if (!show_all && entry->d_name[0] == '.') {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);
        if (lstat(path, &file_stat) == -1) {
            perror("lstat");
            continue;
        }

        if (S_ISDIR(file_stat.st_mode)) {
            printf(BLUE_COLOR "%s  " RESET_COLOR, entry->d_name);
        } else if (S_ISLNK(file_stat.st_mode)) {
            printf(CYAN_COLOR "%s  " RESET_COLOR, entry->d_name);
        } else if (file_stat.st_mode & S_IXUSR) {
            printf(GREEN_COLOR "%s  " RESET_COLOR, entry->d_name);
        } else if ((file_stat.st_mode & S_ISUID) || (file_stat.st_mode & S_ISGID)) {
            printf(RED_COLOR "%s  " RESET_COLOR, entry->d_name);
        } else {
            printf("%s  ", entry->d_name);
        }
    }
    printf("\n");
    closedir(dir);
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
        execvp("ps", args);
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

    char *command = strdup(args[1]);
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
    aliases[alias_count].alias_name = strdup(args[1]);
    aliases[alias_count].command = strdup(args[2]);
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
    printf("\033[H\033[J");
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
        for (int k = 0; k < sizeof(command_list) / sizeof(CommandMap); k++) {
            if (strcmp(cmd_args[0], command_list[k].command_name) == 0) {
                command_list[k].command_func(cmd_args);
                found = 1;
                break;
            }
        }
        if (!found) {
            // Execute external command
            pid_t pid = fork();
            if (pid == 0) {
                execvp(cmd_args[0], cmd_args);
                printf(RED_COLOR "%s: command not found\n" RESET_COLOR, cmd_args[0]);
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                waitpid(pid, NULL, 0);
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

// Initialize shell
void init_shell() {
    // Ignore interactive and job-control signals
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    // Put shell in its own process group
    shell_pgid = getpid();
    if (setpgid(shell_pgid, shell_pgid) < 0) {
        perror("Couldn't put the shell in its own process group");
        exit(1);
    }

    // Grab control of the terminal
    tcsetpgrp(STDIN_FILENO, shell_pgid);

    // Save default terminal attributes for shell
    tcgetattr(STDIN_FILENO, &shell_tmodes);
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
            history[history_count++] = strdup(input);
        }

        // Tokenize input
        args = malloc(MAX_ARGS * sizeof(char *));
        int position = 0;
        char *token = strtok(input, " \t\r\n");

        while (token != NULL && position < MAX_ARGS - 1) {
            // Check for aliases
            char *alias_cmd = check_alias(token);
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
                    int child_status;
                    // Wait for the child process
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
    // Initialize shell
    init_shell();
    initialize_readline();
    rl_bind_key('\t', rl_complete);
    using_history();

    razzshell_loop();
    return EXIT_SUCCESS;
}

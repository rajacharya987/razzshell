#include "undo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// Helper to get RazzShell directory paths
static void get_razz_paths(char *trash_dir, char *journal_path) {
    const char *home = getenv("HOME");
    if (!home) {
        home = getenv("USERPROFILE");
    }
    if (!home) {
        home = ".";
    }
    
    char base_dir[PATH_MAX];
    snprintf(base_dir, sizeof(base_dir), "%s/.razzshell", home);
    
    // Create base dir
    mkdir(base_dir, 0777);
    
    if (trash_dir) {
        snprintf(trash_dir, PATH_MAX, "%s/trash", base_dir);
        mkdir(trash_dir, 0777);
    }
    if (journal_path) {
        snprintf(journal_path, PATH_MAX, "%s/undo_journal.log", base_dir);
    }
}

void undo_init(void) {
    get_razz_paths(NULL, NULL);
}

// Append a line to the undo journal
static void append_journal(const char *entry) {
    char journal_path[PATH_MAX];
    get_razz_paths(NULL, journal_path);
    
    FILE *f = fopen(journal_path, "a");
    if (f) {
        fprintf(f, "%s\n", entry);
        fclose(f);
    }
}

void undo_log_delete(const char *orig_path, const char *trash_path) {
    char entry[PATH_MAX * 2 + 32];
    char abs_orig[PATH_MAX] = {0};
    char abs_trash[PATH_MAX] = {0};
    
    // Get absolute paths
    if (realpath(orig_path, abs_orig) == NULL) {
        strncpy(abs_orig, orig_path, PATH_MAX);
    }
    if (realpath(trash_path, abs_trash) == NULL) {
        strncpy(abs_trash, trash_path, PATH_MAX);
    }
    
    snprintf(entry, sizeof(entry), "DELETE:%s:%s", abs_orig, abs_trash);
    append_journal(entry);
}

void undo_log_move(const char *orig_path, const char *dest_path) {
    char entry[PATH_MAX * 2 + 32];
    char abs_orig[PATH_MAX] = {0};
    char abs_dest[PATH_MAX] = {0};
    
    if (realpath(orig_path, abs_orig) == NULL) {
        strncpy(abs_orig, orig_path, PATH_MAX);
    }
    // Dest might not exist if mv failed, but we log the target absolute path
    if (realpath(dest_path, abs_dest) == NULL) {
        strncpy(abs_dest, dest_path, PATH_MAX);
    }
    
    snprintf(entry, sizeof(entry), "MOVE:%s:%s", abs_orig, abs_dest);
    append_journal(entry);
}

void undo_log_copy(const char *dest_path) {
    char entry[PATH_MAX + 32];
    char abs_dest[PATH_MAX] = {0};
    
    if (realpath(dest_path, abs_dest) == NULL) {
        strncpy(abs_dest, dest_path, PATH_MAX);
    }
    
    snprintf(entry, sizeof(entry), "COPY:%s", abs_dest);
    append_journal(entry);
}

void undo_log_create(const char *path) {
    char entry[PATH_MAX + 32];
    char abs_path[PATH_MAX] = {0};
    
    if (realpath(path, abs_path) == NULL) {
        strncpy(abs_path, path, PATH_MAX);
    }
    
    snprintf(entry, sizeof(entry), "CREATE:%s", abs_path);
    append_journal(entry);
}

void undo_log_git(const char *git_cmd, const char *cwd) {
    char entry[PATH_MAX * 2];
    snprintf(entry, sizeof(entry), "GIT:%s:%s", git_cmd, cwd);
    append_journal(entry);
}

void undo_log_pkg(const char *pkg_mgr, const char *action, const char *package) {
    char entry[512];
    snprintf(entry, sizeof(entry), "PKG:%s:%s:%s", pkg_mgr, action, package);
    append_journal(entry);
}

// Recursively delete a directory (for undoing folder creation or copy)
static int remove_dir_recursive(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        return unlink(path); // Try deleting as file
    }
    
    struct dirent *entry;
    int err = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char subpath[PATH_MAX];
        snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);
        struct stat st;
        if (stat(subpath, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                err = remove_dir_recursive(subpath);
            } else {
                err = unlink(subpath);
            }
        }
        if (err != 0) break;
    }
    closedir(dir);
    if (err == 0) {
        err = rmdir(path);
    }
    return err;
}

// Built-in undo command
int razz_undo(char **args) {
    (void)args; // unused
    
    char journal_path[PATH_MAX];
    get_razz_paths(NULL, journal_path);
    
    // Read all lines to find the last line
    FILE *f = fopen(journal_path, "r");
    if (!f) {
        printf("Undo: No history found or nothing to undo.\n");
        return 1;
    }
    
    char lines[1000][512];
    int line_count = 0;
    while (fgets(lines[line_count], sizeof(lines[0]), f)) {
        // Strip trailing newline
        lines[line_count][strcspn(lines[line_count], "\r\n")] = '\0';
        if (strlen(lines[line_count]) > 0) {
            line_count++;
            if (line_count >= 1000) break;
        }
    }
    fclose(f);
    
    if (line_count == 0) {
        printf("Undo: Nothing to undo.\n");
        return 1;
    }
    
    char *last_line = lines[line_count - 1];
    printf("\033[1;36m🔄 Reverting last operation:\033[0m %s\n", last_line);
    
    // Parse the journal entry
    char *type = strtok(last_line, ":");
    if (strcmp(type, "DELETE") == 0) {
        char *orig_path = strtok(NULL, ":");
        char *trash_path = strtok(NULL, ":");
        if (orig_path && trash_path) {
            if (rename(trash_path, orig_path) == 0) {
                printf("\033[1;32m✓ Restored file/directory to '%s'\033[0m\n", orig_path);
            } else {
                perror("Undo delete failed");
            }
        }
    } else if (strcmp(type, "MOVE") == 0) {
        char *orig_path = strtok(NULL, ":");
        char *dest_path = strtok(NULL, ":");
        if (orig_path && dest_path) {
            if (rename(dest_path, orig_path) == 0) {
                printf("\033[1;32m✓ Moved back from '%s' to '%s'\033[0m\n", dest_path, orig_path);
            } else {
                perror("Undo move failed");
            }
        }
    } else if (strcmp(type, "COPY") == 0) {
        char *dest_path = strtok(NULL, ":");
        if (dest_path) {
            if (remove_dir_recursive(dest_path) == 0) {
                printf("\033[1;32m✓ Removed copied file/directory '%s'\033[0m\n", dest_path);
            } else {
                perror("Undo copy failed");
            }
        }
    } else if (strcmp(type, "CREATE") == 0) {
        char *path = strtok(NULL, ":");
        if (path) {
            if (remove_dir_recursive(path) == 0) {
                printf("\033[1;32m✓ Removed created file/directory '%s'\033[0m\n", path);
            } else {
                perror("Undo create failed");
            }
        }
    } else if (strcmp(type, "GIT") == 0) {
        char *git_cmd = strtok(NULL, ":");
        char *cwd = strtok(NULL, ":");
        if (git_cmd && cwd) {
            // Save old directory
            char old_cwd[PATH_MAX];
            getcwd(old_cwd, sizeof(old_cwd));
            chdir(cwd);
            
            // Revert git command
            if (strcmp(git_cmd, "commit") == 0) {
                printf("Running git reset --soft HEAD~1...\n");
                system("git reset --soft HEAD~1");
            } else if (strcmp(git_cmd, "add") == 0) {
                printf("Running git reset...\n");
                system("git reset");
            } else if (strcmp(git_cmd, "init") == 0) {
                printf("Removing .git directory...\n");
                remove_dir_recursive(".git");
            }
            
            chdir(old_cwd);
            printf("\033[1;32m✓ Reverted git operation\033[0m\n");
        }
    } else if (strcmp(type, "PKG") == 0) {
        char *pkg_mgr = strtok(NULL, ":");
        char *action = strtok(NULL, ":");
        char *package = strtok(NULL, ":");
        if (pkg_mgr && action && package) {
            char cmd[512];
            if (strcmp(pkg_mgr, "pacman") == 0 && strcmp(action, "install") == 0) {
                snprintf(cmd, sizeof(cmd), "pacman -Rns --noconfirm %s", package);
                printf("Running: %s...\n", cmd);
                system(cmd);
                printf("\033[1;32m✓ Uninstalled package %s\033[0m\n", package);
            }
        }
    }
    
    // Write lines back minus the last line
    f = fopen(journal_path, "w");
    if (f) {
        for (int i = 0; i < line_count - 1; i++) {
            // Reconstruct the line using raw content
            // Need to recover the ':' delimiters since strtok replaced them
            // Fortunately, we can write them back by joining the tokens or we can just save the raw line list before strtok
        }
        // To be safe, we read the file again and write back all but the last line
    }
    
    // Let's rewrite the file cleaner:
    // Read raw file, save lines in memory, write back all lines except the last
    f = fopen(journal_path, "r");
    if (f) {
        char file_lines[1000][512];
        int total = 0;
        while (fgets(file_lines[total], sizeof(file_lines[0]), f)) {
            total++;
            if (total >= 1000) break;
        }
        fclose(f);
        
        f = fopen(journal_path, "w");
        if (f) {
            for (int i = 0; i < total - 1; i++) {
                fprintf(f, "%s", file_lines[i]);
            }
            fclose(f);
        }
    }
    
    return 1;
}

// File count recursive helper
static int count_dir_files(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return 0;
    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        count++;
        if (count > 5000) break;
        
        char subpath[PATH_MAX];
        snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);
        struct stat st;
        if (stat(subpath, &st) == 0 && S_ISDIR(st.st_mode)) {
            count += count_dir_files(subpath);
            if (count > 5000) break;
        }
    }
    closedir(dir);
    return count;
}

int safe_delete_check(char **args) {
    int recursive = 0;
    for (int i = 1; args[i] != NULL; i++) {
        if (args[i][0] == '-') {
            for (int j = 1; args[i][j] != '\0'; j++) {
                if (args[i][j] == 'r' || args[i][j] == 'R') {
                    recursive = 1;
                    break;
                }
            }
        }
    }
    if (!recursive) return 1; // Safe
    
    int total_files = 0;
    for (int i = 1; args[i] != NULL; i++) {
        if (args[i][0] == '-') continue;
        
        if (strcmp(args[i], "/") == 0) {
            printf("\033[1;31m⚠️  WARNING: You are trying to delete the root directory '/'! This will render the system unusable.\033[0m\n");
            printf("Confirm deletion by typing 'DELETE_SYSTEM_NOW': ");
            char confirm[64] = {0};
            if (fgets(confirm, sizeof(confirm), stdin)) {
                if (strncmp(confirm, "DELETE_SYSTEM_NOW", 17) == 0) {
                    return 1;
                }
            }
            return 0;
        }
        
        struct stat st;
        if (stat(args[i], &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                total_files += count_dir_files(args[i]);
            } else {
                total_files++;
            }
        }
    }
    
    if (total_files > 100) {
        printf("\033[1;33m⚠️  Warning: This command will delete approximately %d files/directories.\033[0m\n", total_files);
        printf("Are you sure you want to continue? (y/N): ");
        char choice = getchar();
        if (choice != '\n' && choice != EOF) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }
        if (choice == 'y' || choice == 'Y') {
            return 1;
        } else {
            printf("Aborted.\n");
            return 0;
        }
    }
    return 1;
}

int move_to_trash(const char *orig_path, char *trash_path_out) {
    char trash_dir[PATH_MAX];
    get_razz_paths(trash_dir, NULL);
    
    // Resolve absolute path of original
    char abs_orig[PATH_MAX];
    if (realpath(orig_path, abs_orig) == NULL) {
        strncpy(abs_orig, orig_path, PATH_MAX);
    }
    
    // Check if it exists
    struct stat st;
    if (stat(abs_orig, &st) != 0) {
        return -1;
    }
    
    // Extract basename
    const char *base = strrchr(abs_orig, '/');
    if (!base) {
        base = strrchr(abs_orig, '\\');
    }
    if (!base) {
        base = abs_orig;
    } else {
        base++;
    }
    
    // Create unique trash path
    snprintf(trash_path_out, PATH_MAX, "%s/%ld_%s", trash_dir, (long)time(NULL), base);
    
    // Try simple rename first
    if (rename(abs_orig, trash_path_out) == 0) {
        undo_log_delete(abs_orig, trash_path_out);
        return 0;
    }
    
    // If rename failed (e.g. cross-device link), copy and remove
    char cmd[PATH_MAX * 2 + 32];
    snprintf(cmd, sizeof(cmd), "cp -r \"%s\" \"%s\" && rm -rf \"%s\"", abs_orig, trash_path_out, abs_orig);
    int status = system(cmd);
    if (status == 0) {
        undo_log_delete(abs_orig, trash_path_out);
        return 0;
    }
    
    return -1;
}

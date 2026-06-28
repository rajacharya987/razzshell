#ifndef UNDO_H
#define UNDO_H

// Initialize undo system (creates trash directory)
void undo_init(void);

// Log functions for reversible operations
void undo_log_delete(const char *orig_path, const char *trash_path);
void undo_log_move(const char *orig_path, const char *dest_path);
void undo_log_copy(const char *dest_path);
void undo_log_create(const char *path);
void undo_log_git(const char *git_cmd, const char *cwd);
void undo_log_pkg(const char *pkg_mgr, const char *action, const char *package);

// Move file/directory to trash and return 0 on success
int move_to_trash(const char *orig_path, char *trash_path_out);

// Built-in undo command
int razz_undo(char **args);

// Safe execution helper: checks recursive deletion of files
// Returns 1 if safe to proceed, 0 if aborted by user
int safe_delete_check(char **args);

#endif // UNDO_H

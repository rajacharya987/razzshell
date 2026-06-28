#ifndef OBJECT_PIPELINE_H
#define OBJECT_PIPELINE_H

// Command: processes
// Lists running processes in structured table format (PID NAME)
int razz_processes(char **args);

// Command: where
// Filters structured table stdin based on conditions (e.g., where name == python)
int object_where(char **args);

#endif // OBJECT_PIPELINE_H

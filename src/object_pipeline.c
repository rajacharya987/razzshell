#define _GNU_SOURCE
#include "object_pipeline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Trim whitespace from start and end
static char* trim(char *str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

int razz_processes(char **args) {
    (void)args; // unused
    
    // We use popen to run 'ps' and format its output
    FILE *fp = popen("ps -e 2>/dev/null || ps", "r");
    if (!fp) {
        perror("processes failed");
        return 1;
    }
    
    char line[512];
    printf("%-10s %-20s\n", "PID", "NAME");
    
    // Skip header line from ps output
    if (fgets(line, sizeof(line), fp) == NULL) {
        pclose(fp);
        return 1;
    }
    
    // Read each process and print PID and Command Name
    while (fgets(line, sizeof(line), fp)) {
        // ps output columns: PID TTY TIME CMD or similar
        // Let's tokenize by whitespace
        char *pid_str = strtok(line, " \t\r\n");
        if (!pid_str) continue;
        
        // Skip TTY and TIME (usually columns 1 and 2)
        char *tty_str = strtok(NULL, " \t\r\n");
        char *time_str = strtok(NULL, " \t\r\n");
        (void)tty_str; // unused
        (void)time_str; // unused
        
        char *cmd_str = strtok(NULL, " \t\r\n");
        if (!cmd_str) continue;
        
        // Clean up command name (remove path/extensions if any)
        char *base = strrchr(cmd_str, '/');
        if (base) cmd_str = base + 1;
        base = strrchr(cmd_str, '\\');
        if (base) cmd_str = base + 1;
        
        printf("%-10s %-20s\n", pid_str, cmd_str);
    }
    
    pclose(fp);
    return 1;
}

int object_where(char **args) {
    if (args[1] == NULL || args[2] == NULL || args[3] == NULL) {
        // If not enough args, print usage and just pass through stdin
        fprintf(stderr, "Usage: where [column] [operator] [value]\n");
        fprintf(stderr, "Operators: ==, !=, contains\n");
        
        char line[512];
        while (fgets(line, sizeof(line), stdin)) {
            printf("%s", line);
        }
        return 1;
    }
    
    char *target_col = args[1];
    char *op = args[2];
    char *target_val = args[3];
    
    char line[512];
    char header_line[512];
    
    // Read header
    if (fgets(header_line, sizeof(header_line), stdin) == NULL) {
        return 1;
    }
    
    // Parse header to find column index
    char header_cp[512];
    strcpy(header_cp, header_line);
    
    int col_index = -1;
    char *col_name = strtok(header_cp, " \t\r\n");
    int current_index = 0;
    while (col_name) {
        if (strcasecmp(col_name, target_col) == 0) {
            col_index = current_index;
            break;
        }
        col_name = strtok(NULL, " \t\r\n");
        current_index++;
    }
    
    if (col_index == -1) {
        fprintf(stderr, "where: Column '%s' not found.\n", target_col);
        // Print the header and pass through the rest
        printf("%s", header_line);
        while (fgets(line, sizeof(line), stdin)) {
            printf("%s", line);
        }
        return 1;
    }
    
    // Print header
    printf("%s", header_line);
    
    // Process matching rows
    while (fgets(line, sizeof(line), stdin)) {
        char line_cp[512];
        strcpy(line_cp, line);
        
        // Find value at col_index
        char *val = strtok(line_cp, " \t\r\n");
        int idx = 0;
        while (val && idx < col_index) {
            val = strtok(NULL, " \t\r\n");
            idx++;
        }
        
        if (!val) continue;
        
        val = trim(val);
        int match = 0;
        
        if (strcmp(op, "==") == 0) {
            if (strcasecmp(val, target_val) == 0) match = 1;
        } else if (strcmp(op, "!=") == 0) {
            if (strcasecmp(val, target_val) != 0) match = 1;
        } else if (strcmp(op, "contains") == 0) {
            if (strcasestr(val, target_val) != NULL) match = 1;
        }
        
        if (match) {
            printf("%s", line);
        }
    }
    
    return 1;
}

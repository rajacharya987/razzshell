#ifndef AST_H
#define AST_H

#include <stddef.h>

// Forward declarations
typedef struct ASTNode ASTNode;
typedef struct Command Command;
typedef struct Pipeline Pipeline;
typedef struct CommandList CommandList;
typedef struct Redirection Redirection;
typedef struct Assignment Assignment;

// AST node types
typedef enum {
    AST_COMMAND,          // Simple command
    AST_PIPELINE,         // Commands connected by pipes
    AST_LIST,             // Sequential commands (;)
    AST_AND_LIST,         // Commands with && 
    AST_OR_LIST,          // Commands with ||
    AST_SUBSHELL,         // Commands in ( )
    AST_FUNCTION,         // Function definition
    AST_IF,               // if statement
    AST_WHILE,            // while loop
    AST_FOR,              // for loop
    AST_CASE,             // case statement
    AST_ASSIGNMENT,       // Variable assignment
    AST_TEST,             // [[ ]] test
    AST_HEREDOC           // Here-document
} ASTNodeType;

// Redirection types
typedef enum {
    REDIR_INPUT,          // <
    REDIR_OUTPUT,         // >
    REDIR_APPEND,         // >>
    REDIR_ERROR,          // 2>
    REDIR_BOTH,           // &>
    REDIR_HEREDOC,        // <<
    REDIR_HEREDOC_STRIP   // <<-
} RedirectionType;

// Redirection structure
struct Redirection {
    RedirectionType type;
    char *target;         // File or delimiter
    char *content;        // For here-documents
    Redirection *next;    // Linked list
};

// Assignment structure (VAR=value)
struct Assignment {
    char *name;
    char *value;
    Assignment *next;
};

// Simple command structure
struct Command {
    char **argv;          // Command and arguments
    int argc;             // Argument count
    Assignment *assignments; // Variable assignments
    Redirection *redirections; // Redirections
    int background;       // Run in background (&)
};

// Pipeline structure (cmd1 | cmd2 | cmd3)
struct Pipeline {
    ASTNode **commands;   // Array of commands
    int count;            // Number of commands
    int negate;           // ! prefix
};

// Command list structure (cmd1; cmd2; cmd3)
struct CommandList {
    ASTNode **commands;   // Array of commands
    int count;            // Number of commands
};

// Subshell structure
typedef struct {
    ASTNode *body;        // Commands to execute in subshell
} Subshell;

// Function definition
typedef struct {
    char *name;
    ASTNode *body;
} Function;

// If statement
typedef struct {
    ASTNode *condition;
    ASTNode *then_part;
    ASTNode *else_part;   // Can be NULL
} IfStatement;

// While loop
typedef struct {
    ASTNode *condition;
    ASTNode *body;
} WhileLoop;

// For loop
typedef struct {
    char *variable;
    char **values;        // Array of values to iterate
    int value_count;
    ASTNode *body;
} ForLoop;

// Test expression [[ ]]
typedef struct {
    char **expressions;   // Test expressions
    int count;
} TestExpr;

// Here-document
typedef struct {
    char *delimiter;
    char *content;
    int strip_tabs;       // <<- vs <<
} HereDoc;

// Main AST node structure
struct ASTNode {
    ASTNodeType type;
    union {
        Command *command;
        Pipeline *pipeline;
        CommandList *list;
        Subshell *subshell;
        Function *function;
        IfStatement *if_stmt;
        WhileLoop *while_loop;
        ForLoop *for_loop;
        Assignment *assignment;
        TestExpr *test;
        HereDoc *heredoc;
    } data;
};

// Function prototypes
ASTNode* ast_create_command(char **argv, int argc);
ASTNode* ast_create_pipeline(ASTNode **commands, int count);
ASTNode* ast_create_list(ASTNode **commands, int count);
ASTNode* ast_create_and_list(ASTNode *left, ASTNode *right);
ASTNode* ast_create_or_list(ASTNode *left, ASTNode *right);
ASTNode* ast_create_subshell(ASTNode *body);
ASTNode* ast_create_assignment(const char *name, const char *value);
ASTNode* ast_create_test(char **expressions, int count);
ASTNode* ast_create_heredoc(const char *delimiter, const char *content, int strip_tabs);

void ast_destroy(ASTNode *node);
void ast_print(ASTNode *node, int indent);

// Redirection functions
Redirection* redirection_create(RedirectionType type, const char *target);
void redirection_destroy(Redirection *redir);
void command_add_redirection(Command *cmd, Redirection *redir);

// Assignment functions
Assignment* assignment_create(const char *name, const char *value);
void assignment_destroy(Assignment *assign);

#endif // AST_H

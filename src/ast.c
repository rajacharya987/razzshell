#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Create a simple command node
ASTNode* ast_create_command(char **argv, int argc) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = AST_COMMAND;
    node->data.command = malloc(sizeof(Command));
    if (!node->data.command) {
        free(node);
        return NULL;
    }
    
    Command *cmd = node->data.command;
    cmd->argv = malloc(sizeof(char*) * (argc + 1));
    if (!cmd->argv) {
        free(cmd);
        free(node);
        return NULL;
    }
    
    for (int i = 0; i < argc; i++) {
        cmd->argv[i] = strdup(argv[i]);
    }
    cmd->argv[argc] = NULL;
    cmd->argc = argc;
    cmd->assignments = NULL;
    cmd->redirections = NULL;
    cmd->background = 0;
    
    return node;
}

// Create a pipeline node
ASTNode* ast_create_pipeline(ASTNode **commands, int count) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = AST_PIPELINE;
    node->data.pipeline = malloc(sizeof(Pipeline));
    if (!node->data.pipeline) {
        free(node);
        return NULL;
    }
    
    Pipeline *pipe = node->data.pipeline;
    pipe->commands = malloc(sizeof(ASTNode*) * count);
    if (!pipe->commands) {
        free(pipe);
        free(node);
        return NULL;
    }
    
    for (int i = 0; i < count; i++) {
        pipe->commands[i] = commands[i];
    }
    pipe->count = count;
    pipe->negate = 0;
    
    return node;
}

// Create a command list node (sequential)
ASTNode* ast_create_list(ASTNode **commands, int count) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = AST_LIST;
    node->data.list = malloc(sizeof(CommandList));
    if (!node->data.list) {
        free(node);
        return NULL;
    }
    
    CommandList *list = node->data.list;
    list->commands = malloc(sizeof(ASTNode*) * count);
    if (!list->commands) {
        free(list);
        free(node);
        return NULL;
    }
    
    for (int i = 0; i < count; i++) {
        list->commands[i] = commands[i];
    }
    list->count = count;
    
    return node;
}

// Create an AND list node (&&)
ASTNode* ast_create_and_list(ASTNode *left, ASTNode *right) {
    ASTNode *commands[2] = {left, right};
    ASTNode *node = ast_create_list(commands, 2);
    if (node) {
        node->type = AST_AND_LIST;
    }
    return node;
}

// Create an OR list node (||)
ASTNode* ast_create_or_list(ASTNode *left, ASTNode *right) {
    ASTNode *commands[2] = {left, right};
    ASTNode *node = ast_create_list(commands, 2);
    if (node) {
        node->type = AST_OR_LIST;
    }
    return node;
}

// Create a subshell node
ASTNode* ast_create_subshell(ASTNode *body) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = AST_SUBSHELL;
    node->data.subshell = malloc(sizeof(Subshell));
    if (!node->data.subshell) {
        free(node);
        return NULL;
    }
    
    node->data.subshell->body = body;
    return node;
}

// Create an assignment node
ASTNode* ast_create_assignment(const char *name, const char *value) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = AST_ASSIGNMENT;
    node->data.assignment = assignment_create(name, value);
    if (!node->data.assignment) {
        free(node);
        return NULL;
    }
    
    return node;
}

// Create a test node [[ ]]
ASTNode* ast_create_test(char **expressions, int count) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = AST_TEST;
    node->data.test = malloc(sizeof(TestExpr));
    if (!node->data.test) {
        free(node);
        return NULL;
    }
    
    TestExpr *test = node->data.test;
    test->expressions = malloc(sizeof(char*) * count);
    if (!test->expressions) {
        free(test);
        free(node);
        return NULL;
    }
    
    for (int i = 0; i < count; i++) {
        test->expressions[i] = strdup(expressions[i]);
    }
    test->count = count;
    
    return node;
}

// Create a here-document node
ASTNode* ast_create_heredoc(const char *delimiter, const char *content, int strip_tabs) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = AST_HEREDOC;
    node->data.heredoc = malloc(sizeof(HereDoc));
    if (!node->data.heredoc) {
        free(node);
        return NULL;
    }
    
    HereDoc *heredoc = node->data.heredoc;
    heredoc->delimiter = strdup(delimiter);
    heredoc->content = content ? strdup(content) : NULL;
    heredoc->strip_tabs = strip_tabs;
    
    return node;
}

// Create a redirection
Redirection* redirection_create(RedirectionType type, const char *target) {
    Redirection *redir = malloc(sizeof(Redirection));
    if (!redir) return NULL;
    
    redir->type = type;
    redir->target = strdup(target);
    redir->content = NULL;
    redir->next = NULL;
    
    return redir;
}

// Destroy redirection
void redirection_destroy(Redirection *redir) {
    while (redir) {
        Redirection *next = redir->next;
        free(redir->target);
        if (redir->content) free(redir->content);
        free(redir);
        redir = next;
    }
}

// Add redirection to command
void command_add_redirection(Command *cmd, Redirection *redir) {
    if (!cmd->redirections) {
        cmd->redirections = redir;
    } else {
        Redirection *last = cmd->redirections;
        while (last->next) {
            last = last->next;
        }
        last->next = redir;
    }
}

// Create an assignment
Assignment* assignment_create(const char *name, const char *value) {
    Assignment *assign = malloc(sizeof(Assignment));
    if (!assign) return NULL;
    
    assign->name = strdup(name);
    assign->value = strdup(value);
    assign->next = NULL;
    
    return assign;
}

// Destroy assignment
void assignment_destroy(Assignment *assign) {
    while (assign) {
        Assignment *next = assign->next;
        free(assign->name);
        free(assign->value);
        free(assign);
        assign = next;
    }
}

// Destroy AST node
void ast_destroy(ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_COMMAND:
            if (node->data.command) {
                for (int i = 0; i < node->data.command->argc; i++) {
                    free(node->data.command->argv[i]);
                }
                free(node->data.command->argv);
                assignment_destroy(node->data.command->assignments);
                redirection_destroy(node->data.command->redirections);
                free(node->data.command);
            }
            break;
            
        case AST_PIPELINE:
            if (node->data.pipeline) {
                for (int i = 0; i < node->data.pipeline->count; i++) {
                    ast_destroy(node->data.pipeline->commands[i]);
                }
                free(node->data.pipeline->commands);
                free(node->data.pipeline);
            }
            break;
            
        case AST_LIST:
        case AST_AND_LIST:
        case AST_OR_LIST:
            if (node->data.list) {
                for (int i = 0; i < node->data.list->count; i++) {
                    ast_destroy(node->data.list->commands[i]);
                }
                free(node->data.list->commands);
                free(node->data.list);
            }
            break;
            
        case AST_SUBSHELL:
            if (node->data.subshell) {
                ast_destroy(node->data.subshell->body);
                free(node->data.subshell);
            }
            break;
            
        case AST_ASSIGNMENT:
            assignment_destroy(node->data.assignment);
            break;
            
        case AST_TEST:
            if (node->data.test) {
                for (int i = 0; i < node->data.test->count; i++) {
                    free(node->data.test->expressions[i]);
                }
                free(node->data.test->expressions);
                free(node->data.test);
            }
            break;
            
        case AST_HEREDOC:
            if (node->data.heredoc) {
                free(node->data.heredoc->delimiter);
                if (node->data.heredoc->content) {
                    free(node->data.heredoc->content);
                }
                free(node->data.heredoc);
            }
            break;
            
        default:
            // Other types not yet implemented
            break;
    }
    
    free(node);
}

// Print AST (for debugging)
static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

void ast_print(ASTNode *node, int indent) {
    if (!node) return;
    
    print_indent(indent);
    
    switch (node->type) {
        case AST_COMMAND:
            printf("COMMAND:");
            for (int i = 0; i < node->data.command->argc; i++) {
                printf(" %s", node->data.command->argv[i]);
            }
            if (node->data.command->background) {
                printf(" &");
            }
            printf("\n");
            
            // Print redirections
            Redirection *redir = node->data.command->redirections;
            while (redir) {
                print_indent(indent + 1);
                printf("REDIR: ");
                switch (redir->type) {
                    case REDIR_INPUT: printf("<"); break;
                    case REDIR_OUTPUT: printf(">"); break;
                    case REDIR_APPEND: printf(">>"); break;
                    case REDIR_ERROR: printf("2>"); break;
                    case REDIR_BOTH: printf("&>"); break;
                    case REDIR_HEREDOC: printf("<<"); break;
                    case REDIR_HEREDOC_STRIP: printf("<<-"); break;
                }
                printf(" %s\n", redir->target);
                redir = redir->next;
            }
            break;
            
        case AST_PIPELINE:
            printf("PIPELINE (%d commands)\n", node->data.pipeline->count);
            for (int i = 0; i < node->data.pipeline->count; i++) {
                ast_print(node->data.pipeline->commands[i], indent + 1);
            }
            break;
            
        case AST_LIST:
            printf("LIST (%d commands)\n", node->data.list->count);
            for (int i = 0; i < node->data.list->count; i++) {
                ast_print(node->data.list->commands[i], indent + 1);
            }
            break;
            
        case AST_AND_LIST:
            printf("AND_LIST (&&)\n");
            for (int i = 0; i < node->data.list->count; i++) {
                ast_print(node->data.list->commands[i], indent + 1);
            }
            break;
            
        case AST_OR_LIST:
            printf("OR_LIST (||)\n");
            for (int i = 0; i < node->data.list->count; i++) {
                ast_print(node->data.list->commands[i], indent + 1);
            }
            break;
            
        case AST_SUBSHELL:
            printf("SUBSHELL\n");
            ast_print(node->data.subshell->body, indent + 1);
            break;
            
        case AST_ASSIGNMENT:
            printf("ASSIGNMENT: %s=%s\n", 
                   node->data.assignment->name,
                   node->data.assignment->value);
            break;
            
        case AST_TEST:
            printf("TEST [[");
            for (int i = 0; i < node->data.test->count; i++) {
                printf(" %s", node->data.test->expressions[i]);
            }
            printf(" ]]\n");
            break;
            
        case AST_HEREDOC:
            printf("HEREDOC: <<%s %s\n",
                   node->data.heredoc->strip_tabs ? "-" : "",
                   node->data.heredoc->delimiter);
            break;
            
        default:
            printf("UNKNOWN NODE TYPE\n");
            break;
    }
}

#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Create a new parser
Parser* parser_create(const char *input) {
    Parser *parser = malloc(sizeof(Parser));
    if (!parser) return NULL;
    
    parser->lexer = lexer_create(input);
    if (!parser->lexer) {
        free(parser);
        return NULL;
    }
    
    parser->current_token = NULL;
    parser->peek_token = NULL;
    parser->error = 0;
    parser->error_message[0] = '\0';
    
    // Prime the parser with first two tokens
    parser_advance(parser);
    parser_advance(parser);
    
    return parser;
}

// Destroy parser
void parser_destroy(Parser *parser) {
    if (parser) {
        if (parser->current_token) {
            token_destroy(parser->current_token);
        }
        if (parser->peek_token) {
            token_destroy(parser->peek_token);
        }
        if (parser->lexer) {
            lexer_destroy(parser->lexer);
        }
        free(parser);
    }
}

// Advance to next token
void parser_advance(Parser *parser) {
    if (parser->current_token) {
        token_destroy(parser->current_token);
    }
    
    parser->current_token = parser->peek_token;
    parser->peek_token = lexer_next_token(parser->lexer);
    
    // Skip newlines in most contexts
    while (parser->current_token && 
           parser->current_token->type == TOKEN_NEWLINE) {
        token_destroy(parser->current_token);
        parser->current_token = parser->peek_token;
        parser->peek_token = lexer_next_token(parser->lexer);
    }
}

// Check if current token matches expected type
int parser_expect(Parser *parser, TokenType type) {
    if (!parser->current_token || parser->current_token->type != type) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Expected %s, got %s",
                token_type_name(type),
                parser->current_token ? token_type_name(parser->current_token->type) : "EOF");
        parser_error(parser, msg);
        return 0;
    }
    return 1;
}

// Match and consume token if it matches
int parser_match(Parser *parser, TokenType type) {
    if (parser->current_token && parser->current_token->type == type) {
        parser_advance(parser);
        return 1;
    }
    return 0;
}

// Set parser error
void parser_error(Parser *parser, const char *message) {
    parser->error = 1;
    strncpy(parser->error_message, message, sizeof(parser->error_message) - 1);
    parser->error_message[sizeof(parser->error_message) - 1] = '\0';
}

// Parse main entry point
ASTNode* parser_parse(Parser *parser) {
    if (!parser->current_token || parser->current_token->type == TOKEN_EOF) {
        return NULL;
    }
    
    return parser_parse_command_line(parser);
}

// Parse a complete command line (handles ;, &&, ||)
ASTNode* parser_parse_command_line(Parser *parser) {
    ASTNode *left = parser_parse_pipeline(parser);
    if (!left || parser->error) {
        return left;
    }
    
    // Handle logical operators and semicolons
    while (parser->current_token) {
        if (parser_match(parser, TOKEN_AND)) {
            ASTNode *right = parser_parse_pipeline(parser);
            if (!right) {
                ast_destroy(left);
                return NULL;
            }
            left = ast_create_and_list(left, right);
        }
        else if (parser_match(parser, TOKEN_OR)) {
            ASTNode *right = parser_parse_pipeline(parser);
            if (!right) {
                ast_destroy(left);
                return NULL;
            }
            left = ast_create_or_list(left, right);
        }
        else if (parser_match(parser, TOKEN_SEMICOLON)) {
            // Build a list of commands
            ASTNode *right = parser_parse_pipeline(parser);
            if (!right) {
                // Semicolon at end is OK
                break;
            }
            ASTNode *commands[2] = {left, right};
            left = ast_create_list(commands, 2);
        }
        else {
            break;
        }
    }
    
    return left;
}

// Parse a pipeline (cmd1 | cmd2 | cmd3)
ASTNode* parser_parse_pipeline(Parser *parser) {
    ASTNode *first = parser_parse_command(parser);
    if (!first || parser->error) {
        return first;
    }
    
    // Check for pipe
    if (!parser->current_token || parser->current_token->type != TOKEN_PIPE) {
        return first;
    }
    
    // Build pipeline
    ASTNode **commands = malloc(sizeof(ASTNode*) * 16); // Initial capacity
    int count = 0;
    int capacity = 16;
    
    commands[count++] = first;
    
    while (parser_match(parser, TOKEN_PIPE)) {
        ASTNode *next = parser_parse_command(parser);
        if (!next) {
            parser_error(parser, "Expected command after pipe");
            for (int i = 0; i < count; i++) {
                ast_destroy(commands[i]);
            }
            free(commands);
            return NULL;
        }
        
        if (count >= capacity) {
            capacity *= 2;
            commands = realloc(commands, sizeof(ASTNode*) * capacity);
        }
        
        commands[count++] = next;
    }
    
    ASTNode *pipeline = ast_create_pipeline(commands, count);
    free(commands);
    return pipeline;
}

// Parse a single command
ASTNode* parser_parse_command(Parser *parser) {
    if (!parser->current_token) {
        return NULL;
    }
    
    // Handle subshell
    if (parser->current_token->type == TOKEN_LPAREN) {
        return parser_parse_subshell(parser);
    }
    
    // Handle [[ ]] test
    if (parser->current_token->type == TOKEN_DBLBRACKET_L) {
        return parser_parse_test(parser);
    }
    
    // Handle assignment (VAR=value)
    if (parser->current_token->type == TOKEN_WORD &&
        parser->peek_token && parser->peek_token->type == TOKEN_ASSIGN) {
        return parser_parse_assignment(parser);
    }
    
    // Simple command
    return parser_parse_simple_command(parser);
}

// Parse a simple command with arguments and redirections
ASTNode* parser_parse_simple_command(Parser *parser) {
    char **argv = malloc(sizeof(char*) * 64);
    int argc = 0;
    int capacity = 64;
    Redirection *redirections = NULL;
    int background = 0;
    
    // Collect words and handle redirections
    while (parser->current_token) {
        TokenType type = parser->current_token->type;
        
        if (type == TOKEN_WORD) {
            if (argc >= capacity) {
                capacity *= 2;
                argv = realloc(argv, sizeof(char*) * capacity);
            }
            argv[argc++] = strdup(parser->current_token->value);
            parser_advance(parser);
        }
        else if (type == TOKEN_REDIRECT_IN || type == TOKEN_REDIRECT_OUT ||
                 type == TOKEN_REDIRECT_APPEND || type == TOKEN_REDIRECT_ERR ||
                 type == TOKEN_REDIRECT_BOTH) {
            // Parse redirection
            RedirectionType redir_type;
            switch (type) {
                case TOKEN_REDIRECT_IN: redir_type = REDIR_INPUT; break;
                case TOKEN_REDIRECT_OUT: redir_type = REDIR_OUTPUT; break;
                case TOKEN_REDIRECT_APPEND: redir_type = REDIR_APPEND; break;
                case TOKEN_REDIRECT_ERR: redir_type = REDIR_ERROR; break;
                case TOKEN_REDIRECT_BOTH: redir_type = REDIR_BOTH; break;
                default: redir_type = REDIR_OUTPUT; break;
            }
            
            parser_advance(parser);
            
            if (!parser->current_token || parser->current_token->type != TOKEN_WORD) {
                parser_error(parser, "Expected filename after redirection");
                for (int i = 0; i < argc; i++) free(argv[i]);
                free(argv);
                redirection_destroy(redirections);
                return NULL;
            }
            
            Redirection *redir = redirection_create(redir_type, parser->current_token->value);
            if (!redirections) {
                redirections = redir;
            } else {
                Redirection *last = redirections;
                while (last->next) last = last->next;
                last->next = redir;
            }
            
            parser_advance(parser);
        }
        else if (type == TOKEN_BACKGROUND) {
            background = 1;
            parser_advance(parser);
            break;
        }
        else {
            // End of command
            break;
        }
    }
    
    if (argc == 0) {
        free(argv);
        redirection_destroy(redirections);
        return NULL;
    }
    
    ASTNode *node = ast_create_command(argv, argc);
    if (node) {
        node->data.command->redirections = redirections;
        node->data.command->background = background;
    }
    
    for (int i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);
    
    return node;
}

// Parse subshell ( commands )
ASTNode* parser_parse_subshell(Parser *parser) {
    if (!parser_expect(parser, TOKEN_LPAREN)) {
        return NULL;
    }
    parser_advance(parser); // consume (
    
    ASTNode *body = parser_parse_command_line(parser);
    if (!body) {
        parser_error(parser, "Expected commands in subshell");
        return NULL;
    }
    
    if (!parser_expect(parser, TOKEN_RPAREN)) {
        ast_destroy(body);
        return NULL;
    }
    parser_advance(parser); // consume )
    
    return ast_create_subshell(body);
}

// Parse [[ ]] test
ASTNode* parser_parse_test(Parser *parser) {
    if (!parser_expect(parser, TOKEN_DBLBRACKET_L)) {
        return NULL;
    }
    parser_advance(parser); // consume [[
    
    char **expressions = malloc(sizeof(char*) * 32);
    int count = 0;
    int capacity = 32;
    
    // Collect all tokens until ]]
    while (parser->current_token && 
           parser->current_token->type != TOKEN_DBLBRACKET_R) {
        if (count >= capacity) {
            capacity *= 2;
            expressions = realloc(expressions, sizeof(char*) * capacity);
        }
        expressions[count++] = strdup(parser->current_token->value);
        parser_advance(parser);
    }
    
    if (!parser_expect(parser, TOKEN_DBLBRACKET_R)) {
        for (int i = 0; i < count; i++) free(expressions[i]);
        free(expressions);
        return NULL;
    }
    parser_advance(parser); // consume ]]
    
    ASTNode *node = ast_create_test(expressions, count);
    
    for (int i = 0; i < count; i++) {
        free(expressions[i]);
    }
    free(expressions);
    
    return node;
}

// Parse assignment (VAR=value)
ASTNode* parser_parse_assignment(Parser *parser) {
    if (!parser_expect(parser, TOKEN_WORD)) {
        return NULL;
    }
    
    char *name = strdup(parser->current_token->value);
    parser_advance(parser);
    
    if (!parser_expect(parser, TOKEN_ASSIGN)) {
        free(name);
        return NULL;
    }
    parser_advance(parser);
    
    char *value = "";
    if (parser->current_token && parser->current_token->type == TOKEN_WORD) {
        value = parser->current_token->value;
        parser_advance(parser);
    }
    
    ASTNode *node = ast_create_assignment(name, value);
    free(name);
    
    return node;
}

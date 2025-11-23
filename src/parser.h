#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

// Parser state
typedef struct {
    Lexer *lexer;
    Token *current_token;
    Token *peek_token;
    int error;
    char error_message[256];
} Parser;

// Function prototypes
Parser* parser_create(const char *input);
void parser_destroy(Parser *parser);

// Main parsing function
ASTNode* parser_parse(Parser *parser);

// Parsing functions for different constructs
ASTNode* parser_parse_command_line(Parser *parser);
ASTNode* parser_parse_pipeline(Parser *parser);
ASTNode* parser_parse_command(Parser *parser);
ASTNode* parser_parse_simple_command(Parser *parser);
ASTNode* parser_parse_subshell(Parser *parser);
ASTNode* parser_parse_test(Parser *parser);
ASTNode* parser_parse_assignment(Parser *parser);

// Helper functions
void parser_advance(Parser *parser);
int parser_expect(Parser *parser, TokenType type);
int parser_match(Parser *parser, TokenType type);
void parser_error(Parser *parser, const char *message);

#endif // PARSER_H

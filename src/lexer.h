#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

// Token types for shell syntax
typedef enum {
    TOKEN_WORD,           // Regular word/command/argument
    TOKEN_PIPE,           // |
    TOKEN_REDIRECT_IN,    // <
    TOKEN_REDIRECT_OUT,   // >
    TOKEN_REDIRECT_APPEND,// >>
    TOKEN_REDIRECT_ERR,   // 2>
    TOKEN_REDIRECT_BOTH,  // &>
    TOKEN_BACKGROUND,     // &
    TOKEN_SEMICOLON,      // ;
    TOKEN_AND,            // &&
    TOKEN_OR,             // ||
    TOKEN_LPAREN,         // (
    TOKEN_RPAREN,         // )
    TOKEN_LBRACE,         // {
    TOKEN_RBRACE,         // }
    TOKEN_LBRACKET,       // [
    TOKEN_RBRACKET,       // ]
    TOKEN_DBLBRACKET_L,   // [[
    TOKEN_DBLBRACKET_R,   // ]]
    TOKEN_HEREDOC,        // <<
    TOKEN_HEREDOC_STRIP,  // <<-
    TOKEN_SUBST_START,    // $(
    TOKEN_BACKTICK,       // `
    TOKEN_DOLLAR,         // $
    TOKEN_ASSIGN,         // =
    TOKEN_NEWLINE,        // \n
    TOKEN_EOF,            // End of input
    TOKEN_ERROR           // Lexer error
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    char *value;          // Token text
    size_t length;        // Length of value
    int line;             // Line number
    int column;           // Column number
} Token;

// Lexer state
typedef struct {
    const char *input;    // Input string
    size_t pos;           // Current position
    size_t length;        // Input length
    int line;             // Current line
    int column;           // Current column
    int in_quotes;        // Inside quotes flag
    char quote_char;      // Current quote character (' or ")
} Lexer;

// Function prototypes
Lexer* lexer_create(const char *input);
void lexer_destroy(Lexer *lexer);
Token* lexer_next_token(Lexer *lexer);
void token_destroy(Token *token);
const char* token_type_name(TokenType type);

// Helper functions
int lexer_is_whitespace(char c);
int lexer_is_operator_char(char c);
int lexer_is_word_char(char c);
char lexer_peek(Lexer *lexer, int offset);
char lexer_advance(Lexer *lexer);
void lexer_skip_whitespace(Lexer *lexer);

#endif // LEXER_H

#include "lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// Create a new lexer
Lexer* lexer_create(const char *input) {
    Lexer *lexer = malloc(sizeof(Lexer));
    if (!lexer) return NULL;
    
    lexer->input = input;
    lexer->pos = 0;
    lexer->length = strlen(input);
    lexer->line = 1;
    lexer->column = 1;
    lexer->in_quotes = 0;
    lexer->quote_char = 0;
    
    return lexer;
}

// Destroy lexer
void lexer_destroy(Lexer *lexer) {
    if (lexer) {
        free(lexer);
    }
}

// Destroy token
void token_destroy(Token *token) {
    if (token) {
        if (token->value) {
            free(token->value);
        }
        free(token);
    }
}

// Check if character is whitespace
int lexer_is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r';
}

// Check if character is an operator character
int lexer_is_operator_char(char c) {
    return c == '|' || c == '&' || c == ';' || c == '<' || c == '>' ||
           c == '(' || c == ')' || c == '{' || c == '}' ||
           c == '[' || c == ']' || c == '$' || c == '`';
}

// Check if character can be part of a word
int lexer_is_word_char(char c) {
    return !lexer_is_whitespace(c) && !lexer_is_operator_char(c) && c != '\n' && c != '\0';
}

// Peek at character at offset from current position
char lexer_peek(Lexer *lexer, int offset) {
    size_t pos = lexer->pos + offset;
    if (pos >= lexer->length) {
        return '\0';
    }
    return lexer->input[pos];
}

// Advance to next character
char lexer_advance(Lexer *lexer) {
    if (lexer->pos >= lexer->length) {
        return '\0';
    }
    
    char c = lexer->input[lexer->pos++];
    
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    
    return c;
}

// Skip whitespace
void lexer_skip_whitespace(Lexer *lexer) {
    while (lexer->pos < lexer->length && lexer_is_whitespace(lexer->input[lexer->pos])) {
        lexer_advance(lexer);
    }
}

// Create a token
static Token* create_token(TokenType type, const char *value, size_t length, int line, int column) {
    Token *token = malloc(sizeof(Token));
    if (!token) return NULL;
    
    token->type = type;
    token->length = length;
    token->line = line;
    token->column = column;
    
    if (value && length > 0) {
        token->value = malloc(length + 1);
        if (!token->value) {
            free(token);
            return NULL;
        }
        memcpy(token->value, value, length);
        token->value[length] = '\0';
    } else {
        token->value = NULL;
    }
    
    return token;
}

// Read a quoted string
static Token* lexer_read_quoted_string(Lexer *lexer, char quote) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    size_t start = lexer->pos;
    
    lexer_advance(lexer); // Skip opening quote
    
    while (lexer->pos < lexer->length) {
        char c = lexer_peek(lexer, 0);
        
        if (c == '\\' && lexer_peek(lexer, 1) == quote) {
            // Escaped quote
            lexer_advance(lexer);
            lexer_advance(lexer);
        } else if (c == quote) {
            // Closing quote
            size_t end = lexer->pos;
            lexer_advance(lexer); // Skip closing quote
            return create_token(TOKEN_WORD, lexer->input + start, end - start + 1, start_line, start_column);
        } else if (c == '\0') {
            // Unclosed quote
            return create_token(TOKEN_ERROR, "Unclosed quote", 14, start_line, start_column);
        } else {
            lexer_advance(lexer);
        }
    }
    
    return create_token(TOKEN_ERROR, "Unclosed quote", 14, start_line, start_column);
}

// Read a word token
static Token* lexer_read_word(Lexer *lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    size_t start = lexer->pos;
    
    while (lexer->pos < lexer->length) {
        char c = lexer_peek(lexer, 0);
        
        // Handle quotes within words
        if (c == '"' || c == '\'') {
            Token *quoted = lexer_read_quoted_string(lexer, c);
            if (quoted->type == TOKEN_ERROR) {
                return quoted;
            }
            token_destroy(quoted);
            continue;
        }
        
        // Handle escape sequences
        if (c == '\\' && lexer_peek(lexer, 1) != '\0') {
            lexer_advance(lexer);
            lexer_advance(lexer);
            continue;
        }
        
        if (!lexer_is_word_char(c)) {
            break;
        }
        
        lexer_advance(lexer);
    }
    
    size_t length = lexer->pos - start;
    return create_token(TOKEN_WORD, lexer->input + start, length, start_line, start_column);
}

// Get next token
Token* lexer_next_token(Lexer *lexer) {
    lexer_skip_whitespace(lexer);
    
    if (lexer->pos >= lexer->length) {
        return create_token(TOKEN_EOF, NULL, 0, lexer->line, lexer->column);
    }
    
    int start_line = lexer->line;
    int start_column = lexer->column;
    char c = lexer_peek(lexer, 0);
    char next = lexer_peek(lexer, 1);
    
    // Handle newline
    if (c == '\n') {
        lexer_advance(lexer);
        return create_token(TOKEN_NEWLINE, "\n", 1, start_line, start_column);
    }
    
    // Handle comments
    if (c == '#') {
        while (lexer->pos < lexer->length && lexer_peek(lexer, 0) != '\n') {
            lexer_advance(lexer);
        }
        return lexer_next_token(lexer); // Skip comment and get next token
    }
    
    // Handle two-character operators
    if (c == '|' && next == '|') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return create_token(TOKEN_OR, "||", 2, start_line, start_column);
    }
    
    if (c == '&' && next == '&') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return create_token(TOKEN_AND, "&&", 2, start_line, start_column);
    }
    
    if (c == '>' && next == '>') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return create_token(TOKEN_REDIRECT_APPEND, ">>", 2, start_line, start_column);
    }
    
    if (c == '<' && next == '<') {
        char third = lexer_peek(lexer, 2);
        if (third == '-') {
            lexer_advance(lexer);
            lexer_advance(lexer);
            lexer_advance(lexer);
            return create_token(TOKEN_HEREDOC_STRIP, "<<-", 3, start_line, start_column);
        }
        lexer_advance(lexer);
        lexer_advance(lexer);
        return create_token(TOKEN_HEREDOC, "<<", 2, start_line, start_column);
    }
    
    if (c == '&' && next == '>') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return create_token(TOKEN_REDIRECT_BOTH, "&>", 2, start_line, start_column);
    }
    
    if (c == '2' && next == '>') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return create_token(TOKEN_REDIRECT_ERR, "2>", 2, start_line, start_column);
    }
    
    if (c == '[' && next == '[') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return create_token(TOKEN_DBLBRACKET_L, "[[", 2, start_line, start_column);
    }
    
    if (c == ']' && next == ']') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return create_token(TOKEN_DBLBRACKET_R, "]]", 2, start_line, start_column);
    }
    
    if (c == '$' && next == '(') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return create_token(TOKEN_SUBST_START, "$(", 2, start_line, start_column);
    }
    
    // Handle single-character operators
    switch (c) {
        case '|':
            lexer_advance(lexer);
            return create_token(TOKEN_PIPE, "|", 1, start_line, start_column);
        case '&':
            lexer_advance(lexer);
            return create_token(TOKEN_BACKGROUND, "&", 1, start_line, start_column);
        case ';':
            lexer_advance(lexer);
            return create_token(TOKEN_SEMICOLON, ";", 1, start_line, start_column);
        case '<':
            lexer_advance(lexer);
            return create_token(TOKEN_REDIRECT_IN, "<", 1, start_line, start_column);
        case '>':
            lexer_advance(lexer);
            return create_token(TOKEN_REDIRECT_OUT, ">", 1, start_line, start_column);
        case '(':
            lexer_advance(lexer);
            return create_token(TOKEN_LPAREN, "(", 1, start_line, start_column);
        case ')':
            lexer_advance(lexer);
            return create_token(TOKEN_RPAREN, ")", 1, start_line, start_column);
        case '{':
            lexer_advance(lexer);
            return create_token(TOKEN_LBRACE, "{", 1, start_line, start_column);
        case '}':
            lexer_advance(lexer);
            return create_token(TOKEN_RBRACE, "}", 1, start_line, start_column);
        case '[':
            lexer_advance(lexer);
            return create_token(TOKEN_LBRACKET, "[", 1, start_line, start_column);
        case ']':
            lexer_advance(lexer);
            return create_token(TOKEN_RBRACKET, "]", 1, start_line, start_column);
        case '`':
            lexer_advance(lexer);
            return create_token(TOKEN_BACKTICK, "`", 1, start_line, start_column);
        case '$':
            lexer_advance(lexer);
            return create_token(TOKEN_DOLLAR, "$", 1, start_line, start_column);
        case '=':
            lexer_advance(lexer);
            return create_token(TOKEN_ASSIGN, "=", 1, start_line, start_column);
    }
    
    // Handle quoted strings
    if (c == '"' || c == '\'') {
        return lexer_read_quoted_string(lexer, c);
    }
    
    // Handle words
    if (lexer_is_word_char(c)) {
        return lexer_read_word(lexer);
    }
    
    // Unknown character
    lexer_advance(lexer);
    char error_msg[50];
    snprintf(error_msg, sizeof(error_msg), "Unexpected character: '%c'", c);
    return create_token(TOKEN_ERROR, error_msg, strlen(error_msg), start_line, start_column);
}

// Get token type name (for debugging)
const char* token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_WORD: return "WORD";
        case TOKEN_PIPE: return "PIPE";
        case TOKEN_REDIRECT_IN: return "REDIRECT_IN";
        case TOKEN_REDIRECT_OUT: return "REDIRECT_OUT";
        case TOKEN_REDIRECT_APPEND: return "REDIRECT_APPEND";
        case TOKEN_REDIRECT_ERR: return "REDIRECT_ERR";
        case TOKEN_REDIRECT_BOTH: return "REDIRECT_BOTH";
        case TOKEN_BACKGROUND: return "BACKGROUND";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_AND: return "AND";
        case TOKEN_OR: return "OR";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_DBLBRACKET_L: return "DBLBRACKET_L";
        case TOKEN_DBLBRACKET_R: return "DBLBRACKET_R";
        case TOKEN_HEREDOC: return "HEREDOC";
        case TOKEN_HEREDOC_STRIP: return "HEREDOC_STRIP";
        case TOKEN_SUBST_START: return "SUBST_START";
        case TOKEN_BACKTICK: return "BACKTICK";
        case TOKEN_DOLLAR: return "DOLLAR";
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_NEWLINE: return "NEWLINE";
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

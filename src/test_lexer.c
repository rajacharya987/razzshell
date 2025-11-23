#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>

void test_lexer(const char *input) {
    printf("\n========================================\n");
    printf("Input: %s\n", input);
    printf("========================================\n");
    
    Lexer *lexer = lexer_create(input);
    if (!lexer) {
        printf("Failed to create lexer\n");
        return;
    }
    
    Token *token;
    int token_num = 0;
    
    while (1) {
        token = lexer_next_token(lexer);
        if (!token) {
            printf("Failed to get token\n");
            break;
        }
        
        printf("Token %d: %-20s ", ++token_num, token_type_name(token->type));
        if (token->value) {
            printf("'%s'", token->value);
        }
        printf(" (line %d, col %d)\n", token->line, token->column);
        
        TokenType type = token->type;
        token_destroy(token);
        
        if (type == TOKEN_EOF || type == TOKEN_ERROR) {
            break;
        }
    }
    
    lexer_destroy(lexer);
}

int main() {
    printf("RazzShell Lexer Test Suite\n");
    printf("===========================\n");
    
    // Test 1: Simple command
    test_lexer("ls -la /tmp");
    
    // Test 2: Pipe
    test_lexer("cat file.txt | grep pattern");
    
    // Test 3: Redirections
    test_lexer("echo hello > output.txt");
    test_lexer("cat < input.txt");
    test_lexer("command >> append.txt");
    test_lexer("command 2> error.log");
    test_lexer("command &> all.log");
    
    // Test 4: Background
    test_lexer("sleep 10 &");
    
    // Test 5: Logical operators
    test_lexer("make && make install");
    test_lexer("command || echo failed");
    
    // Test 6: Command substitution
    test_lexer("echo $(pwd)");
    test_lexer("echo `date`");
    
    // Test 7: Here-document
    test_lexer("cat << EOF");
    test_lexer("cat <<- EOF");
    
    // Test 8: Bash [[ ]]
    test_lexer("[[ -f file.txt ]]");
    
    // Test 9: Quotes
    test_lexer("echo \"hello world\"");
    test_lexer("echo 'single quotes'");
    
    // Test 10: Variables
    test_lexer("echo $HOME");
    test_lexer("VAR=value");
    
    // Test 11: Complex command
    test_lexer("if [[ -f file ]]; then cat file | grep pattern; fi");
    
    // Test 12: Semicolon
    test_lexer("cd /tmp; ls; pwd");
    
    // Test 13: Parentheses (subshell)
    test_lexer("(cd /tmp && ls)");
    
    // Test 14: Comments
    test_lexer("ls # this is a comment");
    
    // Test 15: Escape sequences
    test_lexer("echo hello\\ world");
    
    printf("\n========================================\n");
    printf("All tests complete!\n");
    printf("========================================\n");
    
    return 0;
}

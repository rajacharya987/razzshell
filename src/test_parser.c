#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

void test_parser(const char *input) {
    printf("\n========================================\n");
    printf("Input: %s\n", input);
    printf("========================================\n");
    
    Parser *parser = parser_create(input);
    if (!parser) {
        printf("Failed to create parser\n");
        return;
    }
    
    ASTNode *ast = parser_parse(parser);
    
    if (parser->error) {
        printf("Parse error: %s\n", parser->error_message);
    } else if (ast) {
        printf("AST:\n");
        ast_print(ast, 0);
        ast_destroy(ast);
    } else {
        printf("Empty input\n");
    }
    
    parser_destroy(parser);
}

int main() {
    printf("RazzShell Parser Test Suite\n");
    printf("============================\n");
    
    // Test 1: Simple command
    test_parser("ls -la /tmp");
    
    // Test 2: Pipe
    test_parser("cat file.txt | grep pattern");
    
    // Test 3: Multiple pipes
    test_parser("cat file | grep foo | sort | uniq");
    
    // Test 4: Redirections
    test_parser("echo hello > output.txt");
    test_parser("cat < input.txt");
    test_parser("command >> append.txt");
    test_parser("command 2> error.log");
    
    // Test 5: Background
    test_parser("sleep 10 &");
    
    // Test 6: Logical AND
    test_parser("make && make install");
    
    // Test 7: Logical OR
    test_parser("command || echo failed");
    
    // Test 8: Semicolon
    test_parser("cd /tmp; ls; pwd");
    
    // Test 9: Subshell
    test_parser("(cd /tmp && ls)");
    
    // Test 10: Test expression
    test_parser("[[ -f file.txt ]]");
    test_parser("[[ $x -eq 5 ]]");
    
    // Test 11: Assignment
    test_parser("VAR=value");
    test_parser("PATH=/usr/bin");
    
    // Test 12: Complex command
    test_parser("cat file.txt | grep pattern > output.txt &");
    
    // Test 13: Multiple redirections
    test_parser("command < input.txt > output.txt 2> error.log");
    
    // Test 14: Nested subshells
    test_parser("(cd /tmp && (ls | wc -l))");
    
    // Test 15: Mixed operators
    test_parser("make && make test || echo failed");
    
    printf("\n========================================\n");
    printf("All tests complete!\n");
    printf("========================================\n");
    
    return 0;
}

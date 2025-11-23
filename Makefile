# RazzShell Makefile
CC = gcc
CFLAGS = -Wall -Wextra -g -I.
LDFLAGS = -lreadline -ldl -lncurses

# Source files
SRCS = razzshell.c src/shell_config.c src/posix_compat.c src/lexer.c src/ast.c src/parser.c
OBJS = $(SRCS:.c=.o)

# Target executable
TARGET = razzshell

# Test programs
TEST_LEXER = test_lexer
TEST_PARSER = test_parser

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete! RazzShell v2.0.0"

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET) $(TEST_LEXER) $(TEST_PARSER)
	@echo "Clean complete"

# Install to system
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "Installed to /usr/local/bin/razzshell"

# Uninstall from system
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstalled from /usr/local/bin"

# Run the shell
run: $(TARGET)
	./$(TARGET)

# Run in POSIX mode
run-posix: $(TARGET)
	./$(TARGET) --posix

# Run in Bash mode
run-bash: $(TARGET)
	./$(TARGET) --bash

# Build and run lexer test
test-lexer: src/test_lexer.c src/lexer.o
	$(CC) $(CFLAGS) src/test_lexer.c src/lexer.o -o $(TEST_LEXER)
	./$(TEST_LEXER)

# Build and run parser test
test-parser: src/test_parser.c src/lexer.o src/ast.o src/parser.o
	$(CC) $(CFLAGS) src/test_parser.c src/lexer.o src/ast.o src/parser.o -o $(TEST_PARSER)
	./$(TEST_PARSER)

# Show help
help:
	@echo "RazzShell Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build RazzShell (default)"
	@echo "  clean      - Remove build artifacts"
	@echo "  install    - Install to /usr/local/bin"
	@echo "  uninstall  - Remove from /usr/local/bin"
	@echo "  run        - Build and run in native mode"
	@echo "  run-posix  - Build and run in POSIX mode"
	@echo "  run-bash   - Build and run in Bash mode"
	@echo "  test-lexer - Build and run lexer tests"
	@echo "  test-parser - Build and run parser tests"
	@echo "  help       - Show this help message"

.PHONY: all clean install uninstall run run-posix run-bash test-lexer test-parser help

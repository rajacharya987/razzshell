# Design Document

## Overview

This design document outlines the architecture and implementation strategy for modernizing RazzShell into a production-ready, feature-rich shell. The design focuses on maintaining backward compatibility with existing RazzShell commands while adding POSIX/Bash compatibility, modern UX features, and a robust plugin ecosystem.

The modernization will be implemented in phases to allow incremental development and testing. The core architecture will be refactored to support multiple execution modes (native RazzShell, POSIX, Bash), a new parser with compatibility layers, and an extensible plugin system.

## Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     RazzShell Core                          │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │   Parser     │  │   Executor   │  │  Job Control │     │
│  │   Engine     │──│   Engine     │──│   Manager    │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
│         │                  │                  │             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ Compatibility│  │   Built-in   │  │   Plugin     │     │
│  │   Layers     │  │   Commands   │  │   System     │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
├─────────────────────────────────────────────────────────────┤
│                    Interactive Layer                        │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │Autocompletion│  │Autosuggestion│  │   Syntax     │     │
│  │   Engine     │  │   Engine     │  │ Highlighting │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

### Component Breakdown

#### 1. Parser Engine
- **Lexer**: Tokenizes input into lexical units
- **Parser**: Builds abstract syntax tree (AST) from tokens
- **Compatibility Translator**: Converts POSIX/Bash syntax to RazzShell AST
- **Expansion Engine**: Handles variable expansion, command substitution, globbing

#### 2. Executor Engine
- **Command Dispatcher**: Routes commands to appropriate handlers
- **Pipeline Manager**: Manages data flow between commands
- **Structured Data Handler**: Processes JSON/tabular data in pipelines
- **Sandbox Manager**: Enforces security policies for script execution

#### 3. Job Control Manager
- **Process Group Manager**: Manages process groups and terminal control
- **Signal Handler**: Handles SIGINT, SIGTSTP, SIGCHLD, etc.
- **Job Table**: Maintains state of background/foreground jobs
- **Notification System**: Alerts user of job status changes

#### 4. Plugin System
- **Plugin Loader**: Dynamically loads shared libraries
- **Plugin Registry**: Maintains metadata and dependencies
- **Plugin Manager (razz-pm)**: CLI tool for plugin management
- **API Layer**: Provides stable API for plugin development

#### 5. Interactive Layer
- **Completion Engine**: Context-aware tab completion
- **Suggestion Engine**: History-based inline suggestions
- **Syntax Highlighter**: Real-time syntax coloring
- **Prompt Renderer**: Customizable prompt with themes

## Components and Interfaces

### 1. Mode System

The shell will support three execution modes:

```c
typedef enum {
    MODE_RAZZSHELL,  // Native RazzShell mode
    MODE_POSIX,      // POSIX-compliant mode
    MODE_BASH        // Bash-compatible mode
} ShellMode;

typedef struct {
    ShellMode mode;
    bool strict_posix;
    bool errexit;      // set -e
    bool pipefail;     // set -o pipefail
    bool nounset;      // set -u
} ShellConfig;
```

**Interface:**
- `shell_set_mode(ShellMode mode)`: Switch execution mode
- `shell_get_config()`: Get current configuration
- `shell_apply_set_options(char *options)`: Apply set -e, -o, etc.

### 2. Enhanced Parser

The parser will be redesigned to support multiple syntaxes:

```c
typedef enum {
    TOKEN_WORD,
    TOKEN_OPERATOR,
    TOKEN_REDIRECT,
    TOKEN_PIPE,
    TOKEN_BACKGROUND,
    TOKEN_SEMICOLON,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_HEREDOC
} TokenType;

typedef struct Token {
    TokenType type;
    char *value;
    int line;
    int column;
} Token;

typedef struct ASTNode {
    enum {
        NODE_COMMAND,
        NODE_PIPELINE,
        NODE_LIST,
        NODE_SUBSHELL,
        NODE_FUNCTION,
        NODE_IF,
        NODE_WHILE,
        NODE_FOR
    } type;
    union {
        struct Command *command;
        struct Pipeline *pipeline;
        struct List *list;
    } data;
    struct ASTNode *next;
} ASTNode;
```

**Interface:**
- `lexer_tokenize(char *input)`: Convert input to tokens
- `parser_parse(Token *tokens)`: Build AST from tokens
- `parser_expand(ASTNode *ast)`: Perform expansions
- `parser_validate(ASTNode *ast)`: Validate syntax

### 3. Command Substitution

Support for both $() and backtick syntax:

```c
typedef struct {
    char *command;
    char *output;
    int exit_status;
} SubstitutionResult;
```

**Interface:**
- `expand_command_substitution(char *expr)`: Execute and capture output
- `expand_variable(char *var_name)`: Expand variable reference
- `expand_glob(char *pattern)`: Expand filename patterns

### 4. Module System

```c
typedef struct Module {
    char *name;
    char *path;
    HashTable *exports;  // Exported functions/variables
    bool loaded;
} Module;

typedef struct ModuleManager {
    HashTable *modules;  // name -> Module
    char **search_paths;
} ModuleManager;
```

**Interface:**
- `module_import(char *name)`: Load and cache module
- `module_export(char *name, void *value)`: Export from module
- `module_resolve_path(char *name)`: Find module file
- `module_get_export(Module *mod, char *name)`: Get exported item

### 5. Completion System

```c
typedef struct Completion {
    char *text;
    char *description;
    char *type;  // "command", "file", "option", etc.
} Completion;

typedef struct CompletionContext {
    char *line;
    int cursor_pos;
    char **words;
    int word_count;
    int current_word;
} CompletionContext;
```

**Interface:**
- `completion_get_suggestions(CompletionContext *ctx)`: Get completions
- `completion_parse_manpage(char *command)`: Extract options from man page
- `completion_filter_files(char *prefix, char *type)`: Filter by file type
- `completion_register_custom(char *command, CompletionFunc func)`: Register custom completer

### 6. Autosuggestion System

```c
typedef struct Suggestion {
    char *text;
    float score;  // Relevance score
    time_t last_used;
    int use_count;
} Suggestion;
```

**Interface:**
- `suggestion_get(char *prefix)`: Get best suggestion
- `suggestion_update_history(char *command)`: Update usage stats
- `suggestion_enable(bool enabled)`: Toggle feature
- `suggestion_configure(SuggestionConfig *config)`: Configure behavior

### 7. Syntax Highlighting

```c
typedef enum {
    HIGHLIGHT_COMMAND,
    HIGHLIGHT_BUILTIN,
    HIGHLIGHT_KEYWORD,
    HIGHLIGHT_STRING,
    HIGHLIGHT_NUMBER,
    HIGHLIGHT_OPERATOR,
    HIGHLIGHT_ERROR
} HighlightType;

typedef struct HighlightSpan {
    int start;
    int end;
    HighlightType type;
    char *color;
} HighlightSpan;
```

**Interface:**
- `highlight_line(char *line)`: Generate highlight spans
- `highlight_apply(char *line, HighlightSpan *spans)`: Apply colors
- `highlight_validate_command(char *cmd)`: Check if command exists
- `highlight_configure_theme(Theme *theme)`: Set color scheme

### 8. Plugin System

```c
typedef struct Plugin {
    char *name;
    char *version;
    char *author;
    char *description;
    void *handle;  // dlopen handle
    
    // Plugin API
    int (*init)(PluginContext *ctx);
    int (*cleanup)();
    int (*command)(char **args);
    Completion* (*complete)(CompletionContext *ctx);
} Plugin;

typedef struct PluginContext {
    ShellConfig *config;
    void (*register_command)(char *name, CommandFunc func);
    void (*register_completer)(char *cmd, CompletionFunc func);
    void (*log)(LogLevel level, char *msg);
} PluginContext;
```

**Interface:**
- `plugin_load(char *path)`: Load plugin from file
- `plugin_unload(char *name)`: Unload plugin
- `plugin_list()`: List loaded plugins
- `plugin_call_command(Plugin *p, char **args)`: Execute plugin command

### 9. Plugin Manager (razz-pm)

```c
typedef struct PluginPackage {
    char *name;
    char *version;
    char *description;
    char *author;
    char **dependencies;
    char *download_url;
    char *signature;
} PluginPackage;
```

**Interface:**
- `pm_install(char *name)`: Install plugin from repository
- `pm_uninstall(char *name)`: Remove plugin
- `pm_update(char *name)`: Update plugin
- `pm_list()`: List installed plugins
- `pm_search(char *query)`: Search repository
- `pm_verify(char *path)`: Verify plugin signature

### 10. Structured Data Pipeline

```c
typedef enum {
    DATA_TYPE_TEXT,
    DATA_TYPE_JSON,
    DATA_TYPE_TABLE
} DataType;

typedef struct DataStream {
    DataType type;
    void *data;
    size_t size;
} DataStream;

typedef struct TableColumn {
    char *name;
    char *type;
    void **values;
} TableColumn;

typedef struct Table {
    int row_count;
    int col_count;
    TableColumn *columns;
} Table;
```

**Interface:**
- `data_detect_type(char *input)`: Auto-detect data format
- `data_parse_json(char *json)`: Parse JSON to internal format
- `data_to_table(void *data, DataType type)`: Convert to table
- `data_filter(Table *table, char *expr)`: Filter rows
- `data_map(Table *table, char *expr)`: Transform columns
- `data_format(void *data, DataType output_type)`: Convert format

### 11. LSP Server

```c
typedef struct LSPServer {
    int socket_fd;
    DocumentStore *documents;
    DiagnosticEngine *diagnostics;
    CompletionEngine *completions;
} LSPServer;

typedef struct Diagnostic {
    int line;
    int column;
    char *message;
    char *severity;  // "error", "warning", "info"
} Diagnostic;
```

**Interface:**
- `lsp_start_server(int port)`: Start LSP server
- `lsp_analyze_document(char *uri)`: Analyze script
- `lsp_get_diagnostics(char *uri)`: Get errors/warnings
- `lsp_get_completions(char *uri, int line, int col)`: Get completions
- `lsp_get_hover(char *uri, int line, int col)`: Get hover info
- `lsp_goto_definition(char *uri, int line, int col)`: Find definition

### 12. Sandbox System

```c
typedef struct SandboxPolicy {
    char **allowed_paths;
    char **blocked_paths;
    bool allow_network;
    bool allow_exec;
    int max_memory_mb;
    int max_cpu_seconds;
} SandboxPolicy;

typedef struct SandboxContext {
    SandboxPolicy *policy;
    AuditLog *log;
    bool active;
} SandboxContext;
```

**Interface:**
- `sandbox_create(SandboxPolicy *policy)`: Create sandbox
- `sandbox_execute(SandboxContext *ctx, ASTNode *ast)`: Execute in sandbox
- `sandbox_check_path(SandboxContext *ctx, char *path)`: Verify path access
- `sandbox_check_network(SandboxContext *ctx)`: Verify network access
- `sandbox_audit_log(SandboxContext *ctx, char *event)`: Log security event

## Data Models

### 1. Command History

```c
typedef struct HistoryEntry {
    int id;
    char *command;
    time_t timestamp;
    int exit_status;
    int use_count;
    time_t last_used;
} HistoryEntry;

typedef struct HistoryDB {
    HistoryEntry *entries;
    int count;
    int capacity;
    HashTable *index;  // For fast lookup
} HistoryDB;
```

### 2. Configuration

```c
typedef struct RazzShellConfig {
    ShellMode default_mode;
    char *config_dir;
    char *plugin_dir;
    char *history_file;
    int history_size;
    
    // Feature flags
    bool enable_autosuggestions;
    bool enable_syntax_highlighting;
    bool enable_completions;
    
    // Theme
    Theme *theme;
    
    // Plugin settings
    char **enabled_plugins;
    char *plugin_repository_url;
} RazzShellConfig;
```

Configuration file format (TOML):

```toml
[shell]
default_mode = "razzshell"
history_size = 10000

[features]
autosuggestions = true
syntax_highlighting = true
completions = true

[theme]
name = "cyber"
command_color = "cyan"
error_color = "red"

[plugins]
enabled = ["git-prompt", "docker-complete", "aws-tools"]
repository = "https://plugins.razzshell.org"

[security]
sandbox_downloads = true
verify_signatures = true
```

### 3. Job Table

```c
typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} JobStatus;

typedef struct Job {
    int job_id;
    pid_t pgid;
    char *command;
    JobStatus status;
    int exit_code;
    bool notified;
} Job;

typedef struct JobTable {
    Job *jobs;
    int count;
    int next_id;
} JobTable;
```

## Error Handling

### Error Types

```c
typedef enum {
    ERROR_SYNTAX,
    ERROR_COMMAND_NOT_FOUND,
    ERROR_PERMISSION_DENIED,
    ERROR_FILE_NOT_FOUND,
    ERROR_EXEC_FAILED,
    ERROR_PLUGIN_LOAD_FAILED,
    ERROR_SANDBOX_VIOLATION
} ErrorType;

typedef struct Error {
    ErrorType type;
    char *message;
    char *file;
    int line;
    int column;
} Error;
```

### Error Handling Strategy

1. **Parse Errors**: Report with line/column information, suggest fixes
2. **Runtime Errors**: Print error message, set exit status, continue shell
3. **Plugin Errors**: Log error, disable plugin, notify user
4. **Sandbox Violations**: Block operation, log to audit, alert user
5. **Fatal Errors**: Clean up resources, save history, exit gracefully

### Error Recovery

- **Syntax Errors**: Highlight error location, suggest corrections
- **Command Not Found**: Suggest similar commands, check PATH
- **Permission Denied**: Suggest using sudo, check file permissions
- **Plugin Failures**: Fallback to built-in functionality

## Testing Strategy

### Unit Tests

- **Parser Tests**: Test lexer, parser, AST generation for all syntaxes
- **Expansion Tests**: Test variable expansion, command substitution, globbing
- **Completion Tests**: Test completion generation for various contexts
- **Plugin Tests**: Test plugin loading, unloading, API calls
- **Sandbox Tests**: Test security policy enforcement

### Integration Tests

- **POSIX Compatibility**: Run standard POSIX test suite
- **Bash Compatibility**: Test common Bash scripts and idioms
- **Pipeline Tests**: Test data flow through complex pipelines
- **Job Control Tests**: Test background jobs, signals, terminal control
- **Cross-platform Tests**: Test on Linux, macOS, Windows/WSL

### End-to-End Tests

- **Interactive Tests**: Simulate user input, verify output and behavior
- **Script Tests**: Run real-world scripts, verify correctness
- **Performance Tests**: Measure startup time, command execution time
- **Plugin Tests**: Test plugin installation, loading, execution
- **LSP Tests**: Test IDE integration with VSCode

### Test Infrastructure

```c
// Test framework
typedef struct TestCase {
    char *name;
    void (*setup)();
    void (*test)();
    void (*teardown)();
} TestCase;

// Assertions
#define ASSERT_EQ(a, b) assert_equal(__FILE__, __LINE__, a, b)
#define ASSERT_TRUE(x) assert_true(__FILE__, __LINE__, x)
#define ASSERT_NULL(x) assert_null(__FILE__, __LINE__, x)

// Mock objects
typedef struct MockShell {
    ShellConfig config;
    char *last_output;
    int last_exit_status;
} MockShell;
```

### Continuous Integration

- Run tests on every commit
- Test on multiple platforms (Linux, macOS, Windows)
- Test with different compilers (GCC, Clang, MSVC)
- Generate code coverage reports
- Run static analysis (cppcheck, clang-tidy)
- Run memory leak detection (valgrind)

## Performance Considerations

### Optimization Strategies

1. **Lazy Loading**: Load plugins and modules on first use
2. **Caching**: Cache parsed ASTs, completion results, man page data
3. **Efficient Data Structures**: Use hash tables for fast lookups
4. **Minimal Allocations**: Reuse buffers, use memory pools
5. **Fast Path**: Optimize common operations (simple commands, built-ins)

### Performance Targets

- **Startup Time**: < 100ms (cold start), < 50ms (warm start)
- **Command Execution**: < 1ms overhead for built-ins
- **Completion Generation**: < 50ms for typical cases
- **Syntax Highlighting**: < 10ms for typical command lines
- **History Search**: < 10ms for 10,000 entries

### Profiling and Monitoring

- Use `perf` for CPU profiling
- Use `valgrind --tool=massif` for memory profiling
- Add timing instrumentation for critical paths
- Monitor plugin performance impact
- Track startup time regression

## Security Considerations

### Threat Model

1. **Malicious Scripts**: Untrusted scripts attempting file system damage
2. **Malicious Plugins**: Plugins attempting to steal data or escalate privileges
3. **Command Injection**: User input containing malicious commands
4. **Path Traversal**: Attempts to access files outside allowed directories
5. **Resource Exhaustion**: Scripts consuming excessive CPU/memory

### Security Measures

1. **Sandbox Execution**: Restrict file system and network access
2. **Plugin Signing**: Verify cryptographic signatures on plugins
3. **Input Validation**: Sanitize all user input
4. **Privilege Separation**: Run plugins with minimal privileges
5. **Audit Logging**: Log all security-relevant events
6. **Safe Defaults**: Require explicit opt-in for dangerous operations

### Security Best Practices

- Never execute commands from untrusted sources without sandboxing
- Validate all file paths before access
- Use secure random number generation for temporary files
- Clear sensitive data from memory after use
- Follow principle of least privilege
- Keep dependencies up to date

## Cross-Platform Strategy

### Platform Abstraction Layer

```c
// Platform-specific implementations
#ifdef __linux__
    #include "platform/linux.h"
#elif __APPLE__
    #include "platform/macos.h"
#elif _WIN32
    #include "platform/windows.h"
#endif

// Unified interface
typedef struct Platform {
    int (*get_terminal_size)(int *rows, int *cols);
    char* (*get_home_dir)();
    char* (*get_config_dir)();
    bool (*is_executable)(char *path);
    void (*set_signal_handler)(int sig, SignalHandler handler);
} Platform;
```

### Platform-Specific Considerations

**Linux:**
- Use `/proc` for process information
- Support systemd integration
- Package for apt, yum, pacman

**macOS:**
- Use BSD-style process APIs
- Integrate with Homebrew
- Support macOS-specific paths

**Windows:**
- Use Windows API for process management
- Support both forward and backslashes in paths
- Provide native Windows installer
- Support PowerShell integration

**WSL:**
- Detect WSL environment
- Support Windows path translation
- Integrate with Windows filesystem

## Migration Path

### Backward Compatibility

- Keep all existing RazzShell commands working
- Add POSIX/Bash commands as aliases or alternatives
- Provide compatibility mode flag for strict POSIX behavior
- Document differences and migration guide

### Deprecation Strategy

- Mark deprecated features in documentation
- Print warnings for deprecated usage
- Provide migration tools/scripts
- Remove deprecated features in major version updates

### Version Strategy

- Use semantic versioning (MAJOR.MINOR.PATCH)
- Current: 1.0.2
- Next: 2.0.0 (major modernization)
- Maintain 1.x branch for critical fixes

# RazzShell v2.0.0 - Build Instructions

## Phase 1 Complete: Core Architecture & Mode System ✅

### What's New

1. **Multi-Mode Support**
   - Native RazzShell mode (default)
   - POSIX-compliant mode (`--posix`)
   - Bash-compatible mode (`--bash` or `-b`)

2. **POSIX Command Aliases**
   - Standard commands (cd, ls, echo, etc.) now work in POSIX/Bash modes
   - Automatic translation to RazzShell commands

3. **Runtime Mode Switching**
   - `mode` command to switch modes on the fly
   - `set` command for shell options (set -e, set -o pipefail, etc.)

4. **New Architecture**
   - Modular design with separate source files
   - `src/shell_config.c` - Mode and configuration management
   - `src/posix_compat.c` - POSIX command translation

### Building on Linux/macOS

```bash
# Install dependencies (if needed)
# Ubuntu/Debian:
sudo apt-get install build-essential libreadline-dev

# Fedora:
sudo dnf install gcc make readline-devel

# macOS:
brew install readline

# Build
make

# Or compile manually:
gcc -Wall -Wextra -g -I. -c src/shell_config.c -o src/shell_config.o
gcc -Wall -Wextra -g -I. -c src/posix_compat.c -o src/posix_compat.o
gcc -Wall -Wextra -g -I. -c razzshell.c -o razzshell.o
gcc razzshell.o src/shell_config.o src/posix_compat.o -o razzshell -lreadline -ldl -lncurses

# Run
./razzshell                # Native mode
./razzshell --posix        # POSIX mode
./razzshell --bash         # Bash mode
```

### Building on Windows (WSL)

Since you're on Windows, the easiest way is to use WSL (Windows Subsystem for Linux):

```bash
# In WSL Ubuntu:
sudo apt-get update
sudo apt-get install build-essential libreadline-dev

# Then follow Linux instructions above
```

### Testing the New Features

Once built, try these commands:

```bash
# Start in native mode
./razzshell

# Try RazzShell commands
list
where
say "Hello from RazzShell!"

# Switch to POSIX mode
mode posix

# Now standard commands work
ls
pwd
echo "Hello from POSIX mode!"

# Switch to Bash mode
mode bash

# Test set options
set -e
set -o pipefail
set

# Check current mode
mode
```

### What Works Now

✅ Mode system with three modes (RazzShell, POSIX, Bash)
✅ Command-line flags (--posix, --bash)
✅ Runtime mode switching with `mode` command
✅ POSIX command translation (35+ commands)
✅ Shell options (set -e, set -o pipefail, etc.)
✅ Mode-aware environment variables
✅ Backward compatibility with existing RazzShell commands

### Next Steps (Phase 2)

The next phase will add:
- Enhanced parser for command substitution ($(), backticks)
- Here-document support (<<)
- Bash [[ ]] test syntax
- Array support
- Improved expansion engine

### File Structure

```
razzshell/
├── razzshell.c              # Main shell (updated)
├── src/
│   ├── shell_config.h       # Mode system header
│   ├── shell_config.c       # Mode system implementation
│   ├── posix_compat.h       # POSIX compatibility header
│   └── posix_compat.c       # POSIX command translation
├── Makefile                 # Build system
├── BUILD_INSTRUCTIONS.md    # This file
└── README.md                # Original README
```

### Troubleshooting

**Issue**: `gcc: command not found`
**Solution**: Install GCC compiler for your platform

**Issue**: `readline/readline.h: No such file or directory`
**Solution**: Install readline development library

**Issue**: Compilation errors
**Solution**: Make sure you're on Linux/macOS or WSL, not native Windows

### Contributing

This is Phase 1 of the RazzShell modernization. See `.kiro/specs/razzshell-modernization/` for the full roadmap.

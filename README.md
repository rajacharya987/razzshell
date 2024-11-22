# RazzShell 1.0.1

RazzShell is a custom, user-friendly shell designed to provide a seamless command-line experience. It includes built-in commands, aliases, and the ability to load plugins for extending its functionality. The shell supports signal handling, job management, and advanced features such as command history and autocompletion.

---

## Table of Contents

- [Features](#features)
- [Differences Between RazzShell and Other Shells](#differences-between-razzshell-and-other-shells)
- [Installation](#installation)
  - [Prerequisites](#prerequisites)
  - [Installing on Ubuntu/Debian](#installing-on-ubuntudebian)
  - [Installing on Fedora](#installing-on-fedora)
  - [Installing on Arch Linux](#installing-on-arch-linux)
- [Usage](#usage)
  - [Starting RazzShell](#starting-razzshell)
  - [Shell Prompt](#shell-prompt)
  - [Built-in Commands](#built-in-commands)
    - [File and Directory Operations](#file-and-directory-operations)
    - [Process Management](#process-management)
    - [System Information](#system-information)
    - [Utilities](#utilities)
    - [Alias and Environment Variable Management](#alias-and-environment-variable-management)
    - [Session Management](#session-management)
    - [Miscellaneous Commands](#miscellaneous-commands)
  - [External Commands](#external-commands)
- [Examples](#examples)
- [License](#license)

---

## Features

- **Built-in Commands**: RazzShell includes a variety of built-in commands, such as `cd`, `exit`, `echo`, `pwd`, `jobs`, `fg`, `bg`, `kill`, `ls`, `cp`, `mv`, `rm`, `find`, `cat`, `grep`, `history`, `touch`, `mkdir`, `rmdir`, `chmod`, `chown`, `ps`, `whoami`, `ping`, `curl`, `sudo`, `save`, `load`, `bookmark`, `listbookmarks`, `visualize`, `sysinfo`, `diskusage`, `cpuusage`, `memusage`, `howto`, `makealias`, `removealias`, `setenv`, `printenv`, `clear`, `today`, `calendar`, `df`, `du`, `head`, `tail`, and `wc`.
- **Aliases**: Users can create custom aliases for frequently used commands or groups of commands, making it easier to execute complex tasks with a single command.
- **Plugin Support**: RazzShell allows users to load custom plugins dynamically, extending the shell's functionality and enabling the integration of third-party tools and scripts.
- **Signal Handling**: The shell supports signal handling for `SIGINT`, `SIGTSTP`, `SIGQUIT`, `SIGTTIN`, and `SIGTTOU`, providing a more robust and user-friendly experience.
- **Job Management**: RazzShell supports job management, allowing users to manage background jobs and bring them to the foreground as needed.
- **Command History**: The shell maintains a command history, which can be accessed and navigated using the Readline library.
- **Autocompletion**: RazzShell includes an autocomplete feature that suggests possible completions for commands, making it easier to find and execute commands without needing to remember their exact names.
- **Customizable**: Users can customize the shell's behavior and appearance by modifying the underlying functions, variables, and configuration settings.

---

## Differences Between RazzShell and Other Shells

RazzShell distinguishes itself from other Unix shells like Bash, Zsh, and Fish by offering:

- **Unique Command Names**: RazzShell uses custom names for its built-in commands (e.g., `change` instead of `cd`, `list` instead of `ls`). This encourages users to learn and use the shell's unique features.
- **Enhanced Visuals**: With colorful output for directory listings and error messages, RazzShell provides a more visually engaging experience compared to traditional shells.
- **Integrated Features**: RazzShell incorporates features like bookmarking commands, visualizing command flow, and session management directly into the shell without requiring additional scripting or plugins.
- **Simplified Command Set**: By providing a curated set of built-in commands, RazzShell aims to simplify command usage, making it more approachable for new users.
- **Consistent Behavior**: Custom commands are designed to behave consistently across different systems, reducing discrepancies that might occur with external commands in other shells.
- **Root Shell Integration**: RazzShell allows users to elevate to a root shell within the shell itself, maintaining the shell environment and features.

---

## Installation

### Prerequisites

- **GCC Compiler**: Ensure that GCC is installed on your system.
- **Readline Library**: RazzShell uses the readline library for command-line input.

#### On Ubuntu/Debian:

```bash

sudo apt-get update
sudo apt-get install build-essential libreadline-dev
On Fedora:
bash



 
sudo dnf install gcc make readline-devel
On Arch Linux:
bash



 
sudo pacman -S base-devel readline
Installing on Ubuntu/Debian
Clone the Repository:
bash



 
git clone https://github.com/rajacharya987/razzshell.git
Navigate to the Directory:
bash



 
cd razzshell
Compile the Source Code:
bash



 
gcc -o razzshell razzshell.c -lreadline -ldl
Install RazzShell:
bash



 
sudo cp razzshell /usr/local/bin/
sudo chmod +x /usr/local/bin/razzshell
Installing on Fedora
Clone the Repository:
bash



 
git clone https://github.com/rajacharya987/razzshell.git
Navigate to the Directory:
bash



 
cd razzshell
Compile the Source Code:
bash



 
gcc -o razzshell razzshell.c -lreadline -ldl
Install RazzShell:
bash



 
sudo cp razzshell /usr/local/bin/
sudo chmod +x /usr/local/bin/razzshell
Installing on Arch Linux
Option 1: Using yay (AUR Helper)
If you have yay installed, you can install RazzShell directly from the AUR:

bash



 
yay -S razzshell
Option 2: Building from Source
Clone the Repository:
bash



 
git clone https://github.com/rajacharya987/razzshell.git
Navigate to the Directory:
bash



 
cd razzshell
Compile the Source Code:
bash



 
gcc -o razzshell razzshell.c -lreadline -ldl
Install RazzShell:
bash



 
sudo cp razzshell /usr/local/bin/
sudo chmod +x /usr/local/bin/razzshell
Usage
Starting RazzShell
Run the compiled executable:

bash



 
razzshell
You will be greeted with a prompt similar to:




 
razzshell-$ [current_directory]>
Shell Prompt
Regular User: razzshell-$ [directory]>
Root User: razzshell-# [directory]>
The prompt displays the shell type ($ for regular users, # for root) and the current directory.

Built-in Commands
File and Directory Operations
change: Change the current working directory.
bash



 
change [directory]
list: List directory contents with color-coded output.
bash



 
list [-a] [directory]
-a: Include hidden files.

copy: Copy files from one location to another.

bash



 
copy [source] [destination]
move: Move or rename files.
bash



 
move [source] [destination]
delete: Delete files.
bash



 
delete [file]
makedir: Create a new directory.
bash



 
makedir [directory]
removedir: Remove an empty directory.
bash



 
removedir [directory]
create: Create a new file.
bash



 
create [filename]
readfile: Display the contents of a file.
bash



 
readfile [filename]
headfile: Display the first lines of a file.
bash



 
headfile [filename]
tailfile: Display the last lines of a file.
bash



 
tailfile [filename]
wordcount: Count words, lines, and characters in a file.
bash



 
wordcount [filename]
Process Management
showprocesses: Display running processes.
bash



 
showprocesses
terminate: Terminate a process.
bash



 
terminate [process id]
viewjobs: List active background jobs.
bash



 
viewjobs
sendtoback: Send a job to the background.
bash



 
sendtoback [job id]
bringtofront: Bring a background job to the foreground.
bash



 
bringtofront [job id]
System Information
where: Display the current working directory.
bash



 
where
whome: Show the current user.
bash



 
whome
sysinfo: Display system information.
bash



 
sysinfo
systemname: Print system name and information.
bash



 
systemname
today: Display the current date and time.
bash



 
today
calendar: Display the calendar.
bash



 
calendar
diskusage: Display disk usage.
bash



 
diskusage
diskfree: Display free disk space.
bash



 
diskfree
cpuusage: Display CPU usage.
bash



 
cpuusage
memusage: Display memory usage.
bash



 
memusage
Utilities
say: Display a line of text (similar to echo).
bash



 
say [text]
searchfile: Search for files in a directory hierarchy.
bash



 
searchfile [filename]
searchtext: Search for a pattern in files.
bash



 
searchtext [pattern] [file]
fetchurl: Fetch content from a URL.
bash



 
fetchurl [URL]
pinghost: Ping a host to check connectivity.
bash



 
pinghost [hostname]
visualize: Visualize the command flow.
bash



 
visualize [command]
repeat: Repeat a command multiple times.
bash



 
repeat [count] [command]
Alias and Environment Variable Management
makealias: Create a command alias.
bash



 
makealias [alias_name] [command]
removealias: Remove a command alias.
bash



 
removealias [alias_name]
aliases: List all aliases.
bash



 
aliases
setenv: Set an environment variable.
bash



 
setenv [VAR] [VALUE]
unsetenv: Unset an environment variable.
bash



 
unsetenv [VAR]
printenv: Print all environment variables.
bash



 
printenv
Session Management
save: Save the current session history.
bash



 
save
load: Load a saved session history.
bash



 
load
bookmark: Bookmark a command.
bash



 
bookmark [command]
listbookmarks: List all bookmarked commands.
bash



 
listbookmarks
commands: Show command history.
bash



 
commands
history_clear: Clear command history.
bash



 
history_clear
Miscellaneous Commands
clear: Clear the terminal screen.
bash



 
clear
howto: Display help for commands.
bash



 
howto
sudo: Run a command as root.
bash



 
sudo [command]
sudo_su: Switch to root shell within RazzShell.
bash



 
sudo su
quit: Exit the shell.
bash



 
quit
External Commands
RazzShell supports executing external programs installed on your system. If a command is not recognized as a built-in command, RazzShell will attempt to execute it as an external command.

Example:

bash



 
razzshell-$ [directory]> nano filename.txt
Examples
Changing Directory:

bash



 
razzshell-$ [directory]> change /path/to/directory
Listing Files with Colors:

bash



 
razzshell-$ [directory]> list
Listing All Files Including Hidden Files:

bash



 
razzshell-$ [directory]> list -a
Creating and Using an Alias:

bash



 
razzshell-$ [directory]> makealias ll list
razzshell-$ [directory]> ll
Setting and Using an Environment Variable:

bash



 
razzshell-$ [directory]> setenv MYVAR HelloWorld
razzshell-$ [directory]> say $MYVAR
Repeating a Command:

bash



 
razzshell-$ [directory]> repeat 5 say "Hello, World!"
Elevating to Root User within RazzShell:

bash



 
 ```bash
razzshell-$ [directory]> sudo su
[sudo] password for user:
razzshell-# [directory]>
Exiting the Shell:

bash



 
razzshell-$ [directory]> quit
License
RazzShell is released under the MIT License.

Disclaimer: Use RazzShell at your own risk. The author is not responsible for any damage or data loss resulting from the use of this shell.

Thank you for choosing RazzShell!

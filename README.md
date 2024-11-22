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




 
sudo dnf install gcc make readline-devel
On Arch Linux:




 
sudo pacman -S base-devel readline
Installing on Ubuntu/Debian
Clone the Repository:




 
git clone https://github.com/rajacharya987/razzshell.git
Navigate to the Directory:




 
cd razzshell
Compile the Source Code:




 
gcc -o razzshell razzshell.c -lreadline -ldl
Install RazzShell:




 
sudo cp razzshell /usr/local/bin/
sudo chmod +x /usr/local/bin/razzshell
Installing on Fedora
Clone the Repository:




 
git clone https://github.com/rajacharya987/razzshell.git
Navigate to the Directory:




 
cd razzshell
Compile the Source Code:




 
gcc -o razzshell razzshell.c -lreadline -ldl
Install RazzShell:




 
sudo cp razzshell /usr/local/bin/
sudo chmod +x /usr/local/bin/razzshell
Installing on Arch Linux
Option 1: Using yay (AUR Helper)
If you have yay installed, you can install RazzShell directly from the AUR:





 
yay -S razzshell
Option 2: Building from Source
Clone the Repository:




 
git clone https://github.com/rajacharya987/razzshell.git
Navigate to the Directory:




 
cd razzshell
Compile the Source Code:




 
gcc -o razzshell razzshell.c -lreadline -ldl
Install RazzShell:




 
sudo cp razzshell /usr/local/bin/
sudo chmod +x /usr/local/bin/razzshell
Usage
Starting RazzShell
Run the compiled executable:





 
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




 
change [directory]
list: List directory contents with color-coded output.




 
list [-a] [directory]
-a: Include hidden files.

copy: Copy files from one location to another.





 
copy [source] [destination]
move: Move or rename files.




 
move [source] [destination]
delete: Delete files.




 
delete [file]
makedir: Create a new directory.




 
makedir [directory]
removedir: Remove an empty directory.




 
removedir [directory]
create: Create a new file.




 
create [filename]
readfile: Display the contents of a file.




 
readfile [filename]
headfile: Display the first lines of a file.




 
headfile [filename]
tailfile: Display the last lines of a file.




 
tailfile [filename]
wordcount: Count words, lines, and characters in a file.




 
wordcount [filename]
Process Management
showprocesses: Display running processes.




 
showprocesses
terminate: Terminate a process.




 
terminate [process id]
viewjobs: List active background jobs.




 
viewjobs
sendtoback: Send a job to the background.




 
sendtoback [job id]
bringtofront: Bring a background job to the foreground.




 
bringtofront [job id]
System Information
where: Display the current working directory.




 
where
whome: Show the current user.




 
whome
sysinfo: Display system information.




 
sysinfo
systemname: Print system name and information.




 
systemname
today: Display the current date and time.




 
today
calendar: Display the calendar.




 
calendar
diskusage: Display disk usage.




 
diskusage
diskfree: Display free disk space.




 
diskfree
cpuusage: Display CPU usage.




 
cpuusage
memusage: Display memory usage.




 
memusage
Utilities
say: Display a line of text (similar to echo).




 
say [text]
searchfile: Search for files in a directory hierarchy.




 
searchfile [filename]
searchtext: Search for a pattern in files.




 
searchtext [pattern] [file]
fetchurl: Fetch content from a URL.




 
fetchurl [URL]
pinghost: Ping a host to check connectivity.




 
pinghost [hostname]
visualize: Visualize the command flow.




 
visualize [command]
repeat: Repeat a command multiple times.




 
repeat [count] [command]
Alias and Environment Variable Management
makealias: Create a command alias.




 
makealias [alias_name] [command]
removealias: Remove a command alias.




 
removealias [alias_name]
aliases: List all aliases.




 
aliases
setenv: Set an environment variable.




 
setenv [VAR] [VALUE]
unsetenv: Unset an environment variable.




 
unsetenv [VAR]
printenv: Print all environment variables.




 
printenv
Session Management
save: Save the current session history.




 
save
load: Load a saved session history.




 
load
bookmark: Bookmark a command.




 
bookmark [command]
listbookmarks: List all bookmarked commands.




 
listbookmarks
commands: Show command history.




 
commands
history_clear: Clear command history.




 
history_clear
Miscellaneous Commands
clear: Clear the terminal screen.




 
clear
howto: Display help for commands.




 
howto
sudo: Run a command as root.




 
sudo [command]
sudo_su: Switch to root shell within RazzShell.




 
sudo su
quit: Exit the shell.




 
quit
External Commands
RazzShell supports executing external programs installed on your system. If a command is not recognized as a built-in command, RazzShell will attempt to execute it as an external command.

Example:





 
razzshell-$ [directory]> nano filename.txt
Examples
Changing Directory:





 
razzshell-$ [directory]> change /path/to/directory
Listing Files with Colors:





 
razzshell-$ [directory]> list
Listing All Files Including Hidden Files:





 
razzshell-$ [directory]> list -a
Creating and Using an Alias:





 
razzshell-$ [directory]> makealias ll list
razzshell-$ [directory]> ll
Setting and Using an Environment Variable:





 
razzshell-$ [directory]> setenv MYVAR HelloWorld
razzshell-$ [directory]> say $MYVAR
Repeating a Command:





 
razzshell-$ [directory]> repeat 5 say "Hello, World!"
Elevating to Root User within RazzShell:





 
 ```
razzshell-$ [directory]> sudo su
[sudo] password for user:
razzshell-# [directory]>
Exiting the Shell:





 
razzshell-$ [directory]> quit
License
RazzShell is released under the MIT License.

Disclaimer: Use RazzShell at your own risk. The author is not responsible for any damage or data loss resulting from the use of this shell.

Thank you for choosing RazzShell!

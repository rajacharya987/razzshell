RazzShell

RazzShell is a custom Unix shell written in C, designed to provide a unique and user-friendly command-line interface. It offers a variety of built-in commands, colorful output, command aliases, environment variable support, and the ability to execute external programs seamlessly. RazzShell aims to combine the functionality of traditional shells with custom features to enhance the user experience.
Table of Contents

    Features
    Differences Between RazzShell and Other Shells
    Installation
    Usage
        Starting RazzShell
        Shell Prompt
        Built-in Commands
            File and Directory Operations
            Process Management
            System Information
            Utilities
            Alias and Environment Variable Management
            Session Management
            Miscellaneous Commands
        External Commands
    Examples
    License

Features

    Custom Shell Prompt: Displays the current directory in the prompt for easy navigation.
    Colorful Output: Directory listings and error messages are color-coded for better readability.
    Unique Command Names: Built-in commands have custom names to avoid conflicts with external commands.
    External Command Execution: Supports running any external program installed on your system.
    Command Aliases: Allows creating shortcuts for frequently used commands.
    Environment Variable Support: Set, unset, and use environment variables within the shell.
    Command History: Navigate through your command history using the up/down arrow keys.
    Root Privilege Elevation: Switch to root user within RazzShell using sudo su.
    Signal Handling: Handles interrupts like Ctrl+C gracefully without exiting the shell.
    Background and Foreground Job Control: Manage jobs running in the background or foreground.
    Bookmarking: Bookmark frequently used commands for quick access.
    Session Saving and Loading: Save your session history and load it later.
    Command Flow Visualization: Visualize the flow of command execution.

Differences Between RazzShell and Other Shells

RazzShell distinguishes itself from other Unix shells like Bash, Zsh, and Fish by offering:

    Unique Command Names: RazzShell uses custom names for its built-in commands (e.g., change instead of cd, list instead of ls). This encourages users to learn and use the shell's unique features.

    Enhanced Visuals: With colorful output for directory listings and error messages, RazzShell provides a more visually engaging experience compared to traditional shells.

    Integrated Features: RazzShell incorporates features like bookmarking commands, visualizing command flow, and session management directly into the shell without requiring additional scripting or plugins.

    Simplified Command Set: By providing a curated set of built-in commands, RazzShell aims to simplify command usage, making it more approachable for new users.

    Consistent Behavior: Custom commands are designed to behave consistently across different systems, reducing discrepancies that might occur with external commands in other shells.

    Root Shell Integration: RazzShell allows users to elevate to a root shell within the shell itself, maintaining the shell environment and features.

Installation
Prerequisites

    GCC Compiler: Ensure that GCC is installed on your system.

    Readline Library: RazzShell uses the readline library for command-line input.

    On Debian-based systems, install it using:

    bash

    sudo apt-get install libreadline-dev

Steps

    Clone the Repository

    bash

git clone https://github.com/rajacharya987/razzshell.git

Navigate to the Directory

bash

cd razzshell

Compile the Source Code

bash

    gcc -o razzshell razzshell.c -lreadline

    If you encounter errors related to readline, ensure that the readline library is installed on your system.

Usage
Starting RazzShell

Run the compiled executable:

bash

./razzshell

You will be greeted with a prompt similar to:

css

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

Signal Handling

    Ctrl+C: Interrupts the current command but keeps the shell running.
    Ctrl+D: Exits the shell.

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

razzshell-$ [directory]> sudo su
[sudo] password for user:
razzshell-# [directory]>

Exiting the Shell:

bash

    razzshell-$ [directory]> quit

License

RazzShell is released under the MIT License.

Disclaimer: Use RazzShell at your own risk. The author is not responsible for any damage or data loss resulting from the use of this shell.

# Basic-Shell

This project implemented to mimic some of the functionalities of the Shell 

## Features implemented
Implemented a command line interpreter (CLI) or, as it is more commonly known, a shell. The shell  operates in this basic way: when you type in a command (in response to its prompt), the shell creates a child process that executes the command you entered and then prompts for more user input when it has finished.

The basic shell command can run in batch mode (takes input commands from a batch file and executes one after the other) or interactive mode. 

Currently supported functionalities : 

1) All built-in commands such as ls, cat etc 

2) Implemented feature to add and reset the path by using 
    By default path will be set to /bin

    ```bash
    path /path1 /path2
    ```
    if paths are left blank none of the path will be reset to empty. 

3) Redirection 
    Supports redirection of the output and standard error to a file. 

4) Supports if then conditions in the shell command 
    Format : 
    ```bash
    prompt> if some_command == 0 then some_other_command fi
    ```
    Same format can be extended to nested if conditions

## Contact
Prajwal Hegde - hegde.cs@wisc.edu
Project Link: https://github.com/hegdeprajwal/Basic-Shell


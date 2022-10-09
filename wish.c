#include <stdio.h> 
#include <stdlib.h> 
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

void printError_exit() {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(0);
}

void printError_exit_1() {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
}

void printError() {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
}

void splitCommand ( char ** tokens, char command[512], size_t buffer_size, char* delim, int* counts) {
    char* token = malloc ( buffer_size * sizeof(char));
    char new_command [512];
    strcpy(new_command, command);
    token = strtok(new_command, delim);
    //If the allocated size is not enough double the size
    int count = 0;
    while (token != NULL) {
        tokens[count++] = strdup(token);

        if (count >= buffer_size) {
            buffer_size += buffer_size;
            tokens = realloc(tokens, buffer_size * sizeof(char*));
            if (!tokens) {
                printError_exit();
            }
        }
        token = strtok(NULL, delim);
    }
    tokens[count] = NULL;
    *counts = count;
    return ;
}

int execBuiltIns ( char **paths, size_t buffer_size, char **arguments, int redirect, char* filepath ) {

    //check for path 
    if ( strcmp (arguments[0], "path") == 0) {
        //copy the rest of tokens to path
        if ( arguments[1] == NULL ) {
            memset(paths, '\0', buffer_size);
        }
        for ( int i = 1; arguments[i] != NULL ; i++) {
            paths[i-1] = arguments[i];
        }
        return 0;
    }

    //check for cd 
    else if (strcmp(arguments[0] , "cd") == 0) {
        if (arguments[2] != NULL) {
            printError();
        }
        else if (chdir(arguments[1]) != 0) {
            printError();             
        }
        return 0;   
    }

    //check for exit
    else if ( strcmp(arguments[0], "exit") == 0) {
        if (arguments[1] != NULL) {
            printError();
        }
        else {
            exit(0);
        }
        return 0;
    }

    char** path_ptr = NULL;
    int flag1 = 0;
    for ( path_ptr = paths ; *path_ptr != NULL ; path_ptr++) {
        //Append the ls to the paths and check 
        char* ls_path = malloc (buffer_size * sizeof(char*));
        strcpy(ls_path, *path_ptr);
        strcat(ls_path, "/");
        strcat(ls_path, arguments[0]);
        if ( access(ls_path, X_OK) == 0 ) {
            flag1 = 1;

            //child process starts here
            pid_t pid = fork();
            if ( pid == 0) {
                if ( redirect ) {
                    int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    dup2(fd, fileno(stdout));
                    dup2(fd, fileno(stderr));                                
                }
                if ( execv(ls_path , arguments) != 0) {
                    printError();
                } 

            }
            int status; 
            waitpid(pid, &status, 0);
            return status;
        }
    }
    // if no executables in this path print error 
    if (!flag1 ) {
        printError();
    }
    return -1;
}

int exec_shell_command ( char *command, char **paths, size_t buffer_size ) {

   //if redirect split w.r.t redirect
    char** commands = malloc (buffer_size * sizeof(char*));
    //get the file path and check for more than 1 file and no file mentioned
    char *filepath = NULL;
    int redirect = 0;
    int count = 0;
    if ( strchr(command, '>') != NULL ) {
        splitCommand (commands, command, buffer_size, ">" , &count);
        //first string is > 
        if ( strcmp((commands[0]), ">") == 0 ) {
            printError_exit();
        }
        // if output file is not present
        else if ( !(commands[1]) || commands[2])  {
            printError_exit();        
        }
        else {
            command = commands[0];
            char** filepath_arr = malloc (buffer_size * sizeof(char*));
            splitCommand ( filepath_arr, commands[1], buffer_size, " " , &count);

            //more than one output file
            if (filepath_arr[1] ){
                printError_exit();
            }
            filepath = filepath_arr[0];
        }
        redirect = 1;         
    }
    
    //Tokenize the string
    char** tokens = malloc (buffer_size * sizeof(char*));
    splitCommand ( tokens, command, buffer_size, " " , &count);

    //Copying the tokens to arguments 
    char** arguments= malloc(buffer_size * sizeof (char*));
    int i =0;
    char** token_ptr = NULL;
    for ( token_ptr = tokens ; *token_ptr != NULL ; token_ptr++) {
        arguments[i++] = *token_ptr;
    }
    arguments[i] = NULL;

    if ( !tokens[0]) {
        return 0;
    }


    int child_status = execBuiltIns(paths, buffer_size, arguments, redirect, filepath);
    return child_status;
}

int check_correctness ( char *command_str, size_t buffer_size) {
        char** tokens2 = malloc (buffer_size * sizeof(char*));
        int count = 0;
        splitCommand ( tokens2, command_str, buffer_size, " ", &count );

        int fi_count = 0;
        int if_count = 0;
        for ( int i = count-1; i>0 ; i--) {
            if ( strcmp(tokens2[i], "fi" ) == 0) fi_count++;
            else break;
        }

        for ( int i = 0; i < count ; i++) {
            if ( strcmp(tokens2[i], "if" ) == 0) if_count++;
        }

        if( fi_count !=  if_count) {
            printError_exit();
        }

        return 1;

}

void shell_execute ( char *command, char **paths, size_t buffer_size ) {

    //See if it contains if condition
    int correct = 0;
    if ( strstr(command, "if") != NULL ) {
        // handle if checks        
        correct = check_correctness ( command,  buffer_size);
        //execute shell commands separated by if or then 
        if (correct) {
            int temp_index;
            temp_index = 0;
            char** tokens3 = malloc (buffer_size * sizeof(char*));
            int count = 0;
            splitCommand ( tokens3, command, buffer_size, " ", &count );

            while ( tokens3[temp_index] && (temp_index < count )) {
                char * operator = NULL;
                int value = 0;
                int if_flag = 0;
                char* new_command = malloc (buffer_size * sizeof(char));

                if (( strcmp(tokens3[temp_index], "then") == 0 ) || ( strcmp(tokens3[temp_index], "fi") == 0 ) ) {
                    temp_index ++; 
                    continue;
                }
                else if ( strcmp(tokens3[temp_index], "if") == 0 ) {

                    temp_index ++; // increment to skip if
                    while( (strcmp(tokens3[temp_index], "==") != 0 ) && (strcmp(tokens3[temp_index], "!=") != 0) && strcmp(tokens3[temp_index], "&|") != 0) {
                        strcat(new_command, " ");    
                        strcat(new_command, tokens3[temp_index++]);
                    }
                    operator = tokens3[temp_index++]; 
                    value = atoi (tokens3[temp_index++]);
                    if_flag = 1; // flag if 
                }
                else {
                    // All the if encountered fi throw error 
                    while( strcmp(tokens3[temp_index], "fi") != 0) {
                        strcat(new_command, " ");
                        strcat(new_command, tokens3[temp_index++]);
                    }      
                }

                // call the process with extracted command
                int child_status = exec_shell_command(new_command, paths, buffer_size);

                // compare the return value 
                if (if_flag) {
                    int success = 0;
                    if(WIFEXITED(child_status)) {
                        child_status = WEXITSTATUS(child_status);
                    }
        
                    if( strcmp(operator, "==") == 0) {
                        if ( child_status == value) 
                        success = 1 ;
                    }
                    else if (strcmp(operator, "!=") == 0) {
                        if ( child_status != value) 
                        success = 1 ;      
                    }
                    else {
                        printError_exit();  
                    }
                    if ( !success ) {
                        return;
                    }
                }
            }
        }
    }
    else {
        int status = exec_shell_command ( command , paths, buffer_size);
    }
}

int main(int argc, char *argv[]) {

    size_t buffer_size = 512;
    char** paths = malloc (buffer_size * sizeof(char*));

    //default path
    paths[0] = "/bin";

    if ( argc == 2 ) {
        FILE * fp;
        char * line = NULL;
        size_t len = 0;
        ssize_t read;

        fp = fopen(argv[1], "r");  
    
        if (fp == NULL) {
            printError_exit_1(); 
        }
        while ((read = getline(&line, &len, fp)) != -1) {
            line[strlen(line)-1] ='\0';
            shell_execute (line, paths, buffer_size);  
        }
    }

    //Multiple batchmode shell files
    else if ( argc > 2 ) {
        printError_exit_1();
    }
    else {
        while(1) {
            printf("my_prompt > ");
            char* buffer = (char *) malloc(buffer_size * sizeof(char));

            if (buffer == NULL) {
                printError_exit_1();
            }

            //get the input argument string
            if (getline(&buffer, &buffer_size, stdin) == -1){
                if (feof(stdin)) {
                    exit(0);  // We recieved an EOF
                } else  {
                    perror("readline");
                    exit(1);
                }
            }
            buffer[strlen(buffer)-1] ='\0';
            shell_execute (buffer, paths, buffer_size);
        }
    }
}
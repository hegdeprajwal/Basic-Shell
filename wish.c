#include <stdio.h> 
#include <stdlib.h> 
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

void splitCommand ( char ** tokens, char* command, size_t buffer_size) {
    char* token = strtok(command, " ");
    //If the allocated size is not enough double the size
    int count = 0;
    while (token != NULL) {
        tokens[count++] = strdup(token);

        if (count >= buffer_size) {
            buffer_size += buffer_size;
            tokens = realloc(tokens, buffer_size * sizeof(char*));
            if (!tokens) {
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1);
            }
        }
        token = strtok(NULL, " ");
    }
    tokens[count] = NULL;
    return ;
}

void shell_execute ( char *command, char **paths, size_t buffer_size ) {

    //Tokenize the string
    char** tokens = malloc (buffer_size * sizeof(char*));
    splitCommand ( tokens, command, buffer_size );

    //Copying the tokens to arguments 
    char** arguments= malloc(buffer_size * sizeof (char*));
    int i =0;
    char** token_ptr = NULL;
    for ( token_ptr = tokens ; *token_ptr != NULL ; token_ptr++) {
        arguments[i++] = *token_ptr;
    }
    arguments[i] = NULL;

    //Iterate and search if redirection operator is present 
    token_ptr = NULL;
    int redirect = 0;
    int cnt = 0;
    for ( token_ptr = arguments ; *token_ptr != NULL ; token_ptr++, cnt++) {
        if ( strcmp(*token_ptr, ">") == 0 ) {
            redirect = 1;
            break;
        }
    }

    //get the file path and check for more than 1 file and no file mentioned
    char *filepath = NULL;
    if ( redirect ) {
        arguments[cnt] = '\0';
        if (*(token_ptr+1) == NULL ) {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
        else if ( *(token_ptr+2) != NULL) {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        } 
        else {
            cnt++ ; 
            filepath = strdup(arguments[cnt++]);
            arguments[cnt] = '\0';
        } 
    
        }    

        // check for path 
        if ( strcmp (tokens[0], "path") == 0) {
            //copy the rest of tokens to path
            if ( arguments[1] == NULL ) {
                memset(paths, '\0', buffer_size);
            }
            for ( int i = 1; arguments[i] != NULL ; i++) {
                paths[i-1] = arguments[i];
            }
        }
        // check for cd 
        else if (strcmp(tokens[0] , "cd") == 0) {
            if (tokens[2] != NULL) {
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
           else if (chdir(tokens[1]) != 0) {
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));                
           }   
        }
        //check for exit
        else if ( strcmp(tokens[0], "exit") == 0) {
            if (tokens[1] != NULL) {
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
            else {
                exit(0);
            }
        }
        //check for other
        else {
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
                            char error_message[30] = "An error has occurred\n";
                            write(STDERR_FILENO, error_message, strlen(error_message));
                        } 

                    }
                    int status; 
                    waitpid(pid, &status, 0);
                }
                // if no executables in this path print error 
                if (!flag1 ) {
                            char error_message[30] = "An error has occurred\n";
                            write(STDERR_FILENO, error_message, strlen(error_message));
                }
            
            } 
        }
    }

int main(int argc, char *argv[]) {

    size_t buffer_size = 64;
    char** paths = malloc (buffer_size * sizeof(char*));
    paths[0] = "/bin";

    if ( argc == 2 ) {
        FILE * fp;
        char * line = NULL;
        size_t len = 0;
        ssize_t read;

        fp = fopen(argv[1], "r");    
    
        if (fp == NULL) {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1); 
        }
        
        while ((read = getline(&line, &len, fp)) != -1) {
            line[strlen(line)-1] ='\0';
            shell_execute (line, paths, buffer_size);  
        }
    }
    else {
        while(1) {
            printf("wish > ");
            char* buffer = (char *) malloc(buffer_size * sizeof(char));

            if (buffer == NULL) {
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1);
            }

            //get the input argument string
            if (getline(&buffer, &buffer_size, stdin) == -1){
                if (feof(stdin)) {
                    exit(EXIT_SUCCESS);  // We recieved an EOF
                } else  {
                    perror("readline");
                    exit(EXIT_FAILURE);
                }
            }
            buffer[strlen(buffer)-1] ='\0';
            shell_execute (buffer, paths, buffer_size);
        }
    }
}
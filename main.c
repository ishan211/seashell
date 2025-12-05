#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Func Declarations for builtin shell cmd
int csh_cd(char **args);
int csh_help(char **args);
int csh_exit(char **args);

// Builtin cmd and their funcs
char *builtin_str[] = {
    "cd",
    "help",
    "exit"    
};

int (*builtin_func[]) (char **) = {
    &csh_cd,
    &csh_help,
    &csh_exit
};

int csh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

// Builtin cmd implementation

/*
    Change Directory
    args: array of arguments; args[0] = "cd", args[1] = directory
    return: Always 1, to continue executing
*/
int csh_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: cd: missing argument\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("lsh");
        }
    }
    return 1;
}

/*
    Help
    args: array of arguments
    return: Always 1, to continue executing
*/
int csh_help(char **args) {
    int i;
    printf("Ishan Leung's C Shell\n");
    printf("Type program names and arguments, and press the enter key.\n");
    printf("The following are built-in commands:\n");

    for (i = 0; i < csh_num_builtins(); i++) {
        printf("  %s\n", builtin_str[i]);
    }

    printf("Use the man command for info on other programs. \n");
    return 1;
}

/*
    Exit
    args: array of arguments
    return: Always 0, to end the execution
*/

int csh_exit(char **args) {
    return 0;
}

/*
    Launch a program and wait for it to terminate
    args: Null terminated list of arguments (inc program)
    Return: 1, always
*/

int csh_launch(char **args) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        // CHILD
        if (execvp(args[0], args) == -1) {
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        perror("lsh");
    } else {
        // PARENT
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

/*
    Execute shell built-in or launch program
    args: Null terminated list of arguments
    return: 1 if shell should continue executing, 0 if it should terminate
*/
int csh_execute(char **args) {
    int i;

    if (args[0] == NULL) {
        // empty cmd was entered
        return 1;
    }

    for (i = 0; i < csh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return csh_launch(args);
}

/*
    Read a line of input from stdin
    return: The line read
*/
char *csh_read_line(void) {
    #ifdef CSH_USE_STD_GETLINE
        char *line = NULL;
        ssize_t bufsize = 0; // use getline to alloc buffer
        if (getline(&line, &bufsize, stdin) == -1) {
            if (feof(stdin)) {
                exit(EXIT_SUCCESS); // EOF
            } else {
                perror("lsh: getline");
                exit(EXIT_FAILURE);
            }
        }

        return line;
    #else
    #define CSH_RL_BUFSIZE 1024
        int bufsize = CSH_RL_BUFSIZE;
        int position = 0;
        char *buffer = malloc(sizeof(char) * bufsize);
        int c;

        if (!buffer) {
            fprintf(stderr, "lsh: allocation error\n");
            exit(EXIT_FAILURE);
        }

        while(1) {
            // Read char
            c = getchar();

            if (c == EOF) {
                exit(EXIT_SUCCESS);
            } else if (c == '\n') {
                buffer[position] = '\0';
                return buffer;
            } else {
                buffer[position] = c;
            }
            position++;

            // If buffer is full, reallocate
            if (position >= bufsize) {
                bufsize += CSH_RL_BUFSIZE;
                buffer = realloc(buffer, bufsize);
                if (!buffer) {
                    fprintf(stderr, "lsh: allocation error\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    #endif
}

#define CSH_TOK_BUFSIZE 64
#define CSH_TOK_DELIM " \t\r\n\a"

/*
    Split a line into tokens (kinda goofy, but it works)
    arg: The line to split
    return: Null terminated array of tokens
*/
char **csh_split_line(char *line) {
    int bufsize = CSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token, **tokens_backup;

    if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, CSH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += CSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, CSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

/*
    Loop getting inp and execute it
*/
void csh_loop(void) {
    char *line;
    char **args;
    int status;

    do {
        printf("> ");
        line = csh_read_line();
        args = csh_split_line(line);
        status = csh_execute(args);

        free(line);
        free(args);
    } while (status);
}

/*
    Main entry point
    args: argc (argument count), argv (argument vector)
    return: status code
*/
int main(int argc, char **argv) {
    // Load any config files
    // Run command loop
    csh_loop();

    // Perform cleanup and shutdown
    return EXIT_SUCCESS;
}

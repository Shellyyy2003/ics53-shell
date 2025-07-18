#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>


#define MAX_LINE 80
#define MAX_ARGC 80

pid_t fg_pid = -1;

void sigint_handler(int sig) {
    if (fg_pid > 0) {
        kill(fg_pid, SIGINT);
    }
}

void execute_foreground_command(char **args) {
        pid_t pid = fork();
        if (pid == 0) {
            if (strchr(args[0], '/') == NULL) {
                char path_buf[256];
                snprintf(path_buf, sizeof(path_buf), "./%s", args[0]);
                execv(path_buf, args);
            }
    
            execvp(args[0], args);
    
            perror("execvp/execv failed");
            exit(1);
        } else if (pid > 0) {
            fg_pid = pid;
            waitpid(pid, NULL, 0);
            fg_pid = -1;
        } else {
            perror("fork failed");
        }
    }


void print_prompt() {
    printf("prompt > ");
    fflush(stdout);
}

int main() {
    signal(SIGINT, sigint_handler);

    char line[MAX_LINE];
    char *args[MAX_ARGC];

    while (1) {
        print_prompt();

        if (fgets(line, MAX_LINE, stdin) == NULL) {
            perror("fgets failed");
            continue;
        }

        line[strcspn(line, "\n")] = 0;

        if (strlen(line) == 0)
            continue;

        int argc = 0;
        char *token = strtok(line, " \t");
        while (token != NULL && argc < MAX_ARGC - 1) {
            args[argc++] = token;
            token = strtok(NULL, " \t");
        }
        args[argc] = NULL;

        if (args[0] == NULL)
            continue;

        if (strcmp(args[0], "quit") == 0) {
            break;
        }
        else if (strcmp(args[0], "pwd") == 0) {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("getcwd failed");
            }
        }
        else if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                fprintf(stderr, "cd: missing argument\n");
            } else {
                if (chdir(args[1]) != 0) {
                    perror("cd failed");
                }
            }
        }

        else if (strcmp(args[0], "ls") == 0) {
            pid_t pid = fork();
            if (pid == 0) {
                execv("/bin/ls", args);
                perror("execv failed");
                exit(1);
            } else if (pid > 0) {
                wait(NULL);
            } else {
                perror("fork failed");
            }
        }
        else {
            execute_foreground_command(args);
        }
    }

    return 0;
}

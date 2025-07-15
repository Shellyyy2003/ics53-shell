#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_LINE 80
#define MAX_ARGC 80

void print_prompt() {
    printf("prompt > ");
    fflush(stdout);
}

int main() {
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

        // 空命令
        if (args[0] == NULL)
            continue;

        // quit 命令
        if (strcmp(args[0], "quit") == 0) {
            break;
        }
        // pwd 命令
        else if (strcmp(args[0], "pwd") == 0) {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("getcwd failed");
            }
        }
        // cd 命令
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
        // 未知命令
        else {
            printf("Unknown command: %s\n", args[0]);
        }
    }

    return 0;
}

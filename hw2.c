#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "h2.c"

void print_prompt() {
    printf("prompt > ");
    fflush(stdout);
}

void sig_handler(int signum) { // make sure each child is in seperate process group and has default signals
    for (int i = 0; i < jobInd; i++) {
        if (jobList[i].status && jobList[i].state) {
            kill(jobList[i].pid, signum);
            jobList[i].status = 0;
            print_prompt();
        }
    }
}

void sig_handler2(int signum) { // reaper
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        jobInd--;
    }
}

void sig_mod() { // Add at start
    signal(SIGTSTP, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGCHLD, sig_handler2);
}

void redir_I(char **args, char *line) {
    int inFileID = open (args[2], O_RDONLY, mode);
    if (inFileID < 0) {
            perror("input redirect failed");
            return;
        }
    int ori = dup(STDIN_FILENO);
    dup2(inFileID, STDIN_FILENO);

    if (fgets(line, MAX_LINE, stdin) == NULL) {
        perror("fgets failed");
        return;
    }

    dup2(ori, STDIN_FILENO);
    close(ori);
    close(inFileID);

    printf("%s", line);
    line[strcspn(line, "\n")] = 0;

    if (strlen(line) == 0)
        return;

    int argc = 1;
    char *token = strtok(line, " \t");
    while (token != NULL && argc < MAX_ARGC - 1) {
        args[argc++] = token;
        token = strtok(NULL, " \t");
    }
    args[argc] = NULL;

    return;
}

void background_check (char ** args) {
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i++], "&") == 0) {
            def_state = 0; // bg
            break;
        }
    } 
}

void redir_O (char **args, char *line) {
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], ">") == 0) {
            redirect_out = 1;
            break;
        }
        else if (strcmp(args[i], ">>") == 0) {
            redirect_out = 2;
            break;
        }
        i++;
    }
    if (redirect_out == 1) {
        int outFileID = open (args[++i], O_CREAT|O_WRONLY|O_TRUNC, mode);
        if (outFileID < 0) {
            perror("output redirect failed");
            return;
        }
        origin = dup(STDOUT_FILENO);
        dup2(outFileID, STDOUT_FILENO);
        close(outFileID);

    }
    else if (redirect_out == 2) {
        int outFileID = open(args[++i], O_CREAT|O_APPEND|O_WRONLY, mode);
        if (outFileID < 0) {
            perror("output append failed");
            return;
        }
        origin = dup(STDOUT_FILENO);
        dup2(outFileID, STDOUT_FILENO);
        close(outFileID);
    }
}

void redir_close() {
    dup2(origin, STDOUT_FILENO);
    close(origin);
}

int main() {
    char line[MAX_LINE];
    char *args[MAX_ARGC];

    sig_mod();
    //mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;

    while (1) {
        print_prompt();

        if (fgets(line, MAX_LINE, stdin) == NULL) {
            perror("fgets failed");
            continue;
        }

        copyLine(line);
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
            clear();
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
                signal(SIGCHLD, SIG_DFL);
                signal(SIGINT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                setpgid(0, 0);
                execv("/bin/ls", args);
                perror("execv failed");
                exit(1);
            } else if (pid > 0) {
                addJob(pid, def_state);
                waitpid(pid, NULL, WUNTRACED);
            } else {
                perror("fork failed");
            }
        }

        // Test background processes
        else if (strcmp(args[0], "test") == 0) {
            pid_t pid = fork();
            if (pid == 0) {
                signal(SIGCHLD, SIG_DFL);
                signal(SIGINT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                setpgid(0, 0);
                execv("./test", args);
                perror("execv failed");
                exit(1);
            } else if (pid > 0) {
                addJob(pid, def_state);
                //waitpid(pid, NULL, WUNTRACED);
            } else {
                perror("fork failed");
            }
        }

        //Jobs
        else if (strcmp(args[0], "jobs") == 0) {
           jobs();
        }

        else if (strcmp(args[0], "fg") == 0) {
           fg(args);
        }

        else if (strcmp(args[0], "bg") == 0) {
           bg(args);
        }
        
        else if (strcmp(args[0], "kill") == 0) {
            if (strcmp(args[1], "<") == 0) {
                redir_I(args, line);
            }
            for (int i = 0; i < 4; i++) {
                if (args[i] == NULL) printf("NULL");
                else printf("%s, ", args[i]);
            }
            killl(args);
            jobs();
            clear();
            exit(0);
        }

        // 未知命令
        else {
            printf("Unknown command: %s\n", args[0]);
        }

        def_state = 1;
        redirect_out = 0;
    }

    clear();
    return 0;
}

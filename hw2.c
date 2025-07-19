#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_JOBS 5
#define MAX_LINE 80
#define MAX_ARGC 80

struct Job {
    int jobid;
    pid_t pid;
    int state; // 0 = bg, 1 = fg
    int status; // 0 = stopped, 1 = running
    char cline[MAX_LINE];
};

int def_state = 1; // fg by default
int redirect_out = 0;
mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;
int origin;
struct Job jobList[MAX_JOBS];
int jobInd = 0;

void addJob(pid_t p, int state) { // Run after every job (fork)
    if (jobInd < MAX_JOBS) {
        jobList[jobInd].jobid = jobInd + 1;
        jobList[jobInd].pid = p;
        jobList[jobInd].state = state;
        jobList[jobInd].status = 1;

        char *s = (state == 0) ? "background" : "foreground";
        printf("job id: [%d] pid: (%d) job state: %s\n", jobList[jobInd].jobid, jobList[jobInd].pid, s);
        jobInd++;
    } else {
        perror("job list full");
    }
}

void copyLine(char *l) { //Run just after reading the line, will overwrite until an actual job is added - Alternatively create buffer in main and add this to addJob
    if (jobInd < MAX_JOBS) {
        strcpy(jobList[jobInd].cline, l);
    } else {
        perror("job list full");
    }
}

void jobs() {
    int sig;
    char status[MAX_LINE];

    for (int i = 0; i < jobInd; i++) {
        if (jobList[i].status == 0)
            strcpy(status, "Stopped");
        else
            strcpy(status, "Running");

        printf("[%d] (%d) %s %s", jobList[i].jobid, jobList[i].pid, status, jobList[i].cline);
    }
}

void clear() {
    for (int i = 0; i < jobInd; i++) {
        kill(jobList[i].pid, SIGINT);
        waitpid(jobList[i].pid, NULL, 0);
    }
}

void fg(char **args) {
    int index = -1;
    if (args[1] == NULL) {
        perror("fg failed");
    }
    else if (args[1][0] == '%') {
        int in = atoi(&args[1][1]) - 1;
        if (in < jobInd && in > -1)
            index = in;
    }
    else {
        for (int i = 0; i < jobInd; i++) {
            if (jobList[i].pid == atoi(args[1])) {
                index = i;
                break;
            }
        }
    }
    if (index == -1) {
        perror("fg failed");
        return;
    }
    jobList[index].state = 1; //fg
    jobList[index].status = 1; //running
    kill(jobList[index].pid, SIGCONT);
    waitpid(jobList[index].pid, NULL, WUNTRACED);
}

void bg(char **args) {
    int index;
    if (args[1] == NULL) {
        perror("bg failed");
    }
    else if (args[1][0] == '%') {
        int in = atoi(&args[1][1]) - 1;
            if (in < jobInd && in > -1)
                index = in;
    }
    else {
        for (int i = 0; i < jobInd; i++) {
            if (jobList[i].pid == atoi(args[1])) {
                index = i;
                break;
            }
        }
    }
    if (index == -1) {
        perror("bg failed");
        return;
    }
    jobList[index].state = 0; //fg
    jobList[index].status = 1; //running
    kill(jobList[index].pid, SIGCONT);
}

void killl(char **args) { //DOesnt remove correct job from list, chang ethat
    pid_t pid = -1;
    if (args[1] == NULL || args[2] == NULL) {
        perror("kill failed");
    }
    else if (args[2][0] == '%') {
        int in = atoi(&args[2][1]) - 1;
        if (in < jobInd && in > -1)
            pid = jobList[in].pid;
    }
    else {
        for (int i = 0; i < jobInd; i++) {
            if (jobList[i].pid == atoi(args[2])) {
                pid = jobList[i].pid;
                break;
            }
        }
    }
    if (pid == -1) {
        perror("kill failed");
        return;
    }
    kill(pid, atoi(args[1]));
}

void print_prompt() {
    printf("prompt > ");
    fflush(stdout);
}

void remove_job(pid_t pid) {
    for (int i = 0; i < jobInd; i++) {
        if (jobList[i].pid == pid) {
            if (i != jobInd - 1) {
                for (int j = i + 1; j < jobInd; ++j) {
                    jobList[j - 1].jobid = jobList[j].jobid;
                    jobList[j - 1].pid = jobList[j].pid;
                    jobList[j - 1].state = jobList[j].state;
                    jobList[j - 1].status = jobList[j].status;
                    strcpy(jobList[j - 1].cline, jobList[j].cline);
                }
            }
            jobInd--;
            return;
        }
    }
}

void sig_handler(int signum) { // make sure each child is in seperate process group and has default signals
    for (int i = 0; i < jobInd; i++) {
        if (jobList[i].status && jobList[i].state) {
            kill(jobList[i].pid, signum);
            jobList[i].status = 0;
        }
    }
}

void sig_handler3(int signum) { // Control C
    for (int i = 0; i < jobInd; i++) {
        if (jobList[i].status && jobList[i].state) {
            kill(jobList[i].pid, signum);
            remove_job(jobList[i].pid);
            return;
        }
    }
}

void sig_handler2(int signum) { // reaper
    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
        remove_job(pid);
}

void sig_mod() { // Add at start
    signal(SIGTSTP, sig_handler);
    signal(SIGINT, sig_handler3);
    signal(SIGCHLD, sig_handler2);
}

void sig_default() {
    signal(SIGCHLD, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    setpgid(0, 0);
}

void redir_I(char **args, char *line) {
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], "<") == 0)
            break;
        i++;
    }
    if (args[i] == NULL)
        return;
    int inFileID = open(args[++i], O_RDONLY, mode);
    if (inFileID < 0) {
            perror("input redirect failed");
            return;
        }
    int ori = dup(STDIN_FILENO);
    dup2(inFileID, STDIN_FILENO);

    if (read(inFileID, line, MAX_LINE) < 0) {
        perror("fgets failed");
        return;
    }

    dup2(ori, STDIN_FILENO);
    close(ori);
    close(inFileID);

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

void redir_O (char **args) { // Redirects Output (Uses redir_close() to close output files end of while loop)
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

void execute_foreground_command(char **args, char * line) {
    background_check(args);
    pid_t pid = fork();
    if (pid == 0) {
        redir_O(args);
        redir_I(args, line);
        sig_default();

        if (strchr(args[0], '/') == NULL) {
            char path_buf[256];
            snprintf(path_buf, sizeof(path_buf), "./%s", args[0]);
            execv(path_buf, args);
        }

        execvp(args[0], args);

        perror("execvp/execv failed");
        exit(1);
    } else if (pid > 0) {
        addJob(pid, def_state);
        if (def_state) {
            int status;
            waitpid(pid, &status, WUNTRACED);
            if (WIFSTOPPED(status))
                return;
            remove_job(pid);
        }
    } else {
        perror("fork failed");
    }
}

int main() {
    char line[MAX_LINE];
    char *args[MAX_ARGC];

    sig_mod();

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

        if (args[0] == NULL)
            continue;

        if (strcmp(args[0], "quit") == 0) {
            clear();
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
            background_check(args);
            pid_t pid = fork();
            if (pid == 0) {
                redir_I(args, line);
                redir_O(args);
                sig_default();

                execv("/bin/ls", args);
                perror("execv failed");
                exit(1);
            } else if (pid > 0) {
                addJob(pid, def_state);
                if (def_state) {
                    int status;
                    waitpid(pid, &status, WUNTRACED);
                    if (!WIFSTOPPED(status))
                        remove_job(pid);
                }
            } else {
                perror("fork failed");
            }
        }

        //4
        else if (strcmp(args[0], "jobs") == 0) {
            background_check(args);
            redir_O(args);
            redir_I(args, line);
            jobs();
        }

        else if (strcmp(args[0], "fg") == 0) {
            background_check(args);
            redir_I(args, line);
            fg(args);
        }

        else if (strcmp(args[0], "bg") == 0) {
            background_check(args);
            redir_I(args, line);
            bg(args);
        }
        
        else if (strcmp(args[0], "kill") == 0) {
            background_check(args);
            redir_O(args);
            redir_I(args, line);
            killl(args);
        }

        // 未知命令
        else {
            execute_foreground_command(args, line);
        }

        if (redirect_out) {
            redir_close();
            redirect_out = 0;
        }
        def_state = 1;
    }
    clear();
    return 0;
}

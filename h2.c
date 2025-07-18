#ifndef HEADER_H
#define HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_JOBS 5
#define MAX_LINE 80 // Moved here so I can use them too
#define MAX_ARGC 80
int def_state = 1; // fg by default
int redirect_out = 0;
mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;
int origin;

struct Job {
    int jobid;
    pid_t pid;
    int state; // 0 = bg, 1 = fg
    int status; // 0 = stopped, 1 = running
    char line[MAX_LINE];
};

struct Job jobList[MAX_JOBS];
int jobInd = 0;

void addJob(pid_t p, int state) { // Run after every job (fork)
    if (jobInd < MAX_JOBS) {
        jobList[jobInd].jobid = jobInd;
        jobList[jobInd].pid = p;
        jobList[jobInd].state = state;
        jobList[jobInd].status = 1;

        char *s = (state == 0) ? "background" : "foreground";
        printf("job id: [%d] pid: (%d) job state: %s\n", jobList[jobInd].jobid + 1, jobList[jobInd].pid, s);
        jobInd++;
    } else {
        perror("job list full");
    }
}

void copyLine(char *l) { //Run just after reading the line, will overwrite until an actual job is added - Alternatively create buffer in main and add this to addJob
    if (jobInd < MAX_JOBS) {
        strcpy(jobList[jobInd].line, l);
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

        printf("[%d] (%d) %s %s", jobList[i].jobid + 1, jobList[i].pid, status, jobList[i].line);
    }
}

void clear() {
    for (int i = 0; i < jobInd; i++) {
        kill(jobList[i].pid, SIGINT);
    }
}

void fg(char **args) {
    int index;
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
                if (jobList[i].pid == atoi(args[1])) {
                    index = i;
                    break;
                }
            }
        }
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
                if (jobList[i].pid == atoi(args[1])) {
                    index = i;
                    break;
                }
            }
        }
    }
    jobList[index].state = 0; //fg
    jobList[index].status = 1; //running
    kill(jobList[index].pid, SIGCONT);
}

void killl(char **args) { //DOesnt remove correct job from list, chang ethat
    pid_t pid;
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
    kill(pid, atoi(args[1]));
}

#endif
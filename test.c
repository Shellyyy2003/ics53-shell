#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int main(int argv, char **argc) {
    while (1) {
        printf("running\n");
        sleep(5);
    }
}
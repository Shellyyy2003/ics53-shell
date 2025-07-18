#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char * argv[])
{ int n = atoi(argv[1]);
    printf("%d \n", n+2); // print n+2
    while(1);
    return 0;
}
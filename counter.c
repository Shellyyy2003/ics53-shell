#include <stdio.h>
#include <unistd.h>

int main() {
    int i = 0;
    while (1) {
        printf("Counter: %d\n", i++);
        sleep(1);
    }
    return 0;
}

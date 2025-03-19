#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    int loop = 0;
    while(1) {
        printf("%d: loop: %d\n", getpid(), loop);
        loop++;
        sleep(3);
    }
}
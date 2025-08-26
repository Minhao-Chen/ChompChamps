#include <stdio.h>
#include <unistd.h>

int main() {
    for (int i = 0; i < 5; i++) {
        char move = i % 4; 
        write(1, &move, 1); 
        sleep(1);
    }
    return 0;
}

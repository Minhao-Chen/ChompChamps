#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define SHM_NAME "/demo_shm"

int main() {
    int fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    int *ptr = mmap(NULL, sizeof(int), PROT_READ, MAP_SHARED, fd, 0);

    printf("Reader: read %d\n", *ptr);

    munmap(ptr, sizeof(int));
    close(fd);
    shm_unlink(SHM_NAME);
    return 0;
}

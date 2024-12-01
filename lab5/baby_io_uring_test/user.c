#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define SHARED_MEM_SIZE 4096
#define DEVICE_NAME "/dev/shm_device"

const char id[] = "22307140095";

int main() {
    int fd = open(DEVICE_NAME, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    char *shared_mem =
        mmap(NULL, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("Failed to mmap");
        close(fd);
        return -1;
    }

    uint32_t *sq_head = (uint32_t *)shared_mem;
    uint32_t *sq_tail = (uint32_t *)(shared_mem + 4);
    char *sq_data = (char *)(shared_mem + 8);

    int len = strlen(id);
    for (int i = 0; i < len; i++) {
        // TODO BEGIN: Write the request (id[i] - '0') to sq_data (Submission Queue, SQ).
        *(sq_data+i*sizeof(char)) = id[i] - '0';   
        *sq_tail += 1;
        // TODO END: Write the request (id[i] - '0') to sq_data (Submission Queue, SQ).
    }

    munmap(shared_mem, SHARED_MEM_SIZE);
    close(fd);
    return 0;
}

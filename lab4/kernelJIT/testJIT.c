/*
 * testJIT.c
 * Copyright (C) 2024 cameudis <cameudis@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "JIT.h"

int my_user_function(void) {
  return 42;
}
void my_user_function_end() {}

int main() {
    int fd;
    struct user_function uf;
    int result;

    fd = open("/dev/JIT", O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    uf.addr = my_user_function;
    uf.length = (size_t)my_user_function_end - (size_t)my_user_function; /* another dark magic ... */

    /* TODO: Add ioctl to register the function */
    if (ioctl(fd, JIT_IOCTL_LOAD_FUNC, &uf) < 0) {
        perror("ioctl load function");
        close(fd);
        return -1;
    }

    /* TODO: Add ioctl to call the function */
    result = ioctl(fd, JIT_IOCTL_EXECUTE_FUNC, 0);

    if (result == 42) {
        printf("Result is 42, success!\n");
    } else {
        printf("Failed, result is %d\n", result);
    }
    

    close(fd);
    return 0;
}


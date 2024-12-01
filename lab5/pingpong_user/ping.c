/*
 * ping.c
 * Copyright (C) 2024 cameudis <cameudis@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define DEVICE_PATH "/dev/pingpong_dev"
#define PING_CMD _IOW('p', 1, int)

int main() {
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }
    while (1) {
        ioctl(fd, PING_CMD);
        printf("Ping!\n");
        sleep(1);
    }
    close(fd);
    return 0;
}

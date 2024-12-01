/*
 * pingpong.c
 * Copyright (C) 2024 cameudis <cameudis@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>

#define DEVICE_NAME "pingpong_dev"
#define PING_CMD _IOW('p', 1, int)
#define PONG_CMD _IOW('p', 2, int)

static int pingpong_flag = 0;
static wait_queue_head_t wait_queue;

/* ioctl handler function */
static long pingpong_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case PING_CMD:
            /* TODO */
            /* Ping命令处理 */
            if (pingpong_flag == 0) {
                /* 当前是Ping，设置标志并唤醒Pong */
                pingpong_flag = 1;
                /* 唤醒Pong命令等待的进程 */
                wake_up(&wait_queue);
            } else {
                /* 如果轮到Pong了，等待Pong的完成 */
                wait_event(wait_queue, pingpong_flag == 0);  // 等待Pong命令执行
            }
            break;
        
        case PONG_CMD:
            /* TODO */
            if (pingpong_flag == 1) {
                /* 当前是Pong，设置标志并唤醒Ping */
                pingpong_flag = 0;
                /* 唤醒Ping命令等待的进程 */
                wake_up(&wait_queue);
            } else {
                /* 如果轮到Ping了，等待Ping的完成 */
                wait_event(wait_queue, pingpong_flag == 1);  // 等待Ping命令执行
            }
            break;

        default:
            return -EINVAL;
    }
    return 0;
}

/* File operations structure for the device */
static const struct file_operations pingpong_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = pingpong_ioctl,
};

/* Define miscdevice structure */
static struct miscdevice pingpong_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &pingpong_fops,
};

/* Module initialization */
static int __init pingpong_init(void) {
    int ret;

    /* TODO: Initialize the wait_queue */
    init_waitqueue_head(&wait_queue);  // 初始化等待队列

    /* Register the misc device */
    ret = misc_register(&pingpong_device);
    if (ret) {
        printk(KERN_ERR "Failed to register misc device\n");
        return ret;
    }

    printk(KERN_INFO "PingPong device initialized with /dev/%s\n", DEVICE_NAME);
    return 0;
}

/* Module cleanup */
static void __exit pingpong_exit(void) {
    /* Unregister the misc device */
    misc_deregister(&pingpong_device);
    printk(KERN_INFO "PingPong device removed\n");
}

module_init(pingpong_init);
module_exit(pingpong_exit);
MODULE_LICENSE("GPL");
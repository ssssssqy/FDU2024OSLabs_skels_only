/*
 * kernelJIT.c
 * Copyright (C) 2024 cameudis <cameudis@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/kprobes.h>
#include "JIT.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Not Linus anyway");
MODULE_DESCRIPTION("A simple Linux kernel module to execute user space function in kernel space");

/* Dark magic to get addresses of essential functions */

unsigned long (*orig_kallsyms_lookup_name)(const char *name);
int (*set_memory_nx)(unsigned long addr, int numpages);
int (*set_memory_rw)(unsigned long addr, int numpages);
int (*set_memory_ro)(unsigned long addr, int numpages);
int (*set_memory_x)(unsigned long addr, int numpages);

struct kprobe kprobe_kallsyms_lookup_name = {.symbol_name = "kallsyms_lookup_name"};

static int fn_kallsyms_lookup_name_init(void) {
    register_kprobe(&kprobe_kallsyms_lookup_name);
    orig_kallsyms_lookup_name = (void *)kprobe_kallsyms_lookup_name.addr;
    unregister_kprobe(&kprobe_kallsyms_lookup_name);

    printk(KERN_INFO "kallsyms_lookup_name is %p\n", orig_kallsyms_lookup_name);

    set_memory_rw = (void *)orig_kallsyms_lookup_name("set_memory_rw");
    set_memory_ro = (void *)orig_kallsyms_lookup_name("set_memory_ro");
    set_memory_x = (void *)orig_kallsyms_lookup_name("set_memory_x");
    set_memory_nx = (void *)orig_kallsyms_lookup_name("set_memory_nx");

    return 0;
}

/* End of dark magic */

static int (*kernel_function)(void) = NULL;
static unsigned long exec_page = 0;

static long JIT_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case JIT_IOCTL_LOAD_FUNC: {

            /* TODO: set memory permissions (make it writable) */
            set_memory_rw(exec_page, 1);

            /* TODO: copy in the uf structure */
            struct user_function user_func;
            if (copy_from_user(&user_func, (struct user_function __user *)arg, sizeof(struct user_function)) != 0) {
                printk(KERN_ERR "JIT: Failed to copy user data\n");
                return -EFAULT;
            }

            /* TODO: copy the function code to exec_page */
            if (copy_from_user((void *)exec_page, user_func.addr, user_func.length) != 0) {
                printk(KERN_ERR "JIT: Failed to copy function code to kernel\n");
                return -EFAULT;
            }

            printk(KERN_INFO "JIT: Function loaded successfully\n");
            break;
        }
        case JIT_IOCTL_EXECUTE_FUNC: {

            /* TODO: set memory permissions (make it executable) */
            set_memory_x(exec_page, 1);

            /* call kernel_function */
            return kernel_function();
            break;
        }
        default:
            return -EINVAL;
    }

    return 0;
}

static struct file_operations fops = {
    .unlocked_ioctl = JIT_ioctl,
};
static int device_major = 0;

static int __init JIT_init(void) {
    fn_kallsyms_lookup_name_init();

    device_major = register_chrdev(0, "JIT", &fops);
    if (device_major < 0) {
        printk(KERN_ERR "JIT: failed to register char device\n");
        return device_major;
    }

    /* TODO: allocate a page with kmalloc */
    /* exec_page = ...; */
    exec_page = (unsigned long)kmalloc(PAGE_SIZE, GFP_KERNEL);

    if (!exec_page) {
        printk(KERN_ERR "JIT: Failed to allocate memory for exec_page\n");
        return -ENOMEM;
    } else {
        printk(KERN_INFO "JIT: allocated executable page at %lx\n", exec_page);
    }

    kernel_function = (int (*)(void))exec_page;

    printk(KERN_INFO "JIT: module loaded\n");
    return 0;
}

static void __exit JIT_exit(void) {
    unregister_chrdev(device_major, "JIT");

    /* TODO: set memory permissions (back to rw) */
    set_memory_rw(exec_page, 1);

    /* TODO: free the page */
    kfree((void *)exec_page);

    printk(KERN_INFO "JIT: module exited\n");
}

module_init(JIT_init);
module_exit(JIT_exit);
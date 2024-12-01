#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define SHARED_MEM_SIZE 4096
#define CLASS_NAME "shm"
#define DEVICE_NAME "shm_device"

#define TODO 0

static char *shared_memory = NULL;
volatile static int *ring_buffer_head = NULL;
volatile static int *ring_buffer_tail = NULL;
volatile static char *sq_data = NULL;

static int major_number;
static struct class *shm_class = NULL;
static struct device *shm_device = NULL;
static struct task_struct *sq_thread = NULL;
static int stop_thread = 0;

static char secret[] = {'H', 'e', 'l', 'l', 'o', 'F', 'u', 'd', 'a', 'n'};

static int device_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "shm_device: Device opened\n");
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "shm_device: Device closed\n");
    return 0;
}

static int device_mmap(struct file *file, struct vm_area_struct *vma) {
    unsigned long pfn = __pa(shared_memory) >> PAGE_SHIFT;
    unsigned long size = vma->vm_end - vma->vm_start;

    if (size > SHARED_MEM_SIZE) {
        return -EINVAL;
    }

    if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
        return -EAGAIN;
    }

    printk(KERN_INFO "shm_device: mmap successful\n");
    return 0;
}

static int worker(void *data) {
    while (!kthread_should_stop()) {
        // TODO BEGIN: Initiazlie head and tail.
        int head = *ring_buffer_head;
        int tail = *ring_buffer_tail;
        // TODO END: Initiazlie head and tail.

        if (head != tail) {
            // TODO BEGIN: Read from SQ and process the request (print secret[i] to dmesg).
            int index = *(sq_data + head * sizeof(char));
            printk(KERN_INFO "%c\n",secret[index]);
            // TODO END: Read from SQ and process the request (print secret[i] to dmesg).

            // TODO BEGIN: Update ring_buffer_head since we have processed the request.
            *ring_buffer_head = head + 1;
            // TODO END: Update ring_buffer_head since we have processed the request.
        } else {
            msleep(100);  // Sleep if no requests
        }
    }

    printk(KERN_INFO "shm_device: Thread stopping\n");
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .mmap = device_mmap,
};

// Module initialization
static int __init shm_device_init(void) {
    // TODO BEGIN: Allocate shared memory with kzalloc.
    shared_memory = kzalloc(SHARED_MEM_SIZE,GFP_KERNEL);
    // TODO END: Allocate shared memory with kzalloc.
    if (!shared_memory) {
        printk(KERN_ALERT "shm_device: Failed to allocate shared memory\n");
        return -ENOMEM;
    }

    // TODO BEGIN: Initialize ring_buffer_head, ring_buffer_tail and sq_data.
    ring_buffer_head = (volatile int*) shared_memory;
    ring_buffer_tail = (volatile int*) (shared_memory + sizeof(int));
    sq_data = (volatile char*) (shared_memory + 2 * sizeof(int));

    *ring_buffer_head = 0;
    *ring_buffer_tail = 0;
    // TODO END: Initialize ring_buffer_head, ring_buffer_tail and sq_data.

    // Register device
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "shm_device: Failed to register device\n");
        kfree(shared_memory);
        return major_number;
    }

    printk(KERN_INFO "shm_device: Device registered with major number %d\n",
           major_number);

    shm_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(shm_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(shm_class);
    }

    shm_device = device_create(shm_class, NULL, MKDEV(major_number, 0), NULL,
                               DEVICE_NAME);
    if (IS_ERR(shm_device)) {
        class_destroy(shm_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(shm_device);
    }

    // Start kernel thread
    sq_thread = kthread_run(worker, NULL, "shm_sq_thread");
    if (IS_ERR(sq_thread)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        kfree(shared_memory);
        printk(KERN_ALERT "shm_device: Failed to create thread\n");
        return PTR_ERR(sq_thread);
    }

    return 0;
}

// Module cleanup
static void __exit shm_device_exit(void) {
    stop_thread = 1;
    if (sq_thread) {
        kthread_stop(sq_thread);
    }

    device_destroy(shm_class, MKDEV(major_number, 0));
    class_unregister(shm_class);
    class_destroy(shm_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    kfree(shared_memory);
    printk(KERN_INFO "shm_device: Module unloaded\n");
}

module_init(shm_device_init);
module_exit(shm_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yi Sun");
MODULE_DESCRIPTION("Baby io_uring.");
MODULE_VERSION("1.0");

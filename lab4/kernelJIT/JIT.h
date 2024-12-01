/*
 * JIT.h
 * Copyright (C) 2024 cameudis <cameudis@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef JIT_H
#define JIT_H

#include <linux/ioctl.h>

#define JIT_IOCTL_MAGIC 'k'

struct user_function {
    void *addr;
    size_t length;
};

// struct user_data {
//     void *func_addr;
//     size_t func_len;
// };

/* ioctl operations */
/* Usage: ioctl(fd, JIT_IOCTL_LOAD_FUNC, &user_function) */
/*        ioctl(fd, JIT_IOCTL_EXECUTE_FUNC) */
#define JIT_IOCTL_LOAD_FUNC _IOW(JIT_IOCTL_MAGIC, 1, struct user_function)
#define JIT_IOCTL_EXECUTE_FUNC _IO(JIT_IOCTL_MAGIC, 2)

#endif /* !JIT_H */

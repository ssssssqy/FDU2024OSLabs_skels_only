#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
/* TODO: add missing headers */
#include <linux/sched.h>
#include <linux/sched/signal.h>

MODULE_DESCRIPTION("List current processes");
MODULE_AUTHOR("Kernel Hacker");
MODULE_LICENSE("GPL");

static int my_proc_init(void)
{
	struct task_struct *p;

	/* TODO: print current process pid and its name */
	pid_t pid;
	char *comm;
	p = current;
	pid = p->pid;
	comm = p->comm;
	printk(KERN_INFO "module loaded, current pid %d, executable file %s\n",pid,comm);
	/* TODO: print the pid and name of all processes */
	for_each_process(p){
		printk(KERN_INFO "pid:%d, executable file:%s\n",p->pid,p->comm);
	}
	return 0;
}

static void my_proc_exit(void)
{
	/* TODO: print current process pid and name */
	struct task_struct *p;
	pid_t pid;
	char *comm;
        p = current;
        pid = p->pid;
        comm = p->comm;
        printk(KERN_INFO "module unloaded, current pid %d, executable file %s\n",pid,comm);
}

module_init(my_proc_init);
module_exit(my_proc_exit);

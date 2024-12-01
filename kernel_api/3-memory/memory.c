/*
 * SO2 lab3 - task 3
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

MODULE_DESCRIPTION("Memory processing");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

struct task_info {
	pid_t pid;
	unsigned long timestamp;
};

static struct task_info *ti1, *ti2, *ti3, *ti4;

static struct task_info *task_info_alloc(int pid)
{
	struct task_info *ti;

	/* TODO 1: allocated and initialize a task_info struct */
	ti = kmalloc(sizeof *ti,GFP_KERNEL);
	
	if (!ti)
		return NULL;
	
	ti->pid = pid;
	ti->timestamp = jiffies;

	return ti;
}

static int memory_init(void)
{
	/* TODO 2: call task_info_alloc for current pid */
	pid_t current_pid, parent_pid, next_pid, next_next_pid;
	current_pid = current->pid;
	ti1 = task_info_alloc(current_pid);

	/* TODO 2: call task_info_alloc for parent PID */
	parent_pid = current->parent->pid;
        ti2 = task_info_alloc(parent_pid);

	/* TODO 2: call task_info alloc for next process PID */
	next_pid = next_task(current)->pid;
	ti3 = task_info_alloc(next_pid);	

	/* TODO 2: call task_info_alloc for next process of the next process */
	next_next_pid = next_task(next_task(current))->pid;
        ti4 = task_info_alloc(next_next_pid);

	return 0;
}

static void memory_exit(void)
{

	/* TODO 3: print ti* field values 
	 * */
	printk(KERN_INFO "current pid: %d, timestamp:%lu \n",ti1->pid,ti1->timestamp);
	printk(KERN_INFO "parent pid: %d, timestamp:%lu \n",ti2->pid,ti2->timestamp);
	printk(KERN_INFO "next pid: %d, timestamp:%lu \n",ti3->pid,ti3->timestamp);
	printk(KERN_INFO "next next pid: %d, timestamp:%lu \n",ti4->pid,ti4->timestamp);
	/* TODO 4: free ti* structures */
	kfree(ti1);
	kfree(ti2);
	kfree(ti3);
	kfree(ti4);
}

module_init(memory_init);
module_exit(memory_exit);

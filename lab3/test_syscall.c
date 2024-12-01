#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>
#define __NR_my_syscall 222
int main()
{
	long ret = syscall(__NR_my_syscall,__NR_my_syscall);
	printf("Return value: %ld\n", ret);
	return 0;
}

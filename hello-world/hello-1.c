// Abe Jordan
// hello-1.c

#include <linux/module.h>
#include <linux/kernel.h>

int init_module(void)
{
	pr_info("Hello world 1.\n");

	return 0;
}

void cleanup_module(void)
{
	pr_info("Goodbye world1.\n");
}

MODULE_LICENSE("GPL");


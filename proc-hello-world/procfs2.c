/******************************************************************************
 * Abe Jordan								      *
 * proc-hello-world							      *
 * procfs2.c								      *
 ******************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define HAVE_PROC_OPS
#endif

#define PROCFS_MAX_SIZE 1024
#define PROCFS_NAME "buffer1k"

// holds information about /proc file
static struct proc_dir_entry *our_proc_file;

// buffer used to store character for module
static char procfs_buffer[PROCFS_MAX_SIZE];

// size of buffer
static unsigned long procfs_buffer_size = 0;

// called when /proc file is read
static ssize_t procfile_read(struct file *filePointer, char __user *buffer,
		size_t buffer_length, loff_t *offset)
{
	char s[13] = "HelloWorld!\n";
	int len = sizeof(s);
	ssize_t ret = len;

	if (*offset >= len || copy_to_user(buffer, s, len)) {
		pr_info("copy_to_user failed\n");
		ret = 0;
	} else {
		pr_info("procfile read %s\n",
				filePointer->f_path.dentry->d_name.name);
		*offset += len;
	}

	return ret;
}

/* called when /proc file written */
static ssize_t procfile_write(struct file *file, const char __user *buff,
		size_t len, loff_t *off)
{
	procfs_buffer_size = len;
	if (procfs_buffer_size > PROCFS_MAX_SIZE)
		procfs_buffer_size = PROCFS_MAX_SIZE;

	if (copy_from_user(procfs_buffer, buff, procfs_buffer_size))
		return -EFAULT;

	procfs_buffer[procfs_buffer_size & (PROCFS_MAX_SIZE - 1)] = '\0';
	pr_info("procfile write %s\n", procfs_buffer);

	return procfs_buffer_size;
}

#ifdef HAVE_PROC_OPS
static const struct proc_ops proc_file_fops = {
	.proc_read = procfile_read,
	.proc_write = procfile_write,
};
#else
static const struct file_operations proc_file_fops = {
	.read = procfile_read,
	.write = procfile_write,
};
#endif

static int __init procfs2_init(void)
{
	our_proc_file = proc_create(procfs_name, 0644, NULL, &proc_file_fops);
	if (our_proc_file == NULL) {
		proc_remove(our_proc_file);
		pr_alert("Error: Could not initialize /proc/%s\n", PROCFS_NAME);
		return -ENOMEM;
	}

	pr_info("/proc%s created\n", PROCFS_NAME);
	return 0;
}

static void __exit procfs2_exit(void)
{
	proc_remove(our_proc_file);
	pr_info("/proc%s removed\n", PROCFS_NAME);
}

module_init(procfs2_init);
module_exit(procfs2_exit);

MODULE_LICENSE("GPL");

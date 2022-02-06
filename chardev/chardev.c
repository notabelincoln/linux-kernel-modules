/******************************************************************************
 * Abe Jordan                                                                 *
 * chardev                                                                    *
 * chardev.c                                                                  *
 ******************************************************************************/
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/poll.h>

/* protoypes would normally go in .h file */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *,	size_t,
		loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "chardev" // dev name as it appears in /proc/devices
#define BUF_LEN 80 // max length of message from device

/* global variables declared static so are global within file */
static int major; // major number assigned to device driver

enum {
	CDEV_NOT_USED = 0,
	CDEV_EXCLUSIVE_OPEN = 1,
};

/* check if device opena nd prevent multiple access to device */
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

static char msg[BUF_LEN]; // msg device gives when asked

static struct class *cls;

static struct file_operations chardev_fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release,
};

static int __init chardev_init(void)
{
	major = register_chrdev(0, DEVICE_NAME, &chardev_fops);

	if (major < 0) {
		pr_alert("Registering char device failed with %d\n", major);
		return major;
	}

	pr_info("I was assigned major number %d.\n", major);

	cls = class_create(THIS_MODULE, DEVICE_NAME);
	device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);

	return SUCCESS;
}

static void __exit chardev_exit(void)
{
	device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);

	unregister_chrdev(major, DEVICE_NAME); // unregister device
}

/* Methods */

/*************************************************
 * called when process tries to open device file *
 * e.g. sudo cat /dev/chardev                    *
 *************************************************/
static int device_open(struct inode *inode, struct file *file)
{
	static int counter = 0;

	if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN))
		return -EBUSY;

	sprintf(msg, "I already told you %d times Hello world!\n", counter++);
	try_module_get(THIS_MODULE);

	return SUCCESS;
}


/******************************************
 * called when process closes device file *
 ******************************************/
static int device_release(struct inode *inode, struct file *file)
{
	// now ready for next caller
	atomic_set(&already_open, CDEV_NOT_USED);

	// decrement usage count or module cannot be gotten rid of once file is opened
	module_put(THIS_MODULE);

	return SUCCESS;
}

/**********************************************************************
 * called when process which opened dev file attempts to read from it *
 **********************************************************************/
static ssize_t device_read(struct file *filp, char __user *buffer,
		size_t length, loff_t *offset)
{
	int bytes_read = 0; // number of bites written to buffer
	const char *msg_ptr = msg;

	// check if at end of message
	if (!*(msg_ptr + *offset)) {
		*offset = 0;
		return 0;
	}

	msg_ptr += *offset;

	// put data into buffer
	while (length && *msg_ptr) {
		/* buffer is in user data segment, not kernel segment, so '*'
		 * assignment won't work, have to use put_user which copies
		 * data from kernel data segment to user data segment */
		put_user(*(msg_ptr++), buffer++);
		length--;
		bytes_read++;
	}

	*offset += bytes_read;

	// return number of bytes put into buffer due to convention
	return bytes_read;
}	

/******************************************
 * called when process writes to dev file *
 * e.g. echo "hi" > /dev/hello            *
 ******************************************/
static ssize_t device_write(struct file *filp, const char __user *buff,
		size_t len, loff_t *off)
{
	pr_alert("Sorry, this operation is not supported.\n");
	return -EINVAL;
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");



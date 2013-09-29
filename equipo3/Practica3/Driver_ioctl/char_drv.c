#include <linux/module.h>	/* Dynamic loading of modules into the kernel */
#include <linux/fs.h>		/* file defs of important data structures */
#include <linux/cdev.h>		/* char devices structure and aux functions */
#include <linux/device.h>	/* generic, centralized driver model */
#include <linux/uaccess.h>	/* for accessing user-space */
#include <linux/kernel.h>
#include <linux/ioctl.h>
#include "operations.h"

#define KBUFF_MAX_SIZE	1024	/* size of the kernel side buffer */
#define DEVICE_NAME	"mydev"

static unsigned major;			/* major number */
static struct cdev mydev;			/* to register with kernel */
static struct class *mydev_class;	/* to register with sysfs */
static struct device *mydev_device;		/* to register with sysfs */

static char kbuffer[KBUFF_MAX_SIZE];		/* kernel side buffer */
static unsigned long kbuffer_size;		/* kernel buffer length */
static int number1;
static int number2;
static char operator;

#define MYCALC_IOC_MAGIC 'C'
#define MYCALC_IOC_SET_NUM1		_IOW(MYCALC_IOC_MAGIC, 0, int)
#define MYCALC_IOC_SET_NUM2		_IOW(MYCALC_IOC_MAGIC, 1, int)
#define MYCALC_IOC_SET_OPERATION	_IOW(MYCALC_IOC_MAGIC, 2, int)
#define MYCALC_IOC_GET_RESULT		_IOR(MYCALC_IOC_MAGIC, 3, int)
#define MYCALC_IOC_DO_OPERATION		_IOWR(MYCALC_IOC_MAGIC, 4, int)

struct struc_func {
	int number1;
	int number2;
	char operator;
	int result;
} *my_func;

/*
 * mydev_read()
 * This function implements the read function in the file operation structure.
 * This is executed on the cat function.
 */
static ssize_t mydev_read(struct file *filp, char __user *buffer, size_t nbuf,
								loff_t *offset)
{
	int ret = 0;
	pr_info("[%d] %s\n", __LINE__, __func__);
	pr_info("[%d] nbuf = %d, offset = %d", __LINE__, (int)nbuf, (int)*offset);
	/* If offset is greater than size of kbuffer, there's nothing to copy */
	if (*offset >= kbuffer_size) {
		pr_info("[%d] Offset greater or equal than kbuffer size: %d >= %ld", __LINE__, (int)*offset, kbuffer_size);
		goto out;
	}

	/* Check for maximum size of kernel buffer */
	if ((nbuf + *offset) > kbuffer_size)
		nbuf = kbuffer_size - *offset;

	/* fill the buffer, return the buffer size */
	pr_info("[%d] Copying Buffer from kernel to user space...\n", __LINE__);
	ret = copy_to_user(buffer, &kbuffer[*offset], nbuf);
	if (ret) {
		pr_info("copy_to_user failed: 0x%x", ret);
		ret = -EFAULT;
		goto out;
	}

	*offset += nbuf;
	ret = nbuf;

out:
	pr_info("[%d] nbuf = %d, offset = %d", __LINE__, (int)nbuf, (int)*offset);
	return ret;
}

/*
 * mydev_write()
 * This function implements the write function in the file operation structure.
 * This is executed on the echo function.
 */
static ssize_t mydev_write(struct file *filp, const char __user *buffer,
						size_t nbuf, loff_t *offset)
{
	int ret = 0;

	pr_info("[%d] nbuf = %d, offset = %d", __LINE__, (int)nbuf, (int)*offset);

	if (*offset >= KBUFF_MAX_SIZE) {
		pr_info("[%d] Offset greater or equal than kbuffer size: %d >= %ld", __LINE__, (int)*offset, kbuffer_size);
		goto out;
	}

	/* Check for maximum size of kernel buffer */
	if ((nbuf + *offset) > KBUFF_MAX_SIZE)
		nbuf = KBUFF_MAX_SIZE - *offset;

	/* fill the buffer, return the buffer size */
	pr_info("[%d] Copying Buffer from user to kernel space...\n", __LINE__);
	ret = copy_from_user(&kbuffer[*offset], buffer, nbuf);
	if (ret) {
		pr_info("copy_from_user failed: 0x%x", ret);
		ret = -EFAULT;
		goto out;
	}

	*offset += nbuf;
	kbuffer_size = *offset;
	ret = nbuf;

out:
	pr_info("[%d] nbuf = %d, offset = %d", __LINE__, (int)nbuf, (int)*offset);
	return ret;
}

/*
 * Function called when the user side application opens a handle to mydev driver
 * by calling the open() function.
 */

static int mydev_open(struct inode *ip, struct file *filp)
{

	pr_info("Driver's open function was called\n");

	return 0;
}

/*
 * Function called when user side application closes a handle to mydev driver
 * by calling the close() function.
 */

static int mydev_close(struct inode *ip, struct file *filp)
{

	pr_info("Driver's close function was called\n");

	return 0;
}

long device_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	int result = 0;

	switch (cmd) {
	case MYCALC_IOC_SET_NUM1:
		get_user(number1, (int *)arg);
		printk(KERN_INFO "About to set the number 1 %i\n", number1);
		break;
	case MYCALC_IOC_SET_NUM2:
		get_user(number2, (int *)arg);
		printk(KERN_INFO "About to set the number 2 %i\n", number2);
		break;
	case MYCALC_IOC_SET_OPERATION:
		get_user(operator, (char *)arg);
		printk(KERN_INFO "About to set the operation %c\n", operator);
		break;
	case MYCALC_IOC_GET_RESULT:
		/* Handle a division by 0 error */
		if (operator == '/' && number2 == 0)
			goto out;

			/* Calculate result */
			result = oper(number1, number2, operator);
			printk(KERN_INFO "Result: %i\n", result);
			put_user(result, (int *)arg);
			break;
	case MYCALC_IOC_DO_OPERATION:
		my_func = arg;
		copy_from_user(&number1, &my_func->number1, 4);
		copy_from_user(&number2, &my_func->number2, 4);
		copy_from_user(&operator, &my_func->operator, 1);

		/* Handle a division by 0 error */
		if (operator == '/' && number2 == 0)
			goto out;

			/* Calculate result */
		result = oper(number1, number2, operator);
		copy_to_user(&my_func->result, &result, 4);
		break;
	default:
		return -ENOTTY;
	}

out:
	return result;
}

/*
 * File operation structure
 */
static const struct file_operations mydev_fops = {
	.open = mydev_open,
	.release = mydev_close,
	.owner = THIS_MODULE,
	.read = mydev_read,
	.write = mydev_write,
	.unlocked_ioctl = device_ioctl,
};

/*
 * init my_calc_init()
 * Module init function. It is called when insmod is executed.
 */
static int __init my_calc_init(void)
{
	int ret;
	dev_t devid;

	/* Dynamic allocation of MAJOR and MINOR numbers */
	ret = alloc_chrdev_region(&devid, 0, 1, DEVICE_NAME);

	if (ret) {
		printk(KERN_ERR "Error: Failed registering major number\n");
		return ret;
	}

	/* save MAJOR number */
	major = MAJOR(devid);

	/* Initalize the cdev structure */
	cdev_init(&mydev, &mydev_fops);

	/* Register the char device with the kernel */
	ret = cdev_add(&mydev, devid, 1);
	if (ret) {
		printk(KERN_ERR "Error: Failed registering with the kernel\n");
		unregister_chrdev_region(devid, 1);
		return ret;
	}

	/* Create a class and register with the sysfs. (Failure is not fatal) */
	mydev_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(mydev_class)) {
		printk(KERN_ERR "class_create() failed: %ld\n", PTR_ERR(mydev_class));
		mydev_class = NULL;
	} else {
		/* Register device with sysfs (creates device node in /dev) */
		mydev_device = device_create(mydev_class, NULL, devid, NULL,\
		 DEVICE_NAME"0");
		if (IS_ERR(mydev_device)) {
			printk(KERN_ERR "device_create() failed: %ld\n", PTR_ERR(mydev_device));
			mydev_device = NULL;
		}
	}

	printk(KERN_INFO "Driver \"%s\" loaded...\n", DEVICE_NAME);
	kbuffer_size = 0;
	return 0;
}

/*
 * init my_calc_init()
 * Module exit function. It is called when rmmod is executed.
 */
static void __exit my_calc_exit(void)
{
	/* Deregister char device from kernel */
	cdev_del(&mydev);

	/* Release MAJOR and MINOR numbers */
	unregister_chrdev_region(MKDEV(major, 0), 1);

	/* Deregister device from sysfs */
	if (mydev_class != NULL) {
		device_destroy(mydev_class, MKDEV(major, 0));
		class_destroy(mydev_class);
	}

	printk(KERN_INFO "Driver \"%s\" unloaded...\n", DEVICE_NAME);
}

module_init(my_calc_init);
module_exit(my_calc_exit);

MODULE_AUTHOR("Ivan Gomez Castellanos");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Character Driver.");

/* Char device calculator */

#include <linux/module.h>	/* Dynamic loading of modules into the kernel */
#include <linux/fs.h>		/* file defs of important data structures */
#include <linux/cdev.h>		/* char devices structure and aux functions */
#include <linux/device.h>	/* generic, centralized driver model */
#include <linux/uaccess.h>	/* for accessing user-space */
#include "operations.h"
#define KBUFF_MAX_SIZE	256	/* size of the kernel side buffer */
#define DEVICE_NAME	"mycalc0"


static unsigned major;				/* major number */
static struct cdev mydev;			/* to register with kernel */
static struct class *mydev_class;		/* to register with sysfs */
static struct device *mydev_device;		/* to register with sysfs */

static char kbuffer[KBUFF_MAX_SIZE];		/* kernel side buffer */
static unsigned long kbuffer_size;		/* kernel buffer length */

/* Our program variables */
int num1, num2, operation, result;		/* global variables */
static int i;
static int sign;
static char stringBuffer[KBUFF_MAX_SIZE];

#define SPACE 0x20		/* Space in hexadecimal */
#define NEGATIVE 0x2D		/* Negativo/Minus in hexadecimal */
#define FIRST_NUM 0x30		/* First number, this is a 0 */
#define LAST_NUM 0x39		/* Last number, this is a 9 */
#define FIRST_OPERATOR 0x2A	/* First operator, this is a * */
#define LAST_OPERATOR 0x2F	/* Last operator, this is a / */

/* mydev_read() */
/* Implements the read function in the file operation structure. */
static ssize_t mycalc_read(struct file *filp, char __user *buffer, size_t nbuf,
								loff_t *offset)
{
	int ret = 0;
	pr_info("[%d] %s\n", __LINE__, __func__);
	pr_info("[%d] nbuf = %d, offset = %d", __LINE__, (int)nbuf, (int)*offset);
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

/* Check if the first number is negative */
void checkSign(void)
{
	if (kbuffer[i] == NEGATIVE) {
		sign = 1;
		i++;
	}
}

/* Erase blank spaces from the beginning (ie: "     100" should be  "100") */
void eraseSpaces(void)
{
	while (kbuffer[i] == SPACE)
		i++;
}

/* If the sign equals 1 then negate the number */
int handleMinusSign(int number)
{
	if (sign == 1) {
		sign = 0;
		return number *= -1;
	} else
		return number;
}

/* Convert the string into a number (numbers are from 0x30 to 0x39) */
int getNumber(void)
{
	int result = 0;

	while (kbuffer[i] >= FIRST_NUM && kbuffer[i] <= LAST_NUM) {
		result *= 10;
		result += (int) (kbuffer[i] - '0');
		i++;
	}
	return result;
}

/*
 * mydev_write()
 * This function implements the write function in the file operation structure.
 */
static ssize_t mycalc_write(struct file *filp, const char __user *buffer,
						size_t nbuf, loff_t *offset)
{
	int ret = 0;
	pr_info("[%d] nbuf = %d, offset = %d", __LINE__, (int)nbuf, (int)*offset);

	i = 0;
	sign = 0;

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

	/* Calculator functions begin, sample input is "    100   + 200"*/

	eraseSpaces(); /* Will transform "     100" into "100" */
	checkSign(); /* Test for "-100" */
	num1 = getNumber(); /* Converts string into a number */
	num1 = handleMinusSign(num1);
	eraseSpaces(); /* Will transform "      + 200" into "+  200" */

	/* Verify that the operator is +, -, * or / */
	if (kbuffer[i] >= FIRST_OPERATOR && kbuffer[i] <= LAST_OPERATOR) {
		operation = kbuffer[i++];
	} else {
		ret = -EINVAL;
		goto out;
	}

	eraseSpaces(); /* Will transform "  200" into "200" */
	checkSign(); /* Test for "-200" */
	num2 = getNumber(); /* Converts string into a number */
	num2 = handleMinusSign(num2);

	/* Handle a division by 0 error */
	if (operation == '/' && num2 == 0) {
		ret = -EINVAL;
		goto out;
	}

	/* Handle dot and comma signs that are on the operators range */
	if (operation == '.' || operation == ',') {
		ret = -EINVAL;
		goto out;
	}

	result = oper(num1, num2, operation); /* Call operations.c */

	/* Convert the int into a string */
	sprintf(stringBuffer, "%d", result);
	sprintf(kbuffer, "%d", result);
	i = strlen(stringBuffer);
	kbuffer[i++] = '\n';

	/* Calculator functions ends */

	*offset += i;
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

static int mycalc_open(struct inode *ip, struct file *filp)
{

	pr_info("Driver's open function was called\n");

	return 0;
}

/*
 * Function called when the user side application closes a handle
 * to mydev driver by calling the close() function.
 */

static int mycalc_close(struct inode *ip, struct file *filp)
{

	pr_info("Driver's close function was called\n");

	return 0;
}


/*
 * File operation structure
 */
static const struct file_operations mydev_fops = {
	.open = mycalc_open,
	.release = mycalc_close,
	.owner = THIS_MODULE,
	.read = mycalc_read,
	.write = mycalc_write
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

MODULE_AUTHOR("Alejandro del Rio, Jorge Paz");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Character Driver.");

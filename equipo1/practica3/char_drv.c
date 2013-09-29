#include <linux/module.h>	/* Dynamic loading of modules into the kernel */
#include <linux/fs.h>		/* file defs of important data structures */
#include <linux/cdev.h>		/* char devices structure and aux functions */
#include <linux/device.h>	/* generic, centralized driver model */
#include <linux/uaccess.h>	/* for accessing user-space */
#include "operations.h"
#define KBUFF_MAX_SIZE	256	/* size of the kernel side buffer */
#define DEVICE_NAME "mycalc"

#define MYCALC_IOC_MAGIC 'C'
#define MYCALC_IOC_SET_NUM1		_IOW(MYCALC_IOC_MAGIC, 0, int)
#define MYCALC_IOC_SET_NUM2		_IOW(MYCALC_IOC_MAGIC, 1, int)
#define MYCALC_IOC_SET_OPERATION	_IOW(MYCALC_IOC_MAGIC, 2, int)
#define MYCALC_IOC_GET_RESULT		_IOR(MYCALC_IOC_MAGIC, 3, int)
#define MYCALC_IOC_DO_OPERATION		_IOWR(MYCALC_IOC_MAGIC, 4, int)

struct oper {
	int num1;
	int num2;
	char operador;
	int result;

} *aptr;

static unsigned major = 12;			/* major number */
static struct cdev mydev;			/* to register with kernel */
static struct class *mydev_class = NULL;	/* to register with sysfs */
static struct device *mydev_device;		/* to register with sysfs */

static char kbuffer[KBUFF_MAX_SIZE];		/* kernel side buffer */
static unsigned long kbuffer_size = 1;		/* kernel buffer length */
int num1, num2, operation, result;		/* global variables */
/*
 * mydev_read()
 * This function implements the read function in the file operation structure.
 */
static ssize_t mycalc_read(struct file *filp, char __user *buffer, size_t nbuf,
								loff_t *offset)
{
	int ret = 0;
	pr_info("[%d] %s\n", __LINE__, __func__);
	pr_info("[%d] nbuf = %d, offset = %d", __LINE__, (int)nbuf, (int)*offset);
	/* If offset is greater that size of kbuffer, there is nothing to copy */
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
 */
static ssize_t mycalc_write(struct file *filp, const char __user *buffer,
						size_t nbuf, loff_t *offset)
{
	int ret = 0, sign = 0;
	int i = 0;
	int tmp;
	int j = 0;
	int k = 0;
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
	/* Quitando los espacios del principio */
	while (kbuffer[i] == 0x20)
		i++;
	/* pasando de string a numeros el primer numero */
	num1 = 0;
	if (kbuffer[i] == 0x2D) {
		sign = 1;
		i++;
	}
	while (kbuffer[i] >= 0x30 && kbuffer[i] <= 0x39) {
		num1 *= 10;
		num1 += (int) (kbuffer[i]-'0');
		i++;
	}
	if (sign == 1)
		num1 *= -1;
	sign = 0;
	/* Quitando los espacios entre el primer numero y la operacion */
	while (kbuffer[i] == 0x20)
		i++;
	/* Verificando que este en el rango de las operaciones */
	if (kbuffer[i] > 0x29 && kbuffer[i] < 0x30) {
		operation = kbuffer[i++];
	} else {
		ret = -EINVAL;
		goto out;
	}
	/* Quitando los espacios entre la operacion y el segundo numero */
	while (kbuffer[i] == 0x20)
		i++;
	num2 = 0;
	/* pasando de string a numero el segundo numero */
	if (kbuffer[i] == 0x2D) {
		sign = 1;
		i++;
	}
	while (kbuffer[i] >= 0x30 && kbuffer[i] <= 0x39) {
		num2 *= 10;
		num2 += (int) (kbuffer[i]-'0');
		i++;
	}

	if (sign == 1)
		num2 *= -1;
	sign = 0;

	switch (operation) {
	case '+':
	result = suma(num1, num2);
	break;
	case '*':
	result = multiplica(num1, num2);
	break;
	case '/':
		if (num2 == 0) {
			ret = -EINVAL;
			goto out;
		}
	result = divide(num1, num2);
	break;
	case '-':
	result = resta(num1, num2);
	break;
	default:
	ret = -EINVAL;
	goto out;
	}

/*Metodo de voltear*/

	j = result;
	k = 0;

	while (j != 0) {
		j /= 10;
		k++;
	}

	i = k;
	if (result < 0) {
		kbuffer[0] = '-';
		result = -result;
		i++;
	} else {
	k--;
	}
/*Termina metodo de voltear*/

	while (result != 0) {
		tmp = result % 10;
		result /= 10;
		kbuffer[k--] = tmp + '0';
	}

	kbuffer[i++] = '\n';

	*offset += i;
	kbuffer_size = *offset;
	ret = nbuf;

out:
	pr_info("[%d] nbuf = %d, offset = %d", __LINE__, (int)nbuf, (int)*offset);
	return ret;
}

long device_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	int len = 200;
	int ret;
	switch (cmd) {
	case MYCALC_IOC_SET_NUM1:
		ret = copy_from_user(&num1, (char *)arg, 4);
		if (ret) {
			pr_info("copy from user failed 0x%x", ret);
			return -EFAULT;
		}
		break;
	case MYCALC_IOC_SET_NUM2:
		ret = copy_from_user(&num2, (char *)arg, 4);
		if (ret) {
			pr_info("copy from user failed 0x%x", ret);
			return -EFAULT;
		}
		break;
	case MYCALC_IOC_SET_OPERATION:
		ret = copy_from_user(&operation, (char *)arg, 2);
		if (ret) {
			pr_info("copy from user failed 0x%x", ret);
			return -EFAULT;
		}
		break;
	case MYCALC_IOC_GET_RESULT:
		switch ((char)operation) {
		case '+':
		result = suma(num1, num2);
		break;
		case '*':
		result = multiplica(num1, num2);
		break;
		case '/':
			if (num2 == 0)
				return -EINVAL;
		result = divide(num1, num2);
		break;
		case '-':
		result = resta(num1, num2);
		}
			ret = copy_to_user((char *)arg, &result, 4);
			if (ret) {
				pr_info("copy to user failed 0x%x", ret);
				return -EFAULT;
			}
		break;
	case MYCALC_IOC_DO_OPERATION:
		aptr = (struct oper *)arg;
		copy_from_user(&num1, &aptr->num1, 4);
		copy_from_user(&num2, &aptr->num2, 4);
		copy_from_user(&operation, &aptr->operador, 2);
		switch ((char)operation) {
		case '+':
		result = suma(num1, num2);
		break;
		case '*':
		result = multiplica(num1, num2);
		break;
		case '/':
			if (num2 == 0)
				return -EINVAL;
		result = divide(num1, num2);
		break;
		case '-':
		result = resta(num1, num2);
		}
		ret = copy_to_user(&aptr->result, &result, 4);
		if (ret) {
			pr_info("copy to user failed 0x%x", ret);
			return -EFAULT;
		}
	break;
	default:
	return -ENOTTY;
	}
	return len;

}

/*
 * Function called when the user side application opens a handle
 * to mydev driver by calling the open() function.
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
	.write = mycalc_write,
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
		printk("Error: Failed registering major number\n");
		return ret;
	}

	/* save MAJOR number */
	major = MAJOR(devid);

	/* Initalize the cdev structure */
	cdev_init(&mydev, &mydev_fops);

	/* Register the char device with the kernel */
	ret = cdev_add(&mydev, devid, 1);
	if (ret) {
		printk("Error: Failed registering with the kernel\n");
		unregister_chrdev_region(devid, 1);
		return ret;
	}

	/* Create a class and register with the sysfs. (Failure is not fatal) */
	mydev_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(mydev_class)) {
		printk("class_create() failed: %ld\n", PTR_ERR(mydev_class));
		mydev_class = NULL;
	} else {
		/* Register device with sysfs (creates device node in /dev) */
		mydev_device = device_create(mydev_class, NULL,
					devid, NULL, DEVICE_NAME"0");
		if (IS_ERR(mydev_device)) {
			printk("device_create() failed: %ld\n", PTR_ERR(mydev_device));
			mydev_device = NULL;
		}
	}

	printk("Driver \"%s\" loaded...\n", DEVICE_NAME);
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

	printk("Driver \"%s\" unloaded...\n", DEVICE_NAME);
}

module_init(my_calc_init);
module_exit(my_calc_exit);

MODULE_AUTHOR("Robert y Lasa");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Character Driver.");



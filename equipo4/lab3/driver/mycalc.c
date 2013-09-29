#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include "operations.h"
#include "mycalc.h"

#define DEVICE_NAME	"LAB3_DEV"

static unsigned major;                  /* major number */
static struct cdev my_dev;                  /* to register with kernel */
static struct class *mydev_class = NULL;	/* to register with sysfs */
static struct device *mydev_device;         /* to register with sysfs */

static char operation[2];
static int first;
static int second;
static char val_valid[15] = {"0123456789-+*/"};

static struct operation_t k_operation = {
	.num1 = 0,
	.num2 = 0,
	.operator_type = MYCALC_ADD,
	.result = 0
};  /* kernel buffer operation object */

static int lab3_open(struct inode *ip, struct file *filp)
{
	printk(KERN_INFO "[%d] %s\n", __LINE__, __func__);
	return 0;
}

static int lab3_close(struct inode *ip, struct file *filp)
{
	printk(KERN_INFO "[%d] %s\n", __LINE__, __func__);
	return 0;
}

static ssize_t lab3_read(struct file *filp, char __user *buf,
					size_t nbuf, loff_t *offs)
{
	int status, compute, lenght;
	char msg_buf[40];
	status = 0;
	printk(KERN_INFO "[%d] %s\n", __LINE__, __func__);

	compute = compute_operation(&k_operation);
	lenght = get_lenght_number(k_operation.result) + 15;
	printk(KERN_INFO "[%d] lenght:%d\n", __LINE__, lenght);

	if ((nbuf + *offs) > lenght)
		nbuf = lenght - *offs;

	if (compute != -1) {
		sprintf(msg_buf, "Result is %d\n", k_operation.result);
		status = copy_to_user(buf, msg_buf, lenght);
		if (status) {
			printk(KERN_INFO "Copy failed: %d", status);
			status = -EFAULT;
		} else {
			*offs += nbuf;
			status = nbuf;
		}
	} else
		status = -EINVAL;
	printk(KERN_INFO "%s = %d\n", __func__, status);
	return status;
}

int get_lenght_number(int number)
{
	int size, decimal;
	size = 0;
	decimal = 10;

	while (number > decimal && size < 10) {
		size++;
		decimal = decimal * 10;
	}
	if (number < 0)
		size++;
	return size;
}

static ssize_t lab3_write(struct file *filp, const char __user *buf,
					size_t nbuf, loff_t *offs)
{
	char tmp_input[50];
	int status;
	status = 0;
	printk(KERN_INFO "[%d] %s\n", __LINE__, __func__);

	copy_from_user(&tmp_input[*offs], buf, nbuf);
	status = validate_input(tmp_input, nbuf-1);
	printk(KERN_INFO "status = %d\n", status);

	if (status >= 0) {
		k_operation.num1 = first;
		k_operation.num2 = second;
		k_operation.operator_type =
		    get_definition_operator(operation[0]);
		printk(KERN_INFO "num1: %d\tnum2: %d\toper: %d\n",
			k_operation.num1, k_operation.num2,
			k_operation.operator_type);
	}
	*offs += nbuf;
	return nbuf;
}

int get_definition_operator(char optor)
{
	if (optor == '+')
		return MYCALC_ADD;
	else if (optor == '-')
		return MYCALC_STR;
	else if (optor == '*')
		return MYCALC_MUL;
	else if (optor == '/')
		return MYCALC_DIV;
	else
		return -1;
}

static int validate_input(char *input, int lenght)
{
	char input_temp[50];
	char cfirst[50];
	char csecond[50];
	int i = 0;
	int j = 0;
	int z, w;
	int op_n = 0;
	int op_f = 0;
	int y = 0;

	/* Verificamos caracteres de función y removemos espacios */
	while (i < lenght) {
		if (input[i] != ' ') {
			z = 0;
			w = 0;
			y = 0;
			/* Verificación de caracteres */
			while (val_valid[z] != 0) {
				if (*(input + i) == val_valid[z])
					w++;
				z++;
			}
			if (w == 0) {
				/* Character invalid */
				printk(KERN_INFO "%c %d", *(input + i), i);
				return -2;
			}

			if (*(input + i) == '*' || *(input + i) == '/')
				y++;

			/* Buscamos el operador */
			if (j > 0) {
				if (op_f == 0 && (
				    *(input + i) == '*' ||
				    *(input + i) == '/' ||
				    *(input + i) == '+' ||
				    *(input + i) == '-')) {
					operation[0] = *(input + i);
					op_n = j;
					op_f++;
				}
			}
			input_temp[j] = *(input + i);
			j++;
		}
		i++;
	}

	/* Verificamos operadores */
	if (y > 1) {
		/* Operación no válida o faltan operadores */
		return -6;
	}

	/* Verificamos longitud de string*/
	if (j < 3 || j > 13) {
		/* Operación no válida o faltan operadores */
		return -1;
	}

	i = 0;
	j = 0;
	while (i < op_n) {
		cfirst[i] = input_temp[i];
		if (i > 0 && (
		    *(input_temp + i) == '-' || *(input_temp + i) == '+' ||
		    *(input_temp + i) == '/' || *(input_temp + i) == '*')) {
			/* Hay más de un operador en la función */
			return -3;
		}
		i++;
	}
	i++;
	while (input_temp[i] != 0) {
		csecond[j] = input_temp[i];
		if (j > 0 && (
		    *(input_temp + i) == '-' || *(input_temp + i) == '+' ||
		    *(input_temp + i) == '/' || *(input_temp + i) == '*')) {
			/* Hay más de un operador en la función */
			return -3;
		}
		j++;
		i++;
	}

	if ((cfirst[0] == '*' || cfirst[0] == '/') ||
	    (csecond[0] == '*' || csecond[0] == '/')) {
		/* El número exede el límite de tamaño */
		return -3;
	}

	first = simple_strtol(cfirst, NULL, 0);
	if (first > 32767 || first < 32768) {
		/* El número exede el límite de tamaño */
		return -4;
	}

	second = simple_strtol(csecond, NULL, 0);
	if (second > 32767 || second < 32768) {
		/* El número exede el límite de tamaño */
		return -4;
	}

	return op_n;
}

static long lab3_ioctl(struct file *filp, unsigned int command,
						unsigned long args)
{
	long status;
	int tmp;
	printk(KERN_INFO "[%d] %s\n", __LINE__, __func__);
	status = 0;
	switch (command) {
	case MYCALC_IOC_SET_NUM1:
		if (!copy_from_user(&tmp, (int *)args, sizeof(tmp))) {
			if (validate_number(tmp, -32768, 32768) == -1)
				status = -4;
			else
				k_operation.num1 = tmp;
		} else
		    status = -EACCES;
	break;
	case MYCALC_IOC_SET_NUM2:
		if (!copy_from_user(&tmp, (int *)args, sizeof(tmp))) {
			if (validate_number(tmp, -32768, 32768) == -1)
				status = -4;
			else
				k_operation.num2 = tmp;
		} else
		    status = -EACCES;
	break;
	case MYCALC_IOC_SET_OPERATION:
		if (!copy_from_user(&tmp, (int *)args, 4)) {
			if (validate_number(tmp, 0, 3) == -1)
				status = -2;
			else
				k_operation.operator_type = tmp;
		} else
		    status = -EACCES;
	break;
	case MYCALC_IOC_GET_RESULT:
		if (compute_operation(&k_operation) != -1) {
			if (copy_to_user((int *)args,
			    &k_operation.result, sizeof(tmp)))
				status = -EINVAL;
		} else
			status = -EINVAL;
	break;
	case MYCALC_IOC_DO_OPERATION:
		if (!copy_from_user(&k_operation,
		    (struct operation_t *)args, sizeof(k_operation))) {
			if (compute_operation(&k_operation) != -1) {
				if (copy_to_user((struct operation_t *)args,
				    &k_operation, sizeof(k_operation)))
					status = -EINVAL;
			} else
				status = -EINVAL;
		} else
		    status = -EACCES;
	break;
	default:
		status = -EINVAL;
	}
	printk(KERN_INFO "num1: %d\tnum2: %d\toper: %d\tresult: %d\n",
		k_operation.num1, k_operation.num2,
		k_operation.operator_type, k_operation.result);
	return status;
}

int compute_operation(struct operation_t *operation)
{
	int status;
	long result;
	status = 0;
	result = 0;
	switch ((*operation).operator_type) {
	case MYCALC_ADD:
	    result = add((*operation).num1, (*operation).num2);
	break;
	case MYCALC_STR:
		result = substract((*operation).num1, (*operation).num2);
	break;
	case MYCALC_DIV:
		result = divide((*operation).num1, (*operation).num2);
	break;
	case MYCALC_MUL:
		result = multiply((*operation).num1, (*operation).num2);
	break;
	default:
		status = -1;
	}
	(*operation).result = (int)result;
	printk(KERN_INFO "result: %li\n", result);
	if (validate_number(result, -2147483647, 2147483647) == 0)
		return status;
	else
		return -1;
}

int validate_number(int value, int min, int max)
{
	if (value >= min && value <= max)
		return 0;
	else
		return -1;
}

static const struct file_operations mydev_fops = {
	.owner = THIS_MODULE,
	.read = lab3_read,
	.write = lab3_write,
	.unlocked_ioctl = lab3_ioctl,
	.open = lab3_open,
	.release = lab3_close,
};

static int __init mymodule_init(void)
{
	dev_t dev_id;
	int ret;

	/* Allocate major/minor numbers */
	ret = alloc_chrdev_region(&dev_id, 0, 1, DEVICE_NAME);
	if (ret) {
		printk(KERN_INFO "Error: Failed registering major number\n");
		return -1;
	}
	/* save MAJOR number */
	major = MAJOR(dev_id);

	/* Initialize cdev structure and register it with the kernel*/
	cdev_init(&my_dev, &mydev_fops);
	my_dev.owner = THIS_MODULE;
	/* Register the char device with the kernel */
	ret = cdev_add(&my_dev, dev_id, 1);
	if (ret) {
		printk(KERN_INFO "Error: Failed registering with the kernel\n");
		unregister_chrdev_region(dev_id, 1);
		return -1;
	}

	/* Create a class and register with the sysfs. (Failure is not fatal) */
	mydev_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(mydev_class)) {
		printk(KERN_INFO "class_create() failed: %ld\n",
						PTR_ERR(mydev_class));
		mydev_class = NULL;
	} else {
		/* Register device with sysfs (creates device node in /dev) */
		mydev_device = device_create(mydev_class, NULL, dev_id,
							NULL, DEVICE_NAME"0");
		if (IS_ERR(mydev_device)) {
			printk(KERN_INFO "device_create() failed: %ld\n",
						PTR_ERR(mydev_device));
			mydev_device = NULL;
		}
	}

	printk(KERN_INFO "Driver \"%s\" loaded...\n", DEVICE_NAME);
	return 0;
}

static void __exit mymodule_exit(void)
{
	/* Deregister services from kernel */
	cdev_del(&my_dev);

	/* Release major/minor numbers */
	unregister_chrdev_region(MKDEV(major, 0), 1);

	/* Undone everything initialized in init function */
	if (mydev_class != NULL) {
		device_destroy(mydev_class, MKDEV(major, 0));
		class_destroy(mydev_class);
	}

	printk(KERN_INFO "Driver \"%s\" unloaded...\n", DEVICE_NAME);
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_AUTHOR("Equipo 4 - Lab 3");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Char driver - Lab 3");


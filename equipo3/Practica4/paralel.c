/*
 * Parallel port sample
 * Jorge Paz / Alejandro del Rio
 * Equipo 3
 * Github URL: (falta)
 * Google docs report: https://docs.google.com/document/d/1xz2cTTLxF0HPI6LWHoKaDr1MdFgZujwLCCi9A55sN-Y/edit
 * Use write with echo on /dev: echo -n -e "\xAA" > parallel0
 * Use read with cat on /dev: cat parallel0 | od -dAn
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/signal.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include "config.h"
#include <linux/stat.h>
#include <linux/delay.h>

#define DEVICE_NAME			"parallel"	/* Our module name */
#define IOC_MAGIC			0x94
#define BASEPORT			0x378		/* May change with \
							a different platform */

#define PARA_IOC_SET_WRITE	_IO(IOC_MAGIC, 0x1)	/* To set data pins \
							as outputs */
#define PARA_IOC_SET_READ	_IO(IOC_MAGIC, 0x2)	/* To set data pins \
							as inputs*/
#define PARA_IOC_SET_STROBE	_IO(IOC_MAGIC, 0x3)	/* To set strobe to 1 */
#define PARA_IOC_CLEAR_STROBE	_IO(IOC_MAGIC, 0x4)	/* To set strobe to 0 */

#define READ_PARALLEL 0
#define WRITE_PARALLEL 1
DECLARE_WAIT_QUEUE_HEAD(program_queue);

static unsigned major;				/* Major number */
static struct cdev my_dev;			/* Register with kernel */
static struct class *mydev_class;		/* Register with sysfs */
static struct device *mydev_device;		/* Register with sysfs */
int size, tmp;
static int parallelPort;
char kbuff[128];

long num1;
long num2;
char operand;
long result;
int dataReadyFlag;
int portMode;
int port;
int instances;
uint8_t byte;

bool searchForIRQ = true;
bool readInterrupts = 1; /* Change to allow interrupts from IRQ \
	(in the module Y equal 1, and N equals 0) */

/* Modify a parameter without recompiling */
module_param(readInterrupts, bool, S_IRUGO | S_IWUSR);

/* Reset the irq pin, needed to enable future interrupts */
static void resetIRQ(void)
{
	byte = inb(0x37A);
	byte = byte & 0xEF; /* 0xEF = binary 11101111 */
	outb(byte, 0x37A);
	mb(); /* Avoid compiler optimization that might \
			cause errors */
	byte = inb(0x37A);
	byte = byte | 0x10; /* 0x10 = binary 00010000 */
	outb(byte, 0x37A);
}

/* Interrupt service */
static irqreturn_t isr_code(int irq, void *dev_id)
{
	resetIRQ();
	pr_info("IRQ executed\n");
	dataReadyFlag = 1;
	wake_up_interruptible(&program_queue);
	return IRQ_HANDLED;
}

/* This functions is called when the device is read */
static ssize_t my_read(struct file *filp, char __user *buf,
					size_t nbuf, loff_t *offs)
{
	if (readInterrupts == 1) {
		/* Wait until data ready and port in read mode */
		if (wait_event_interruptible(program_queue,
			(dataReadyFlag == 1) && (portMode == READ_PARALLEL))) {
			pr_info("Error in wait.\n");
			return -ERESTARTSYS;
		}
	}

	/* Reads port */
	kbuff[0] = inb(0x378);
	pr_info("%x\n", kbuff[0]);
	/* Sets flag */
	dataReadyFlag = 0;

	/* Transfers data to user space */
	copy_to_user(buf, &kbuff[0], 1);

	/* Manages offset */
	if (*offs == 0) {
		*offs += 1;
		return 1;
	} else {
		return 0;
	}

}

/* Function called when data is written to the device */
static ssize_t my_write(struct file *filp, const char __user *buf,
						size_t nbuf, loff_t *offs)
{
	/* Copy from user space */
	copy_from_user(kbuff, buf, 1);
	pr_info("%s:%d\n", __func__, __LINE__);
	pr_info("Write: %X", kbuff[0]);

	/* Writes to the port */
	outb(kbuff[0], 0x378);

	/* Output for user */
	pr_info("Finished\n");
	sprintf(kbuff, "Finished\n");
	size = strlen(kbuff);

	return 1;
}

/* Function called when the device is open */
static int my_open(struct inode *ip, struct file *filp)
{
	int ret;
	int i;
	unsigned long probeMask;
	int irqNumber = -1;

	if (searchForIRQ == true) {
		parallelPort = 7; /* Assume that the IRQ number is 7 */

		/* Check the correct IRQ automatically */
		probeMask = probe_irq_on();

		/* Cycle */
		for (i = 0; i < 10; i++) {
			/* Activate irq on control */
			resetIRQ();

			/* Strobe functions */
			/* Set strobe to 1 */
			byte = inb(0x37A);
			byte = byte | 0x01;
			outb(byte, 0x37A);
			mb(); /* Avoid compiler optimization that /
				might cause errors */
			/* Set strobe to 0 */
			byte = inb(0x37A);
			byte = byte & 0xFE;
			outb(byte, 0x37A);

			mdelay(5); /* Delay for 5 ms */

			/* Get the IRQ number */
			irqNumber = probe_irq_off(probeMask);
			if (irqNumber > 0) {
				searchForIRQ = false;
				pr_info("IRQ found at: %i\n", irqNumber);
				parallelPort = irqNumber;
				break;
			}
		}

		/* Check if not found */
		if (irqNumber <= 0)
			pr_info("IRQ not found, using default IRQ number (7), may be wrong.\n");

		pr_info("Using IRQ: %i", irqNumber);
	}

	/* Keep track of the number of instances */
	instances++;

	if (readInterrupts == 1) {
		if (instances == 1) {
			/* Request ISR */
			/* Enable interrupts on pin */
			ret = request_irq(parallelPort, isr_code,
				IRQF_DISABLED, "parallelport", 0);
			if (ret) {
				pr_info("parint: error requesting irq 7: \
					returned %d\n", ret);
				pr_info(">cannot register IRQ %d\n", 7);
				return -EIO;
			}

			outb_p(0x10, BASEPORT + 2);
			pr_info("Interrupt enabled.\n");
		}
	}

	/* Log */
	pr_info("%s:%d\n", __func__, __LINE__);
	return 0;
}

/* Closes the driver device, frees the irq */
static int my_close(struct inode *ip, struct file *filp)
{
	/* Keep track of the number of instances */
	instances--;

	if (readInterrupts == 1) {
		if (instances == 0) {
			/* Disable interrupts on pin */
			byte = inb(0x37A);
			byte = byte & 0xEF; /* hex EF = binary 11101111 */
			outb(byte, 0x37A);

			/* Free ISR */
			/*disable_irq(7);*/
			free_irq(7, NULL);
			pr_info("Interrupt disabled.\n");
		}
	}

	/* Log */
	pr_info("%s:%d\n", __func__, __LINE__);
	return 0;
}

/* IOCTL caller function */
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	pr_info("CMD entered%d\n", cmd);

	switch (cmd) {
	case PARA_IOC_SET_WRITE:
		/* Configure data pins as outputs */
		byte = inb(0x37A);
		byte = byte & 0xDF; /* 0xDF = binary 11011111 */
		outb(byte, 0x37A);
		portMode = WRITE_PARALLEL;
		pr_info("Write mode\n");
		break;

	case PARA_IOC_SET_READ:
		/* Configure data pins as inputs */
		byte = inb(0x37A);
		byte = byte | 0x20; /* 0x10 = binary 00100000 */
		outb(byte, 0x37A);
		portMode = READ_PARALLEL;
		pr_info("Read mode\n");
		break;

	case PARA_IOC_SET_STROBE:
		/* Set strobe to 1 */
		byte = inb(0x37A);
		byte = byte | 0x01;
		outb(byte, 0x37A);
		pr_info("Strobe set to 1\n");
		break;

	case PARA_IOC_CLEAR_STROBE:
		/* Clear strobe to 0*/
		byte = inb(0x37A);
		byte = byte & 0xFE;
		outb(byte, 0x37A);
		pr_info("Strobe set to 0\n");
		break;

	default:
		return -ENOTTY;
		break;
	}

	return 0;
}

/* File operations struct */
static const struct file_operations mydev_fops = {
	.owner = THIS_MODULE,
	.read = my_read,
	.write = my_write,
	.unlocked_ioctl = device_ioctl,
	.open = my_open,
	.release = my_close,
};

/* Module init function */
static int __init mymodule_init(void)
{
	dev_t dev_id;
	int ret;

	/* Allocate major/minor numbers */
	ret = alloc_chrdev_region(&dev_id, 0, 1, DEVICE_NAME);

	if (ret) {
		pr_info("Error: Failed registering major number\n");
		return -1;
	}

	/* save MAJOR number */
	major = 0;
	major = MAJOR(dev_id);

	/* Initialize cdev structure and register it with the kernel*/
	cdev_init(&my_dev, &mydev_fops);
	my_dev.owner = THIS_MODULE;

	/* Register the char device with the kernel */
	ret = cdev_add(&my_dev, dev_id, 1);

	if (ret) {
		pr_info("Error: Failed registering with the kernel\n");
		unregister_chrdev_region(dev_id, 1);
		return -1;
	}

	/* Create a class and register with the sysfs. (Failure is not fatal) */
	mydev_class = NULL;
	mydev_class = class_create(THIS_MODULE, DEVICE_NAME);

	if (IS_ERR(mydev_class)) {
		pr_info("class_create() failed: %ld\n", PTR_ERR(mydev_class));
		mydev_class = NULL;
	} else {
		/* Register device with sysfs (creates device node in /dev) */
		mydev_device = device_create(mydev_class,
			NULL, dev_id, NULL, DEVICE_NAME "0");

		if (IS_ERR(mydev_device)) {
			pr_info("device_create() failed: %ld\n",
					PTR_ERR(mydev_device));
			mydev_device = NULL;
		}
	}

	/* Claim parallel port region */
	port =  check_region(0x378, 3);
	if (port) {
		pr_info("<1>parllel port: unable to  reserve 0x378-0x37A\n");
		return -1;
	}
	request_region(0x378, 3, DEVICE_NAME);

	/* Init some variables */
	portMode = WRITE_PARALLEL;

	pr_info("Driver \"%s\" loaded...\n", DEVICE_NAME);
	return 0;
}

/* Module exit function */
static void __exit mymodule_exit(void)
{
	/* Free parallel port */
	release_region(0x378, 3);

	/* Deregister services from kernel */
	cdev_del(&my_dev);

	/* Release major/minor numbers */
	unregister_chrdev_region(MKDEV(major, 0), 1);

	/* Undo everything initialized in init function */
	if (mydev_class != NULL) {
		device_destroy(mydev_class, MKDEV(major, 0));
		class_destroy(mydev_class);
	}

	pr_info("Driver \"%s\" unloaded...\n", DEVICE_NAME);
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_AUTHOR("Jorge Paz / Alejandro del Rio - Equipo 3");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Parallel port driver");

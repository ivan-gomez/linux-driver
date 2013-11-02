#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include "parallel.h"

#define DEVICE_NAME	"PARALLEL_DEV"
#define BASE		0x378
#define STATUS		0x379
#define CONTROL		0x37A

DECLARE_WAIT_QUEUE_HEAD(program_queue);

static unsigned major;			/* major number */
static struct cdev parallel_cdev;		/* to register with kernel */
static struct class *parallel_class;	/* to register with sysfs */
static struct device *parallel_device;	/* to register with sysfs */
static int irq_id;

static int interrupts_enabled = ON;	/* Flag to enable interrupts mode */
static int irq_handler_flag = OFF;		/* Flag for handler interrupt */
static int parallel_mode = WRITE_MODE;	/* Parallel mode status */
static int instances;			/* Instances Count status */

/*
 * irqreturn_t parrallel_interrupt()
 *  Gets interrupted when a signal comes through IRQ
 * Parameters:
 *  irq_id - Id of the IRQ
 * Returns:
 *  -none-
 */
static irqreturn_t parrallel_interrupt(int irq, void *dev_id)
{
	resetInterruptIRQ();
	printk(KERN_INFO "Interrupted!!!\n");
	irq_handler_flag = 1;
	wake_up_interruptible(&program_queue);
	return IRQ_HANDLED;
}


/*
 * resetInterruptIRQ()
 *  Reset the IRQ registers
 * Parameters:
 *  -none-
 * Returns:
 *  -none-
 */
void resetInterruptIRQ()
{
	uint8_t value;
	value = inb(CONTROL);
	outb(value & 0xEF, CONTROL);
	value = inb(CONTROL);
	outb(value | 0x10, CONTROL);
}

/*
 * lab4_open()
 *  Open device driver
 * Parameters:
 *  struct inode ip
 *  struct file fd
 * Returns:
 *  int result
 */
static int lab4_open(struct inode *ip, struct file *filp)
{
	printk(KERN_INFO "[%d] %s\n", __LINE__, __func__);

	instances++;
	if (interrupts_enabled == ON) {
		if (instances == 1) {
			/* Requesting IRQ interrupt */
			irq_id = 7;
			if (request_irq(irq_id, parrallel_interrupt,
				IRQF_DISABLED, "parallelport", 0)) {
				printk(KERN_ERR "Interrupt (%d) requested unsuccessfully\n",
					irq_id);
				return -EIO;
			}
			outb_p(0x10, CONTROL);
			printk(KERN_INFO "Interrupt enabled\n");
		}
	}
	return 0;
}

/*
 * lab4_read()
 *  Read device driver
 * Parameters:
 *  struct file fd
 *  char user buf from user space
 *  size of buf user space
 *  offset of user space bf
 * Returns:
 *  int bytes read
 */
static ssize_t lab4_read(struct file *filp, char __user *buf,
					size_t nbuf, loff_t *offs)
{
	char value;
	int status;
	status = 0;
	printk(KERN_INFO "[%d] %s\n", __LINE__, __func__);

	if (interrupts_enabled) {
		printk(KERN_INFO "In state: wait_event_interruptible()...\n");
		if (wait_event_interruptible(program_queue,
			(irq_handler_flag == 1) &&
			(parallel_mode == READ_MODE))) {
			printk(KERN_INFO "Error in wait interruptible...\n");
			return -ERESTARTSYS;
		}
	}

	value = inb(BASE);
	irq_handler_flag = 0;

	printk(KERN_INFO " Reading %d...\n", value);
	status = copy_to_user(buf, &value, 1);
	if (status) {
		printk(KERN_INFO "Copy failed: %d\n", status);
		return -EFAULT;
	}

	if (*offs == 0) {
		*offs += 1;
		return 1;
	} else
		return 0;
}

/*
 * lab4_write()
 *  Write device driver
 * Parameters:
 *  struct file fd
 *  char user buf that will be written to user space
 *  size of buf user space
 *  offset of user space bf
 * Returns:
 *  int bytes
 */
static ssize_t lab4_write(struct file *filp, const char __user *buf,
					size_t nbuf, loff_t *offs)
{
	uint8_t number;
	printk(KERN_INFO "[%d] %s\n", __LINE__, __func__);

	copy_from_user(&number, buf, 1);

	printk(KERN_INFO " Writing %d...\n", number);
	outb(number, BASE);
	*offs += nbuf;
	return nbuf;
}

/*
 * lab4_ioctl()
 *  Ioctl device driver
 * Parameters:
 *  struct file fd
 *  uint command from user
 *  ulong args to pointer
 * Returns:
 *  int result from command
 */
static long lab4_ioctl(struct file *filp, unsigned int command,
	unsigned long args)
{
	long status;
	status = 0;
	printk(KERN_INFO "[%d] %s\n", __LINE__, __func__);
	switch (command) {
	case PARALLEL_SET_OUTPUT:
		config_parallel_output();
	break;
	case PARALLEL_SET_INPUT:
		config_parallel_input();
	break;
	case PARALLEL_STROBE_ON:
		simulate_parallel_irq_strb_on();
	break;
	case PARALLEL_STROBE_OFF:
		simulate_parallel_irq_strb_off();
	break;
	defaul	1
t:
		status = -EINVAL;
	}
	return status;
}

/*
 * lab4_close()
 *  Close device driver
 * Parameters:
 *  struct inode
 *  struct file fd
 * Returns:
 *  int close result
 */
static int lab4_close(struct inode *ip, struct file *filp)
{
	printk(KERN_INFO "[%d] %s\n", __LINE__, __func__);
	instances--;
	if (interrupts_enabled == ON) {
		if (instances == 0) {
			uint8_t tmp;
			tmp = inb(CONTROL);
			tmp = tmp & 0xEF;
			outb(tmp, CONTROL);

			/* Releasing IRQ interrupt */
			free_irq(irq_id, NULL);
			printk(KERN_INFO "Interrup disabled\n");
		}
	}
	return 0;
}

/*
 * config_parallel_output()
 *  Configure parallel registers as output mode
 * Parameters:
 *  -none-
 * Returns:
 *  -none-
 */
void config_parallel_output()
{
	uint8_t tmp;
	tmp = inb(CONTROL);
	tmp = tmp & 0xDF;
	outb(tmp | 0x10, CONTROL);
	parallel_mode = WRITE_MODE;
}

/*
 * config_parallel_input()
 *  Configure parallel registers as input mode
 * Parameters:
 *  -none-
 * Returns:
 *  -none-
 */
void config_parallel_input()
{
	uint8_t tmp;
	tmp = inb(CONTROL);
	tmp = tmp | 0x20;
	outb(tmp | 0x10, CONTROL);
	parallel_mode = READ_MODE;
}

/*
 * simulate_parallel_irq_strb_on()
 *  Set strobe register from parallel port
 * Parameters:
 *  -none-
 * Returns:
 *  -none-
 */
void simulate_parallel_irq_strb_on()
{
	uint8_t tmp;
	tmp = inb(CONTROL);
	tmp = tmp | 0x01;
	outb(tmp, CONTROL);
	printk(KERN_INFO "PARALLEL_STROBE_ON\n");
}

/*
 * simulate_parallel_irq_strb_off()
 *  Clear strobe register from parallel port
 * Parameters:
 *  -none-
 * Returns:
 *  -none-
 */
void simulate_parallel_irq_strb_off()
{
	uint8_t tmp;
	tmp = inb(CONTROL);
	tmp = tmp & 0xFE;
	outb(tmp, CONTROL);
	printk(KERN_INFO "PARALLEL_STROBE_OFF\n");
}

/*
 * File Operations Structure definition
 */
static const struct file_operations parallel_fops = {
	.owner = THIS_MODULE,
	.read = lab4_read,
	.write = lab4_write,
	.unlocked_ioctl = lab4_ioctl,
	.open = lab4_open,
	.release = lab4_close,
};

/*
 * __init mymodule_init()
 *  Initialization module function
 * Parameters:
 *  -none-
 * Returns:
 *  int module output result
 */
static int __init mymodule_init(void)
{
	dev_t dev_id;
	int ret;

	printk(KERN_INFO "Interrupts enabled: %u\n", interrupts_enabled);

	/* Allocate major/minor numbers */
	ret = alloc_chrdev_region(&dev_id, 0, 1, DEVICE_NAME);
	if (ret) {
		printk(KERN_INFO "Error: Failed registering major number\n");
		return -1;
	}
	/* save MAJOR number */
	major = MAJOR(dev_id);

	/* Initialize cdev structure and register it with the kernel*/
	cdev_init(&parallel_cdev, &parallel_fops);
	parallel_cdev.owner = THIS_MODULE;
	/* Register the char device with the kernel */
	ret = cdev_add(&parallel_cdev, dev_id, 1);
	if (ret) {
		printk(KERN_INFO "Error: Failed registering with the kernel\n");
		unregister_chrdev_region(dev_id, 1);
		return -1;
	}

	/* Create a class and register with the sysfs. (Failure is not fatal) */
	parallel_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(parallel_class)) {
		printk(KERN_INFO "class_create() failed: %ld\n",
						PTR_ERR(parallel_class));
		parallel_class = NULL;
	} else {
		/* Register device with sysfs (creates device node in /dev) */
		parallel_device = device_create(parallel_class, NULL, dev_id,
							NULL, DEVICE_NAME"0");
		if (IS_ERR(parallel_device)) {
			printk(KERN_INFO "device_create() failed: %ld\n",
						PTR_ERR(parallel_device));
			parallel_device = NULL;
		}
	}

	/* Request region for parallel device */
	if (check_region(BASE, 3)) {
		printk(KERN_WARNING "Error checking/reserving region: %X\n",
			BASE);
		return -1;
	}

	/* Claim region for parallel device */
	if (!(request_region(BASE, 3, DEVICE_NAME))) {
		printk(KERN_WARNING "Error requesting region: %X\n", BASE);
		release_region(BASE, 3);
		return ret;
	}

	/* Initialize port mode as write/output */
	parallel_mode = WRITE_MODE;

	printk(KERN_INFO "Driver \"%s\" loaded...\n", DEVICE_NAME);
	return 0;
}

/*
 * __exit mymodule_exit()
 *  Cleanup module function
 * Parameters:
 *  -none-
 * Returns:
 *  int module output result cleanup
 */
static void __exit mymodule_exit(void)
{

	/* Release port region */
	release_region(BASE, 3);

	/* Deregister services from kernel */
	cdev_del(&parallel_cdev);

	/* Release major/minor numbers */
	unregister_chrdev_region(MKDEV(major, 0), 1);

	/* Undone everything initialized in init function */
	if (parallel_class != NULL) {
		device_destroy(parallel_class, MKDEV(major, 0));
		class_destroy(parallel_class);
	}

	printk(KERN_INFO "Driver \"%s\" unloaded...\n", DEVICE_NAME);
}

module_param(interrupts_enabled, int, S_IRUGO);
module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_AUTHOR("Equipo 4 - Lab 4");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Parallel driver - Lab 4");


#include <linux/module.h>	/* Dynamic loading of modules into the kernel */
#include <linux/fs.h>		/* file defs of important data structures */
#include <linux/cdev.h>		/* char devices structure and aux functions */
#include <linux/device.h>	/* generic, centralized driver model */
#include <linux/uaccess.h>	/* for accessing user-space */
#include <linux/slab.h>		/* for kmalloc */

#define KBUFF_MAX_SIZE	1024	/* size of the kernel side buffer */
#define DEVICE_NAME 	"mydev"
#define NUM_DEVICES	10

struct priv_data {
	unsigned minor;
	unsigned major;
};

static unsigned major = 0;			/* major number */
static struct cdev mydev;			/* to register with kernel */
static struct class *mydev_class = NULL;	/* to register with sysfs */
static struct device *mydev_device;		/* to register with sysfs */
static char kbuffers[NUM_DEVICES][KBUFF_MAX_SIZE];	/* kernel side buffer */
static unsigned long kbuffer_sizes[NUM_DEVICES];	/* kernel buffer length */

/*
 * mydev_read()
 * This function implements the read function in the file operation structure.
 */
static ssize_t mydev_read(struct file *filp, char __user *buffer, size_t nbuf,
								loff_t *offset)
{
	int ret = 0;
	struct priv_data *priv_data = filp->private_data;
	char *kbuffer = &(kbuffers[priv_data->minor][0]);
	unsigned long *kbuffer_size = &kbuffer_sizes[priv_data->minor];

	pr_info("kbuffer = %s, size = %ld\n", kbuffer, *kbuffer_size);
	pr_info("Major : %d, Minor : %d\n", priv_data->major, priv_data->minor);
	pr_info("[%d] nbuf = %d, offset = %d", __LINE__, (int)nbuf, (int)*offset);
	/* If offset is greater that size of kbuffer, there is nothing to copy */
	if (*offset >= *kbuffer_size) {
		pr_info("[%d] Offset greater or equal than kbuffer size: %d >= %ld", __LINE__, (int)*offset, *kbuffer_size);
		goto out;
	}

	/* Check for maximum size of kernel buffer */
	if ((nbuf + *offset) > *kbuffer_size)
		nbuf = *kbuffer_size - *offset;

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
static ssize_t mydev_write(struct file *filp, const char __user *buffer,
						size_t nbuf, loff_t *offset)
{
	int ret = 0;
	struct priv_data *priv_data = filp->private_data;
	char *kbuffer = &(kbuffers[priv_data->minor][0]);
	unsigned long *kbuffer_size = &kbuffer_sizes[priv_data->minor];

	pr_info("Major : %d, Minor : %d\n", priv_data->major, priv_data->minor);
	pr_info("[%d] nbuf = %d, offset = %d", __LINE__, (int)nbuf, (int)*offset);

	if (*offset >= KBUFF_MAX_SIZE) {
		pr_info("[%d] Offset greater or equal than kbuffer size: %d >= %ld", __LINE__, (int)*offset, *kbuffer_size);
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
	*kbuffer_size = *offset;
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
	struct priv_data *p_data;

	p_data = kmalloc(sizeof(p_data), GFP_KERNEL);
	if (p_data == NULL)
		return -ENOMEM;

	p_data->major = imajor(ip);
	p_data->minor = iminor(ip);

	filp->private_data = p_data;
	pr_info("Driver's open function was called\n");

	return 0;
}

/*
 * Function called when the user side application closes a handle to mydev driver
 * by calling the close() function.
 */

static int mydev_close(struct inode *ip, struct file *filp)
{
	kfree(filp->private_data);
	pr_info("Driver's close function was called\n");

	return 0;
}


/*
 * File operation structure
 */
static const struct file_operations mydev_fops = {
	.open = mydev_open,
	.release = mydev_close,
	.owner = THIS_MODULE,
	.read = mydev_read,
	.write = mydev_write
};

/*
 * init my_calc_init()
 * Module init function. It is called when insmod is executed.
 */
static int __init my_calc_init (void)
{
	int ret;
	int minor;
	dev_t devid;

	/* Dynamic allocation of MAJOR and MINOR numbers */
	ret = alloc_chrdev_region(&devid, 0, NUM_DEVICES, DEVICE_NAME);

	if (ret) {
		printk("Error: Failed registering major number\n");
		return ret;
	}

	/* save MAJOR number */
	major = MAJOR(devid);

	/* Initalize the cdev structure */
	cdev_init(&mydev, &mydev_fops);

	/* Register the char device with the kernel */
	ret = cdev_add(&mydev, devid, NUM_DEVICES);
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
		for (minor = 0; minor < NUM_DEVICES; minor++) {
			mydev_device = device_create(mydev_class, NULL, MKDEV(major, minor), NULL, DEVICE_NAME"%d", minor);
			if (IS_ERR(mydev_device)) {
				printk("device_create() failed: %ld\n", PTR_ERR(mydev_device));
				mydev_device = NULL;
			}
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
	int minor;

	/* Deregister char device from kernel */
	cdev_del(&mydev);

	/* Release MAJOR and MINOR numbers */
	unregister_chrdev_region(MKDEV(major, 0), NUM_DEVICES);

	/* Deregister device from sysfs */
	if (mydev_class != NULL) {
		for (minor = 0; minor < NUM_DEVICES; minor++) {
			device_destroy(mydev_class, MKDEV(major, minor));
		}
		class_destroy(mydev_class);
	}

	printk("Driver \"%s\" unloaded...\n", DEVICE_NAME);
}  

module_init(my_calc_init);
module_exit(my_calc_exit);

MODULE_AUTHOR("Ivan Gomez Castellanos"); 
MODULE_LICENSE("GPL"); 
MODULE_DESCRIPTION("Character Driver.");



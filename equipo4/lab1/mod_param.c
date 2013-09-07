#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include "myfuncs.h"

static int value = 1;

/*Init Module Function*/
static int __init lab1_init(void)
{
	pr_info("-----------------------\n");
	pr_info("Lab1 initing started...\n");
	pr_info("Value = %d\n", value);
	pr_info("Lab1 initing exited...\n");
	return 0;
}

/*Exit Module Function*/
static void __exit lab1_exit(void)
{
	pr_info("Lab1 exit started...\n");
	int final_value = factorial(value);

	/*Check negative input factorial*/
	if (final_value != -1)
		pr_info("Factorial de %d es %d\n", value, final_value);
	else
		pr_info("Factorial de %d can not be computed. Only possitive values allowed\n",
			 value);
	pr_info("Lab1 exit finished...\n");
}


module_param(value, int, S_IRUGO);
module_init(lab1_init);
module_exit(lab1_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("felipe.galindo.sanchez@intel.com");
MODULE_DESCRIPTION("Lab1 - LDD");

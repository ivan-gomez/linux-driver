#include <linux/module.h>
#include <linux/stat.h>
#include "myfuncs.h"

int result=0;
static int num=1;
module_param(num,int,S_IRUGO);

static int __init hello_init(void){

	pr_info("Factorial init\n");
	printk("El numero den entrada es: %d\n", num);
	return 0;
}

static void __exit hello_exit(void){
	
	pr_info("Factorial exit!!\n");
	result=funcx(num);
	printk("El factorial de %d es %d\n", num, result);
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jorge Paz, Alejandro DR");
MODULE_DESCRIPTION("Prac1");

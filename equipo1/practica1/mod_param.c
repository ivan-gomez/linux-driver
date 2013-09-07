#include <linux/module.h>
#include "myfuncs.h"

static int module_param = 1;

static int __init hello_init(void)
{
pr_info("%i\n", module_param);
return 0;
}

static void __exit hello_exit(void)
{
pr_info("%i\n", factorial(module_param));
}

module_init(hello_init);
module_exit(hello_exit);
module_param(module_param, int, (S_IWUSR|S_IRUSR));
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Robert");
MODULE_DESCRIPTION("Practica1");

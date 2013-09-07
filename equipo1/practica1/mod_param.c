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
/* Validando que los valores del parametro esten en un rango aceptable */
if (module_param < 0 || module_param > 50)
	pr_info("Parametro invalido, proporcione uno entre el rango 0-50");
else
	pr_info("%i\n", factorial(module_param));
}

module_init(hello_init);
module_exit(hello_exit);
module_param(module_param, int, (S_IWUSR|S_IRUSR));
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Robert y Lasa");
MODULE_DESCRIPTION("Practica1");

#include <linux/module.h>
#include <linux/kernel.h>
#include "myfuncs.h"

static int value = 1;
module_param(value, int, 0764);//Variable que se puede ingresar su valor desde el insmod

static int __init mod_param_init(void)
{
	/*initialization code here*/
	pr_info("value = %d\n",value);
	return 0;
}

static void __exit mod_param_exit(void)
{
	/*cleanup code here*/
	if (value < 0)
		pr_info("ERROR no existe factorial de numero negativo \n");
	else
		pr_info("factorial = %d\n", factorial(value));   //si el valor tiene factorial, se manda llamar la funcion para obtenerlo.
}

module_init(mod_param_init);
module_exit(mod_param_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Team 2");
MODULE_DESCRIPTION("Mod Module");

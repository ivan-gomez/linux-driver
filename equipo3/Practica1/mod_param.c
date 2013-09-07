#include <linux/module.h>
#include <linux/stat.h>
#include "myfuncs.h"

int result = 1;
static int num = 1;
module_param(num, int, S_IRUGO);
/*Init Module Function*/
static int __init hello_init(void){

	pr_info("Factorial init\n");
	pr_info("El numero de entrada es: %d\n", num);
	return 0;
}
/*Exit Module Function*/
static void __exit hello_exit(void){

	pr_info("Factorial exit!!\n");
/*Validar si el número queda entre los parametros correctos*/
	if ((num <= 15) && (num >= 0)) {
		result = funcx(num);
		pr_info("El factorial de %d es %d\n", num, result);
	} else{
		pr_info("Parametro Invalido. Ingresar numero de 0-15\n");
	}
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jorge Paz, Alejandro DR");
MODULE_DESCRIPTION("Prac1");

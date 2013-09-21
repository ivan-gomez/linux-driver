#include <linux/module.h>
#include <linux/kernel.h>
#include "myfuncs.h"

/*Funcion recursiva para calcular el factorial de un numero*/
int factorial(int numero)
{
	if (numero == 0)
		return 1;
	else
		return numero*factorial(numero-1);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Team 2");
MODULE_DESCRIPTION("Factorial Module");


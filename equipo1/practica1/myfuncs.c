#include "myfuncs.h"
/* Funcion factorial, recibe un parametro mayor a 0 y menor o igual a 50
   como precondicion y calcula el factorial de ese numero de manera
   recursiva. */
int factorial(int n)
{
if (n < 2)
	return 1;
else
	return n * factorial(n-1);
}


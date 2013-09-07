#include "myfuncs.h"

int factorial(int i)
{
	/* Validate positive numbers */
	if (i >= 0) {
		if (i == 0)
			return 1;
		else
			return i * factorial(i-1);
	} else
		return -1;
}

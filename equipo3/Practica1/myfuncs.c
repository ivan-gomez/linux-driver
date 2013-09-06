#include "myfuncs.h"

int funcx(int number){
	if (number == 0) {
		return 1;
	} else {
		return (number * funcx(number - 1));
	}
}

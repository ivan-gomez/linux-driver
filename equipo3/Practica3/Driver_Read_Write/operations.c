#include "operations.h"

int oper(int num1, int num2, char operation)
{

	switch (operation) {
	case '+':
		return num1 + num2;
		break;
	case '*':
		return num1 * num2;
		break;
	case '/':
		return num1 / num2;
		break;
	case '-':
		return num1 - num2;
		break;
	}
	return 0;
}

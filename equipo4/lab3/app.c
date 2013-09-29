#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include "driver/mycalc.h"
#include "app.h"

int fd;

void doActivities()
{
	setNum1(4545);
	setNum2(5);
	setOperation(8);
	getResult();

	doOperation(10, 51, MYCALC_ADD);

	setNum1(4545);
	setNum2(5);
	setOperation(MYCALC_DIV);
	getResult();

	doOperation(740, -4551, MYCALC_MUL);
}

int main()
{
	int rc, ic;
	fd = open(DEVICE, O_RDWR);
	if (fd < 0) {
		printf("Error al abrir el dispositivo\n");
		return fd;
	}
	printf("Open\n");

	doActivities();

	rc = close(fd);
	if (rc < 0) {
		printf("Error al cerrar el dispositivo\n");
		return rc;
	}
	printf("Close\n");

	return 0;
}

/* Calc functions API interface */

void setNum1(int num)
{
	int a = num;
	printf("SET_NUM1(%d) = %d\n", a, ioctl(fd, MYCALC_IOC_SET_NUM1, NULL));
}

void setNum2(int num)
{
	int a = num;
	printf("SET_NUM2(%d) = %d\n", a, ioctl(fd, MYCALC_IOC_SET_NUM2, &a));
}

void setOperation(int op)
{
	int a = op;
	printf("SET_OPERATION(%d) = %d\n", a,
	    ioctl(fd, MYCALC_IOC_SET_OPERATION, &a));
}

void getResult()
{
	int a;
	printf("GET_RESULT() = %d\n", ioctl(fd, MYCALC_IOC_GET_RESULT, &a));
	printf("\tResult:%d\n", a);
}

void doOperation(int num1, int num2, int op)
{
	struct operation_t user_operation = {
		.num1 = num1,
		.num2 = num2,
		.operator_type = op,
		.result = 0
	};
	printf("DO_OPERATION() = %d\n",
	    ioctl(fd, MYCALC_IOC_DO_OPERATION, &user_operation));
	printf("\tNum1:%d\n", user_operation.num1);
	printf("\tNum2:%d\n", user_operation.num2);
	printf("\tOperator:%d\n", user_operation.operator_type);
	printf("\tResult:%d\n", user_operation.result);
}

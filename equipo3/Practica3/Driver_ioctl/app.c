#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>

#define DEVICE "/dev/mydev0"
#define BUFF_SIZE	10

#define MYCALC_IOC_MAGIC 'C'
#define MYCALC_IOC_SET_NUM1		_IOW(MYCALC_IOC_MAGIC, 0, int)
#define MYCALC_IOC_SET_NUM2		_IOW(MYCALC_IOC_MAGIC, 1, int)
#define MYCALC_IOC_SET_OPERATION	_IOW(MYCALC_IOC_MAGIC, 2, int)
#define MYCALC_IOC_GET_RESULT		_IOR(MYCALC_IOC_MAGIC, 3, int)
#define MYCALC_IOC_DO_OPERATION		_IOWR(MYCALC_IOC_MAGIC, 4, int)

struct struc_func {
	int number1;
	int number2;
	char operator;
	int result;
};

int main()
{
	/* File operation variables */
	int fd;
	int rc;
	char buff[BUFF_SIZE];
	/* IOCTL operation values */
	int first_number;
	int second_number;
	char op;
	int res;

	fd = open(DEVICE, O_RDWR);
	if (fd < 0) {
		printf("Error al abrir el dispositivo\n");
		return fd;
	}
	printf("\nDevice opened\n");

	/* Operation values */
	first_number = 100;
	second_number = 200;
	op = '*';
	res = 0;

	/* Operation structure */
	struct struc_func my_operation = {
		.number1 = first_number,
		.number2 = second_number,
		.operator = op,
		.result = res
	};

	/* Operation via variables */
	printf("\nOperation via variables\n");
	ioctl(fd, MYCALC_IOC_SET_NUM1, &first_number);
	printf("Number 1 set\n");
	ioctl(fd, MYCALC_IOC_SET_NUM2, &second_number);
	printf("Number 2 set\n");
	ioctl(fd, MYCALC_IOC_SET_OPERATION, &op);
	printf("Operation set\n");
	ioctl(fd, MYCALC_IOC_GET_RESULT, &res);
	printf("Result called\n");
	printf("Result is %i\n", res);

	/* Operation via structure */
	printf("\nOperation via structure\n");
	ioctl(fd, MYCALC_IOC_DO_OPERATION, &my_operation);
	printf("Do operation called\n");

	printf("my_operation.number1 is %i\n", my_operation.number1);
	printf("my_operation.number2 is %i\n", my_operation.number2);
	printf("my_operation.operation is %c\n", my_operation.operator);
	printf("my_operation.result is %i\n", my_operation.result);
	/* End call ioctl */

	rc = close(fd);
	if (rc < 0) {
		printf("Error al cerrar el dispositivo\n");
		return rc;
	}
	printf("\nDevice closed\n");

	return 0;
}

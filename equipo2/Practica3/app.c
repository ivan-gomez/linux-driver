#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define DEVICE "/dev/mydev0"
#define BUFF_SIZE	10

#define MYCALC_IOC_MAGIC 'C'
#define MYCALC_IOC_SET_NUM1	_IOW(MYCALC_IOC_MAGIC, 0, int)
#define MYCALC_IOC_SET_NUM2	_IOW(MYCALC_IOC_MAGIC, 1, int)
#define MYCALC_IOC_SET_OPERATION	_IOW(MYCALC_IOC_MAGIC, 2, int)
#define MYCALC_IOC_SET_RESULT		_IOR(MYCALC_IOC_MAGIC, 3, int)
#define MYCALC_IOC_DO_OPERATION		_IORW(MYCALC_IOC_MAGIC, 4, int)

int main(int argc, char **argv)
{
	int fd;
	int rc;
	int res, num1, num2, op;
	char buff[BUFF_SIZE];
	char operation;
	if (argc != 4) {
		printf("Need to have this Format:\n");
		printf("./app <num1> <operator in string format> <num2>\n");
		return -1;
	}
	if (strcmp(argv[2], "/") == 0) {
		op = 4;
		operation = '/';
	} else if (strcmp(argv[2], "*") == 0) {
		op = 3;
		operation = '*';
	} else if (strcmp(argv[2], "-") == 0) {
		op = 2;
		operation = '-';
	} else if (strcmp(argv[2], "+") == 0) {
		op = 1;
		operation = '+';
	} else {
		printf("Wrong format, follow the example:\n");
		printf("./app 4 * 5\n");
		return -1;
	}
	if (strcmp(argv[1], "0") != 0 && atoi(argv[1]) == 0) {
		printf("num1 value must be between -65536 and 65535\n");
		return -1;
	} else if (atoi(argv[1]) <= -65536 || atoi(argv[1]) > 65535) {
		printf("num1 value must be between -65536 and 65535\n");
		return -1;
	}
	if (strcmp(argv[3], "0") != 0 && atoi(argv[3]) == 0) {
		printf("num2 value must be between -65536 and 65535\n");
		return -1;
	} else if (atoi(argv[3]) <= -65536 || atoi(argv[3]) > 65535) {
		printf("num2 value must be between -65536 and 65535\n");
		return -1;
	}
	num1 = atoi(argv[1]);
	num2 = atoi(argv[3]);
	fd = open(DEVICE, O_RDWR);
	if (fd < 0) {
		printf("Error al abrir el dispositivo\n");
		return fd;
	}
	if (ioctl(fd, MYCALC_IOC_SET_NUM1, &num1) < 0)
		perror("1 ioctl");
	if (ioctl(fd, MYCALC_IOC_SET_NUM2, &num2) < 0)
		perror("2 ioctl");
	if (ioctl(fd, MYCALC_IOC_SET_OPERATION, &op) < 0)
		perror("3 ioctl");
	if (ioctl(fd, MYCALC_IOC_SET_RESULT, &res) < 0)
		perror("4 ioctl");
	printf("%i %c %i = %i\n", num1, operation, num2, res);
	rc = close(fd);
	if (rc < 0) {
		printf("Error al cerrar el dispositivo\n");
		return rc;
	}
	return 0;
}

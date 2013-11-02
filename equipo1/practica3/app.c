#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#define DEVICE "/dev/mycalc0"
#define BUFF_SIZE 10

#define MYCALC_IOC_MAGIC 'C'
#define MYCALC_IOC_SET_NUM1	_IOW(MYCALC_IOC_MAGIC, 0, int)
#define MYCALC_IOC_SET_NUM2	_IOW(MYCALC_IOC_MAGIC, 1, int)
#define MYCALC_IOC_SET_OPERATION	_IOW(MYCALC_IOC_MAGIC, 2, int)
#define MYCALC_IOC_GET_RESULT		_IOR(MYCALC_IOC_MAGIC, 3, int)
#define MYCALC_IOC_DO_OPERATION		_IOWR(MYCALC_IOC_MAGIC, 4, int)

struct oper {
	int num1;
	int num2;
	char operador;
	int result;

} a;

int main()
{
	int fd;
	int rc;
	char buff[BUFF_SIZE];
	printf("test\n");
	fd = open(DEVICE, O_RDWR);
	if (fd < 0) {
		printf("Error al abrir el dispositivo\n");
		close(fd);
		return fd;
	}
	printf("Open\n");
	/* Aqui se le ponen las operaciones para las dos pruebas */
	int bufff, numero1 = 4, numero2 = -2;
	char operacion = '/';
	a.num1 = 5;
	a.num2 = -7;
	a.operador = '*';

	/* Call ioctl */
	printf("about to call ioctl");
	/* Setting the first number */
	if (ioctl(fd, MYCALC_IOC_SET_NUM1, &numero1) < 0)
		perror("first ioctl");
	/* Setting the second number */
	if (ioctl(fd, MYCALC_IOC_SET_NUM2, &numero2) < 0)
		perror("first ioctl");
	/* Setting the operation */
	if (ioctl(fd, MYCALC_IOC_SET_OPERATION, &operacion) < 0)
		perror("first ioctl");
	/* Getting the result */
	if (ioctl(fd, MYCALC_IOC_GET_RESULT, &bufff) < 0)
		perror("first ioctl");
	/* Doing all the above on one ioctl */
	if (ioctl(fd, MYCALC_IOC_DO_OPERATION, &a) < 0)
		perror("first ioctl");
	printf("Prueba con los 4 ioctl");
	printf("num1 = %i\nnum2 = %i\noperacion = %c\nresult %i\n",
			numero1, numero2, operacion, bufff);

	printf("Prueba con 1 ioctl");
	printf("a.num1 = %i\n", a.num1);
	printf("a.num2 = %i\n", a.num2);
	printf("a.operador %c\n", a.operador);
	printf("resultado %i\n", a.result);

	/* End call ioctl */

	rc = close(fd);
	if (rc < 0) {
		printf("Error al cerrar el dispositivo\n");
		close(fd);
		return rc;
	}
	printf("Close\n");

	return 0;
}

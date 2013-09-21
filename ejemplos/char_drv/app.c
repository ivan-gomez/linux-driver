#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DEVICE "/dev/MY_DEV0"
#define BUFF_SIZE	10

int main()
{
	int fd;
	int rc;
	char buff[BUFF_SIZE];

	fd = open(DEVICE, O_RDWR);
	if (fd < 0) {
		printf("Error al abrir el dispositivo\n");
		return fd;
	}
	printf("Open\n");

	rc = read(fd, buff, BUFF_SIZE);
	if (rc < 0) {
		printf("Error al leer del dispositivo\n");
		return rc;
	}
	printf("Read\n");

	rc = write(fd, buff, BUFF_SIZE);
	if (rc < 0) {
		printf("Error al escribir al dispositivo\n");
		return rc;
	}
	printf("Write\n");

	rc = close(fd);
	if (rc < 0) {
		printf("Error al cerrar el dispositivo\n");
		return rc;
	}
	printf("Close\n");

	return 0;
}

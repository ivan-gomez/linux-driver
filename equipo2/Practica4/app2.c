#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>

#define IOC_MAGIC		0x94
/* Defines our ioctl call. */
#define PARA_IOC_SET_WRITE	_IO(IOC_MAGIC, 0x1)
#define PARA_IOC_SET_READ	_IO(IOC_MAGIC, 0x2)
#define PARA_IOC_SET_STROBE	_IO(IOC_MAGIC, 0x3)
#define PARA_IOC_CLEAR_STROBE	_IO(IOC_MAGIC, 0x4)

int main(int argc, char **argv)
{
	char rbuffer[512];
	int f, ret;
	ssize_t bytes;
	/* Open a file */
	f = open("/dev/para0", O_RDWR, 0666);
	if (f == -1) {
		printf("failed to open paralel port.\n");
		return 1;
	}
	/* Configure as output */
	ret = ioctl(f, PARA_IOC_SET_WRITE);
	if (ret < 0) {
		printf("ioctl failed.\n");
		return -1;
	}
	if (argc == 2) {
		if (strcmp(argv[1], "0") == 0) {
			printf("no led will be turned on\n");
			} else {
				ret = atoi(argv[1]);
				if (ret > 0 && ret <= 255) {
					printf("write = 0x%X\n", ret);
					write(f, &ret, 8);
				} else {
					printf("parameters must be > 0 and <= 255\n");
					return -1;
			}
		}
	} else {
		printf("Insert only 1 argument between 0 and 255");
		return 1;
	}
	printf("Waiting a little to read\n");
	/* Configure as input */
	ret = ioctl(f, PARA_IOC_SET_READ);
	usleep(5000000);
	/* Clear strobe */
	ret = ioctl(f, PARA_IOC_SET_STROBE);
	usleep(100);
	/* Set strobe */
	ret = ioctl(f, PARA_IOC_CLEAR_STROBE);
	/* Read from file */
	bytes = read(f, rbuffer, sizeof(rbuffer));
	printf("You read: 0x%X\n", (0xFF) & rbuffer[0]);
	/* Close file */
	close(f);
}

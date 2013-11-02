#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include "driver/parallel.h"
#include "strobe.h"

int fd;

int main()
{
	int rc, ic;
	fd = open(DEVICE, O_RDWR);
	if (fd < 0) {
		printf("Error opening device\n");
		return fd;
	}
	printf("Open\n");


	ioctl(fd, PARALLEL_STROBE_ON);
	printf("PARALLEL_STROBE_ON\n");
	usleep(10000);
	ioctl(fd, PARALLEL_STROBE_OFF);
	printf("PARALLEL_STROBE_OFF\n");

	rc = close(fd);
	if (rc < 0) {
		printf("Error closing device\n");
		return rc;
	}
	printf("Close\n");

	return 0;
}

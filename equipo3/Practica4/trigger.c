/*
 * Strobe trigger
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include "config.h"

#define IOC_MAGIC			0x94

#define PARA_IOC_SET_STROBE		_IO(IOC_MAGIC, 0x3) /* Defines \
						our ioctl call */
#define PARA_IOC_CLEAR_STROBE		_IO(IOC_MAGIC, 0x4) /* Defines our\
						ioctl call */

int main()
{
	int f, i, ret;

	/* Open a file */
	f = open("/dev/parallel0", O_RDWR, 0666);

	if (f == -1) {
		printf("Failed to open paralel port.\n");
		return 1;
	}

	/* Clear strobe */
	ret = ioctl(f, PARA_IOC_SET_STROBE);
	printf("Strobe set to 1.\n");
	usleep(10000);
	/* Set strobe */
	ret = ioctl(f, PARA_IOC_CLEAR_STROBE);
	printf("Strobe set to 0.\n");

	/* Close file */
	close(f);
	printf("Strobe set successfully.\n");
}

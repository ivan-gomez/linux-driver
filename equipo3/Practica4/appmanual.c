/*
 * Parallel read and write user application
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include "config.h"

#define IOC_MAGIC		0x94

#define PARA_IOC_SET_WRITE	_IO(IOC_MAGIC, 0x1) /* To set data pins as \
							outputs */
#define PARA_IOC_SET_READ	_IO(IOC_MAGIC, 0x2) /* To set data pins as \
							inputs*/
#define PARA_IOC_SET_STROBE	_IO(IOC_MAGIC, 0x3) /* To set strobe to 1 */
#define PARA_IOC_CLEAR_STROBE	_IO(IOC_MAGIC, 0x4) /* To set strobe to 0 */

int main()
{
	char wbuffer[512];
	char rbuffer[512];
	int f, i, ret;
	long result;
	ssize_t bytes;

	/* Open a file */
	f = open("/dev/parallel0", O_RDWR, 0666);

	if (f == -1) {
		printf("Failed to open paralel port.\n");
		return 1;
	}

	/* Configure data pins as outputs */
	ret = ioctl(f, PARA_IOC_SET_WRITE);

	/* Flash leds */
	printf("About to flash leds\n");
	sprintf(wbuffer, "%c", 0x55);
	write(f, wbuffer, 1);
	usleep(1000000);
	sprintf(wbuffer, "%c", 0xAA);
	write(f, wbuffer, 1);
	usleep(1000000);

	printf("About to read data\n");

	/* Wait for enter (or strobe)*/
	printf("Manual wait, enter a string to continue...\n");
	scanf("%s", wbuffer);

	/* Configure as input */
	ret = ioctl(f, PARA_IOC_SET_READ);

	/* Read from file */
	bytes = read(f, rbuffer, sizeof(rbuffer));

	rbuffer[bytes] = 0;

	printf("\nSuccessfully read %d bytes.\n", bytes);
	printf("\nDATA: 0x%X\n", (0xFF) & rbuffer[0]);

	/* Close file */
	close(f);
}

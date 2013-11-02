#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include "driver/parallel.h"
#include "app.h"

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

	/* Write */
	parallel_config_output();
	usleep(300000);
	parallel_write(1);
	usleep(300000);
	parallel_write(4);
	usleep(300000);
	parallel_write(16);
	usleep(300000);
	parallel_write(64);
	usleep(300000);
	blink();
	blink();
	blink();
	blink();

	/* Wait enter */
	printf("Press ENTER to continue...\n");
	while (getchar() != '\n');

	/* Read */
	parallel_config_input();
	usleep(300000);
	parallel_read();

	rc = close(fd);
	if (rc < 0) {
		printf("Error closing device\n");
		return rc;
	}
	printf("Close\n");

	return 0;
}

/*
 * blink()
 *  Blink function to intercalate leds output to parallel
 * Parameters:
 *  -none-
 * Returns:
 *  -none-
 */
void blink()
{
	parallel_write(0x55);
	usleep(300000);
	parallel_write(0xAA);
	usleep(300000);
}


/*
 * parallel_config_output()
 *  Configure parallel registers as output mode
 * Parameters:
 *  -none-
 * Returns:
 *  -none-
 */
void parallel_config_output()
{
	ioctl(fd, PARALLEL_SET_OUTPUT);
	printf("PARALLEL_SET_OUTPUT()\n");
}

/*
 * parallel_config_input()
 *  Configure parallel registers as input mode
 * Parameters:
 *  -none-
 * Returns:
 *  -none-
 */
void parallel_config_input()
{
	ioctl(fd, PARALLEL_SET_INPUT);
	printf("PARALLEL_SET_INPUT()\n");
}

/*
 * parallel_write()
 *  Write value to parallel port
 * Parameters:
 *  uchar value to write
 * Returns:
 *  -none-
 */
void parallel_write(unsigned char value)
{
	write(fd, &value);
	printf("WRITE(%d)\n", value, 1);
}

/*
 * parallel_read()
 *  Read value from parallel port
 * Parameters:
 *  -none-
 * Returns:
 *  uchar value read
 */
char parallel_read()
{
	char value[512];
	value[0] = 0;
	read(fd, &value, sizeof(value));
	printf("READ() = 0x%X\n", (0xFF) & value[0]);
	return (0xFF) & value[0];
}

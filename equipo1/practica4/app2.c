#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>


#define IOC_MAGIC				0x94

#define PARA_IOC_SET_WRITE	_IO(IOC_MAGIC, 0x1)
#define PARA_IOC_SET_READ	_IO(IOC_MAGIC, 0x2)
#define PARA_IOC_SET_STROBE	_IO(IOC_MAGIC, 0x3)
#define PARA_IOC_CLEAR_STROBE	_IO(IOC_MAGIC, 0x4)

int main()
{
	char rbuffer;
	int f, i, ret;
	long result;
	ssize_t bytes;

	/* Open a file */
	f = open("/dev/para0", O_RDWR, 0666);

	if (f == -1) {
		printf("failed to open paralel port.\n");
		return 1;
	}

	ret = 0xAA;

	write(f, &ret, 1);

	printf("Lee el 0xAA que hay en los leds por un segundo\n\n");
	/* Configure as input */

	printf("APP2>Mandame la interrupcion =).\n");

	bytes = read(f, &rbuffer, sizeof(rbuffer));

	printf("\n\nDato Leido: 0x%X\n\n", (0xFF) & rbuffer);

	/* Close file */
	close(f);
	printf("APP2>Closed.\n");

}

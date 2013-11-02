#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>


#define IOC_MAGIC   0x94

#define PARA_IOC_SET_WRITE	_IO(IOC_MAGIC, 0x1)
#define PARA_IOC_SET_READ	_IO(IOC_MAGIC, 0x2)

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
	return -1;
    }

	/* Configure as output */
	ret = ioctl(f, PARA_IOC_SET_WRITE);

	if (ret < 0) {
		printf("ioctl failed.\n");
		return -1;
	}

    /* Write to buffer and file*/
	ret = 0xAA;

	write(f, &ret, 1);

	/* Wait for enter */
	printf("Enter a key to continue..\n");
	(void) getchar();

	/* Configure as input */
	ret = ioctl(f, PARA_IOC_SET_READ);

	/* Read from file */
	bytes = read(f, &rbuffer, sizeof(rbuffer));

	printf("\n\nDato Leido: 0x%X\n\n", (0xFF) & rbuffer);

	/* Close file */
	close(f);

	return 0;
}

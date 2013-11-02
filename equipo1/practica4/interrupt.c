#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>

#define IOC_MAGIC			0x94
#define PARA_IOC_SET_STROBE		_IO(IOC_MAGIC, 0x3)
#define PARA_IOC_CLEAR_STROBE		_IO(IOC_MAGIC, 0x4)

int main()
{
	int ret, f;
	f = open("/dev/para0", O_RDWR, 0666);

	if (f == -1) {
		printf("failed to open paralel port.\n");
		return 1;
	}

	ret = ioctl(f, PARA_IOC_SET_STROBE);
	usleep(10);

	ret = ioctl(f, PARA_IOC_CLEAR_STROBE);
	printf("interrupcion mandada");

	return 0;
}

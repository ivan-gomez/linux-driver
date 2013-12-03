/* App to control the turret from the terminal */

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>

#define DEVICE "/dev/skel0"
#define BUFF_SIZE	8

#define TURRET_MAGIC 'C'
#define TURRET_COMMAND		_IOW(TURRET_MAGIC, 0, int)

/* Turret commands */
#define TURRET_STOP		0x20
#define TURRET_UP		0x01
#define TURRET_DOWN		0x02
#define TURRET_LEFT		0x04
#define TURRET_RIGHT		0x08
#define TURRET_FIRE		0x10

#define DEFAULT_DURATION	500
#define FIRE_DELAY		5000

/* Shows usage of turret */
static void usage(char *name)
{
	fprintf(stderr,
			"\nusage: %s [-udlrsfh]\n\n"
			"  -u      turn up\n"
			"  -d      turn down\n"
			"  -l      turn left\n"
			"  -r      turn right\n"
			"  -s      stop\n"
			"  -f      fire\n"
			"  -h      display this help\n\n"
			"" , name);
	exit(1);
}

int main(int argc, char *argv[])
{
	/* File operation variables */
	char c;
	int fd;
	int rc;
	char buff[BUFF_SIZE];
	int duration;
	int turretCommand; /* The command to execute */

	if (argc < 2)
		usage(argv[0]);

	turretCommand = TURRET_STOP; /* Assume that the turret will stop */

	while ((c = getopt(argc, argv, "lrudfs:")) != -1) {
		switch (c) {
		case 'l':
			turretCommand = TURRET_LEFT;
			break;
		case 'r':
			turretCommand = TURRET_RIGHT;
			break;
		case 'u':
			turretCommand = TURRET_UP;
			break;
		case 'd':
			turretCommand = TURRET_DOWN;
			break;
		case 'f':
			turretCommand = TURRET_FIRE;
			break;
		case 's':
			turretCommand = TURRET_STOP;
			break;
		default:
			usage(argv[0]);
		}
	}

	fd = open(DEVICE, O_RDWR);
	if (fd < 0) {
		printf("Error opening device\n");
		return fd;
	}
	printf("Device opened\n");

	/* Operation via variables */
	printf("About to send command to turret driver\n");
	ioctl(fd, TURRET_COMMAND, &turretCommand);
	printf("Command sent to driver\n");

	/* Fix the time durations */
	duration = DEFAULT_DURATION;
	if (turretCommand & TURRET_FIRE)
		duration = FIRE_DELAY;
	else if (turretCommand == TURRET_UP || turretCommand == TURRET_DOWN)
		duration /= 2;
	usleep(duration * 1000); /* msleep does not exist */

	turretCommand = TURRET_STOP;
	ioctl(fd, TURRET_COMMAND, &turretCommand);
	printf("Stop sent to driver\n");

	rc = close(fd);
	if (rc < 0) {
		printf("Error closing the device\n");
		return rc;
	}
	printf("Device closed\n");

	return 0;
}

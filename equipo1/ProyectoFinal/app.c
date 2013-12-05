#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#define DEVICE "/dev/js0"
#define BUFF_SIZE 50

int x = 200, y = 200, vel = 10;
 
int main()
{
    int fd, rc;
    char buff[BUFF_SIZE];
    char str[70];
    printf("App para mover el mouse con el joystick");
    fd = open(DEVICE, O_RDWR|O_NONBLOCK);
    if (fd < 0) {
            printf("Error al abrir el dispositivo");
	    return fd;
    }
     
       do{
	    rc = read(fd, buff, 4);
		    if(buff[2] & 0x80)
		    {
			//boton 8
			printf("boton 8 presionado\n");
			if(vel < 50)
				vel+=10;
		    }
		    if(buff[2] & 0x40)
		    {
			//boton 7
			printf("boton 7 presionado\n");
			if(vel-10 <= 0)
				vel=10;
			else
				vel-=10;
		    }
            if(buff[2] & 0x20)
		    {
                        //boton 6
			printf("boton 6 presionado\n");
                    }
			if(buff[2] & 0x10)
		    {
                        //boton 5 
			printf("boton 5 presionado\n");
                    }
			if(buff[2] & 0x08)
		    {
                        //boton 4 
			printf("boton 4 presionado\n");
                    }
			if(buff[2] & 0x04)
		    {
                        //boton 3 
	 		printf("boton 3 presionado\n");
			sprintf(str,"xdotool getmouselocation");	
			system(str);
                    }
			if(buff[2] & 0x02)
		    {
                        //boton 2 
			printf("boton 2 presionado\n");
			sprintf(str,"xdotool click 3");	
			system(str);
                    }
			if(buff[2] & 0x01)
		    {
                        //boton 1 
			printf("boton 1 presionado\n");
			sprintf(str,"xdotool click 1");	
			system(str);
                    }
		    if(buff[2] == 0)
		    {
			if(buff[3] == 0)
			{
			    //default
			    printf("nada presionado\n");
			}
			if(buff[3] & 0x01)
			{
			    //boton 9
			    printf("boton 9\n");
			}
			if(buff[3] & 0x02)
			{
			    //boton 10
			    printf("boton 10\n");
			}
	    }
		if(buff[3] & 0x04)
	    {
            	//izq
		printf("izquierda\n");
		if(x - vel < 0)
		    x = 0;
		else
		    x = x-vel;
		sprintf(str,"xdotool mousemove --sync %d %d ", x, y);	
		system(str);
        }
	    if(buff[3] & 0x20)
	    {
		//abajo
		printf("abajo\n");
		if(y + vel > 760)
		    y = 760;
		else
		    y = y+vel;
		sprintf(str,"xdotool mousemove --sync %d %d ", x, y);
		system(str);
            }
            if(buff[3] & 0x08)
	    {
            	//derecha
		printf("derecha\n");
		if(x + vel > 1365)
		    x = 1365;
		else
		    x = x+vel;
		sprintf(str,"xdotool mousemove --sync %d %d ", x, y);
		system(str);
            }
            if(buff[3] & 0x10)
            {
		//arriba
		printf("arriba\n");
		if(y - vel < 0)
		    y = 0;
		else
		    y = y-vel;
		sprintf(str,"xdotool mousemove --sync %d %d ", x, y);
		system(str);
            }
        }while(!(buff[3] & 0x02) );

    close(fd); 
    return 0;
}

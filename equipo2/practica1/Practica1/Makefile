obj-m += practica1.o
practica1-objs := mod_param.o myfuncs.o

PWD := $(shell pwd)

default:
	$(MAKE) -C /lib/modules/`uname -r`/build M=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/`uname -r`/build M=$(PWD) clean
	$(RM) Module.markers modules.order

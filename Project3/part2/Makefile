CC = i586-poky-linux-gcc
ARCH = x86
CROSS_COMPILE = i586-poky-linux-

IOT_HOME = /opt/iot-devkit/1.7.2/sysroots
PATH := $(PATH):$(IOT_HOME)/x86_64-pokysdk-linux/usr/bin/i586-poky-linux
SROOT=$(IOT_HOME)/i586-poky-linux
obj-m = bbang_hrtimer.o bbang_ndelay.o
ccflags-y = -g

all:
	 make ARCH=x86 CROSS_COMPILE=i586-poky-linux- -C $(SROOT)/usr/src/kernel M=$(shell pwd) modules -Wall
	i586-poky-linux-gcc -o user.o user.c --sysroot=$(SROOT) -Wall
	#make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
	#gcc -o test2.o test2.c

clean:
	make ARCH=x86 CROSS_COMPILE=i586-poky-linux- -C $(SROOT)/usr/src/kernel M=$(shell pwd) clean
	rm -f *.ko
	rm -f *.o

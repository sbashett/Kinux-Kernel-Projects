CSE 530 : ASSIGNMENT 3
TEAM 21
SA KRISHNA BASHETTY : 1213376903
SAAD KATRAWALA :  1213260514

#####################################################################################
INTRODUCTION:
In part 1 of this a linux driver code is written for the led ring implementation using spi protocol.

SPI is a synchronus protocol the ws2812 device is an asynchronus protocol. So to transfer data to ws2812 device using SPI we implemented a 8 bit encoding on the data bytes before sending it to ws2812 device.
The frequency of spi transfer is 6.4 MHZ and the frequency at which device operates is 800HZ. 
An ioctl command is implemented to set the specifications of spi device and reset the ws2812 device whenever required from the user space.

There are three files (in folder part1) :
spi_ring_device.c : The file for initializing the spi device in kernel
spi_ring_driver.c : The file for registering the spi driver and probing the driver once the device is found. It even includes the character device file interface and necessary functions for implementing the spi transfer.
user : This is a user space code which can be modified and used to transmit data to ws2812 led ring. For testing the drivers we initialized a data buffer for displaying different colours.



The part II of approaches to solve the problem ina crude fashion using a technique called "bit banging". In this approach one uses a gpio pin to generate the required waveform with the help of high precision timing sources. Here, we have used hrtimer and ndelay for that purpose. To voercome the performance bottleneck we are using iowrite32() for writingdata to the GPIO pins. 

There are 3 files (in folder part2):

bbang_hrtimer.c : The driver module for implementing bit banging using hrtimer 
bbang_ndelay.c : The driver module for implementng bit banging using ndelay.
user : The user program for testing the driver.

 

#######################################################################################
STEPS TO RUN PROGRAM 1:

1) Use the Makefile to generate the .ko files using command "make" in the terminal
2) Insert the modules spi_ring_device.ko and spi_ring_driver.ko using the command 
	"insmod spi_ring_device.ko"
	"insmod spi_ring_driver.ko"
3) Run the user program by using command 
 	"./user" 
in terminal.

Now you can observe all the leds of ring glowig with a pattern and the pattern rotates for 20 times and then all the leds are turned off.

#############################################################################################

STEPS TO EXECUTE PROGRAM 2 :

1) Use the Makefile to generate the .ko files using command "make" in the terminal

2) Insert the module bbang_hrtimer.ko from the required directory : 
 
	"insmod bbang_hrtimer.ko"

3) Run the user program by using command 
	"./user.o"

4) Remove the other module using 

	"rmmod bbang_hrtimer.ko"

5) Insert the module bbang_ndelay.ko from the required directory : 

	"insmod bbang_ndelay.ko"

6) Run the user program by using command 

 	"./user.o" 

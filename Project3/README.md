## INTRODUCTION:
### PART I
In part 1 of this a linux driver code is written for the led ring implementation using spi protocol.

SPI is a synchronous protocol the ws2812 device is an asynchronous protocol. So to transfer data to ws2812 device using SPI we implemented a 8 bit encoding on the data bytes before sending it to ws2812 device.
The frequency of spi transfer is 6.4 MHZ and the frequency at which device operates is 800HZ.
An ioctl command is implemented to set the specifications of spi device and reset the ws2812 device whenever required from the user space.

There are three files (in folder part1) :
* spi_ring_device.c : The file for initializing the spi device in kernel
* spi_ring_driver.c : The file for registering the spi driver and probing the driver once the device is found. It even includes the character device file interface and necessary functions for implementing the spi transfer.
* user : This is a user space code which can be modified and used to transmit data to ws2812 led ring. For testing the drivers we initialized a data buffer for displaying different colors.

### PART II

The part 2 tries to solve the problem in a crude fashion using a technique called "bit banging". In this approach one uses a gpio pin to generate the required waveform with the help of high precision timing sources. Here, we have used hrtimer and ndelay for that purpose. To overcome the performance bottleneck we are using iowrite32() for writing data to the GPIO pins.

There are 3 files (in folder part2):

* bbang_hrtimer.c : The driver module for implementing bit banging using hrtimer
* bbang_ndelay.c : The driver module for implementng bit banging using ndelay.
* user : The user program for testing the driver.

### STEPS TO RUN PROGRAM 1:

* Use the Makefile to generate the .ko files using command "make" in the terminal
* Insert the modules spi_ring_device.ko and spi_ring_driver.ko using the commands
```
	insmod spi_ring_device.ko
	insmod spi_ring_driver.ko
  ```
* Run the user program by using command ```./user``` in terminal.
Now you can observe all the LEDs of ring glowing with a pattern and the pattern rotates for 20 times and then all the LEDs are turned off.

### STEPS TO RUN PROGRAM 2 :

* Use the Makefile to generate the .ko files using command ```make``` in the terminal
* Insert the module bbang_hrtimer.ko from the required directory
```
	insmod bbang_hrtimer.ko
  ```
* Run the user program by using command ```./user.o```
* Remove the other module using
```
	rmmod bbang_hrtimer.ko
  ```

* Insert the module bbang_ndelay.ko from the required directory :
```
	insmod bbang_ndelay.ko
  ```

* Run the user program by using command ``./user.o``

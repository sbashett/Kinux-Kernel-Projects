Make the following changes to the Makefile :

Edit the IOT_HOME to set the location of the kernel source for the build
Then run the "make" command from the PWD (where the driver source code is present)


### LOADING THE MODULE

Run the following commands from the shell :
```
insmod platform_device.ko
insmod platform_driver.ko
( in any order )

chmod 777 /dev/HSC_N
chmod 777 /sys/class/HCSR/HSC_N
```


### SCRIPT FILES

* script.sh is used to interact with the sysfs. It writes to the default pins mentioned at the end of the document.
* interface.sh will ask you two choose from two commands : show and store
	* store is used to input values to the particular sensor (e.g. ECHO,TRIGGER,etc.)
	* show is used to display all the attributes of a particular sensor(s)

### CODE DESCRIPTION

**Device Structures:**

There per device structure HSC_struct which is created when we open a device.The structure HSC_struct has the members interval,samples,trigger which are used in according with their names.
The timeval ini,end,tdiff are used to calcualte the time difference for calculation of the distance. HR_timer is being used to generate interrupts for collecting the samples.The distance buffer is used to store the samples for averaging later.The flag1 and flag2 are being polled to wait for the timer to expire.


All the basic driver functions:

***sensor_open:***

The sensor_open initializes the device specific structure pointer "struct1" and allocates some memory dynamically.

We create a reference to this object for other entry points by storing it in the "file->private_data" structure;

***sensor_ioctl:***

The ioctl has two commands namely :

* CONFIG_PINS:
The config pins command stores the echo and trigger pins into the respective per device structure elements.

* SET_PARAMS:
The set params command stores the samples and intervals into the respective per device structure elements.

***sensor_write:***

The write function takes input from the ioctl commands and if the gpio config was done successfully, the write command will trigger the "trigger" pin every "delta(interval)" ms for samples+2 times. The distance is calculated using the formula (170 * timediff)/10^6; where timediff is in micrseconds and the result is in cms.

***sensor_read:***

This function sorts the array base on the value using the bubblesort and then removes the outlier, calulcates the average of the rest and copies it to the user space using copy_to_user.


### USER PROGRAM

Run the ```make``` command in the shell from the directory containing "user.c" to generate compiled object:


Run the user program using ```./user.o``` from the directory where all the files are present.

### USER PROGRAM TESTING

The user code will use the cdev interface to communicate with the device :

DEFAULT PIN CONFIG FOR GALILEO HARDWARE:

Sensor 1 :

	Echo    : 8
	Trigger : 3
	Samples : 5

Sensor 2 :

	Echo    : 10
	Trigger : 2
	Samples : 5

You can change the the pin numbers in user.c file.

### CROSS-COMPILING THE MODULE

Make the following changes to the Makefile :

Edit the IOT_HOME to set the location of the kernel source for the build

Then run the "make" command from the PWD (where the driver source code is present)


### LOADING THE MODULE

Run the following commands from the shell :

TO install module
```
insmod ultrasonic.ko
```
and then give permissions to the driver
```
chmod 777 /dev/HSC_N
```

### CODE DESCRIPTION

**Structures:**

The per device structure HSC_struct which is created when we open a device.The structure HSC_struct has the members interval,samples,trigger which are used in according with their names.
The timeval ini,end,tdiff are used to calcualte the time difference for calculation of the distance. HR_timer is being used to generate interrupts for collecting the samples.The distance buffer is used to store the samples for averaging later.The flag1 and flag2 are being polled to wait for the timer to expire.

**All the basic driver functions are working properly :**

***sensor_open:***
The rbt_driver_open intializes the device specific structure pointer "rbt_devp" and allocates some memory dynamically.
We create a reference to this object for other entry points by storing it in the "file->private_data" structure.

***sensor_ioctl:***
The ioctl has two commands namely

* CONFIG_PINS:
  The config pins command stores the echo and trigger pins into the respective per device structure elements.

* SET_PARAMS:
  The set params command stores the samples and intervals into the respective per device structure elements.

***sensor_write:***
The write function takes input from the ioctl commands and if the gpio config was done successfully, the write command will trigger the "trigger" pin every "delta(interval)" ms for samples+2 times. The distance is calculated using the formula :
(170 * timediff)/10^4; where timediff is in micrseconds and the result is in cms.

***sensor_read:***
This function sorts the array base on the value using the bubblesort and then removes the outlier, calulcates the average of the rest and copies it to the user space using copy_to_user.


### USER PROGRAM

Use the ```make``` command in the shell from the directory containing the user program "user.c" to generate executable.

Run the user program using ```./user``` from the directory where all the files are present

The user program has load() and print_all() functions which loads the objects into the rb-tree and prints all the objects.

### USER PROGRAM TESTING

The user code will ask for trigger and echo inputs, the number of samples and intervals. The driver will print the the individual samples on the logs and the user code prints out the averaged out distance and the timestamps.

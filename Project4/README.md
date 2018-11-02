## INTRODUCTION

This is a code which adds a custom system call to the linux source code and you can use the system call in any of your user code directly.
The necessary changes to the source code are provided in the form of a patch. The steps to apply the patch are mentioned in below sections.
There is a file "barrier.patch" which needs to be used for patching and then build the kernel.
There is a "user" program which can be used to test the syscall after building and booting into the new kernel

There are three functions related to barrier synchronization:
* barrier_init
* barrier_wait
* barrier_destroy

***barrier_init:***
This syscall will create a new barrier to synchronize. The parameters to this function are:

* *unsigned int* count : The number of threads to synchronize using this barrier
* *signed int* timeout : The value of timeout in ns. If less than or equal to zero no timeout is defined.

**NOTE:**
WE ARE RETURNING THE barrier-id when the system call is called and NOT STORING IT IN THE BARRIER ID POINTER. (and thus only 2 variables)

***barrier_wait:*** This syscall is called by a thread which needs to be synchronised. It has one parameter

* *unsigned int* barrier_id which identifies the barrier which it belongs to.
It will return a zero if synchronisation was properly done or -ETIME if a timeout situation arised in the barrier

***barrier_destroy :*** This syscall destroys the barrier whose id is mentioned in its arguments

* *unsigned int* barrier_id: This call returns -EBUSY if the barrier to be destroyed is busy.

## STEPS FOR PATCHING AND BUILDING KERNEL:

To generate a patch run the diff command with options -rNu between the update linux source directory and the original source. For Example
 ```
 diff -rNu source_orig/ new_source/ > barrier.patch
 ```

To apply the patch copy the original linux source(with the name kernel) and the ```barrier.patch``` into a new directory and run the patch command
```
	patch -p0 < barrier.patch
	```

After the patch has been applied build the kernel using the following commands to build the new kernel (from the new kernel source directory) :

* Make appropriate changes to path_to_sdk
```
 export PATH=path_to_sdk/sysroots/x86_64-pokysdk-linux/usr/bin/i586-poky-linux:$PATH
 ```

* Cross compile the kernel using

```
ARCH=x86 LOCALVERSION=CROSS_COMPILE=i586-poky-linux- make -j4
```

* Build and extract the kernel modules from the build to a target directory (e.g ../galileo-install) using the command

```
ARCH=x86 LOCALVERSION= INSTALL_MOD_PATH=../galileo-install CROSS_COMPILE=i586-poky-linux- make modules_install
```

* Extract the kernel image (bzImage) from the build to a target directory (e.g ../galileo-install)

```
cp arch/x86/boot/bzImage ../galileo-install/
```

* Install the new kernel and modules from the target directory (e.g ../galileo-install) to your micro
SD card
 Replace the bzImage found in the first partition (ESP) of your micro SD card with the one from your target directory (backup the bzImage on the micro SD card e.g. rename it to bzImage.old)

* Now copy the kernel modules from the target directory to the ```/lib/modules/``` directory found in the second partition of your micro SD card.

AFTER BOOTING INTO BUILT KERNEL:

copy the ```user``` object file and run on the board. There are some default timeout, thread count and no:of synchronizations values in the program. If required they can be changed by changing the macro values at the beginning of ```user.c``` file.
After changing the values the program can be compiled to generate the "user" object file which runs on the board. It can be done by using the Makefile. (use command make in terminal).

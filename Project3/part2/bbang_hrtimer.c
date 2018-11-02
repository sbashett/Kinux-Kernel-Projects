#include <linux/fs.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/time.h>
#include<linux/completion.h>
#include<linux/pci.h>
#include "pinmux.h"


#define DEVICE_NAME "hrt_1"
#define MS_TO_NS(x) (x * 1E6L)


typedef struct{ 
struct hrtimer hrt;
struct cdev cdev;
unsigned int led_count;
unsigned int msg[16];
volatile int c;
bool done;
bool bit;
char name[20];
struct completion biw;
ktime_t  ONE_ON,ONE_OFF,ZERO_ON,ZERO_OFF;
void __iomem *reg_data;
u32 val_data;
}devp;

static dev_t hrt_dev_number;      /* Allotted device number */
struct class *hrt_dev_class;    /* Tie with the device model */
static struct device *hrt_dev_device;
devp *hrt_devp;


/*HR Timer Callback function
@timer : HR Timer structure pointer
*/
static enum hrtimer_restart my_hrtimer_callback( struct hrtimer *timer )
{
	ktime_t on_interval,off_interval;
	//struct timeval t_now;

    	hrt_devp=container_of(timer, devp, hrt);
	    
	if(hrt_devp->c--)
	{
		
				if(hrt_devp->bit)
				{ 
				  on_interval=hrt_devp->ONE_ON;
				  off_interval=hrt_devp->ONE_OFF;
				}
				else
				{
				  on_interval=hrt_devp->ZERO_ON;
				  off_interval=hrt_devp->ZERO_OFF;
				}
			
			
				if(hrt_devp->done == 0)
				{
				iowrite32(149, hrt_devp->reg_data);
				
								hrtimer_forward_now(timer,on_interval);
					hrt_devp->done=1;
				}	
				else
				{
				iowrite32(21, hrt_devp->reg_data);		
				hrtimer_forward_now(timer,off_interval);
				hrt_devp->msg[hrt_devp->led_count]=(hrt_devp->msg[hrt_devp->led_count] << 1);
				hrt_devp->bit=hrt_devp->msg[hrt_devp->led_count]&0x80;
				hrt_devp->done=0;
				
				if ( (hrt_devp->c == 0) &&( hrt_devp->led_count< 16) )
				{	
					hrt_devp->led_count++;
					hrt_devp->c=48;
				}

				}

		return HRTIMER_RESTART;
}
    else
		{
		complete(&hrt_devp->biw);
		return HRTIMER_NORESTART;
		}
    }
 

/* Function for intializing the HR Timer : Mode, Clock Base 
@timer : The HR timer structure pointer
*/ 
static void timer_initialize(struct hrtimer *timer)
{
  printk("HR Timer initializiing\n");

  hrtimer_init( timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
   
  timer->function = &my_hrtimer_callback;
}

/* Function for starting the timer with appropriate delay 
@delay_in_ns : The expiry time in nanoseconds 
@timer : HR Timer structure pointer
*/
static int timer_start( unsigned long delay_in_ns, struct hrtimer* timer )
{
   ktime_t ktime;
   
   ktime = ktime_set( 0, delay_in_ns);
 
   hrtimer_start(timer,ktime,HRTIMER_MODE_REL);
 
   return 0;
}
 

static int hrt_open(struct inode *inode, struct file *file)
{
	resource_size_t start=0,len=0;
	void __iomem *reg_base;
	struct pci_dev *pdev;

	/* Get the per-device structure that contains this cdev */
	hrt_devp = container_of(inode->i_cdev, devp, cdev);
	gpio_config(12,OUT);

	timer_initialize(&hrt_devp->hrt);
	if(hrtimer_is_queued(&hrt_devp->hrt))
	printk(KERN_DEBUG"The Timer has been enqueued");

	// Fetching the pointer to pci device structure 
	pdev = pci_get_device(0x8086, 0x0934, NULL);

	//Fetching the pci Base Address Register info
	start = pci_resource_start(pdev, 1);

	len = pci_resource_len(pdev, 1);

	reg_base = ioremap_nocache(start, len);

	// Port A data register address
	hrt_devp->reg_data = reg_base + 0x00;

	hrt_devp->val_data = ioread32(hrt_devp->reg_data);

	file->private_data = hrt_devp;
	printk(KERN_DEBUG "\n%s is opening \n", hrt_devp->name);
    return 0;
}

static int hrt_close(struct inode *inode, struct file *file)
{

    /* Get the per-device structure that contains this cdev */
    hrt_devp = container_of(inode->i_cdev, devp, cdev);
	hrt_devp->c=0;
	hrtimer_cancel(&hrt_devp->hrt);
	iounmap(hrt_devp->reg_data);
	gpio_deregister_out(12);
	pr_info("Sleepy time\n");
	return 0;
}

static ssize_t hrt_write(struct file *file, const char __user *buf,
		       size_t len, loff_t *ppos)
{
	int g,r,b,i;
	hrt_devp = file->private_data;
	init_completion(&hrt_devp->biw);

	// Writing the GRB pixel data;
	g=255;
	r=255;
	b=255;

	// Setting the ON and OFF interval for writing "1" and "0"
	hrt_devp->ONE_ON=ktime_set(0,650);
	hrt_devp->ONE_OFF=ktime_set(0,600);
	hrt_devp->ZERO_ON=ktime_set(0,500);
	hrt_devp->ZERO_OFF=ktime_set(0,850);

	// Initializing the counts 
	hrt_devp->led_count=0;
	hrt_devp->c=48;

	for(i=0;i<16;i++)
	{
		hrt_devp->msg[i]= ( g << 24 | r << 16 | b << 8 );
	}

	// Starting the timer with a dummy time
    timer_start(1,&hrt_devp->hrt);

	// Waiting for completion of timer function
	wait_for_completion_interruptible(&hrt_devp->biw);
	return 0;
  
}


static struct file_operations hrt_fops = {
.owner			= THIS_MODULE,
.open			= hrt_open,
.release		= hrt_close,
.write			= hrt_write,
    
};


int __init hrt_driver_init(void)
{
        int ret;

        /* Request dynamic allocation of a device major number */
        if (alloc_chrdev_region(&hrt_dev_number, 0, 1, DEVICE_NAME) < 0) {
                        printk(KERN_DEBUG "Can't register device\n"); return -1;
        }

        /* Populate sysfs entries */
        hrt_dev_class = class_create(THIS_MODULE, DEVICE_NAME);

        /* Allocate memory for the per-device structure */
        hrt_devp = kmalloc(sizeof(devp), GFP_KERNEL);

        if (!hrt_devp) {
                printk(KERN_DEBUG "Bad Kmalloc\n"); return -ENOMEM;
        }


        /* Request I/O region */
        sprintf(hrt_devp->name, DEVICE_NAME);

        /* Connect the file operations with the cdev */
        cdev_init(&hrt_devp->cdev, &hrt_fops);

  		/* Connect the major/minor number to the cdev */
        ret = cdev_add(&hrt_devp->cdev, (hrt_dev_number), 1);

        if (ret) {
                printk(KERN_DEBUG "Bad cdev\n");
                return ret;
        }

        /* Send uevents to udev, so it'll create /dev nodes */
        hrt_dev_device = device_create(hrt_dev_class, NULL, MKDEV(MAJOR(hrt_dev_number), 0), NULL, DEVICE_NAME);

        printk(KERN_DEBUG "HR Timer driver initialized.\n");
        return 0;
}
/* Driver Exit */
void __exit hrt_driver_exit(void)
{


        /* Release the major number */
        unregister_chrdev_region((hrt_dev_number), 1);

        /* Destroy device */
        device_destroy(hrt_dev_class, MKDEV(MAJOR(hrt_dev_number), 0));
        cdev_del(&hrt_devp->cdev);
        kfree(hrt_devp);

        /* Destroy driver_class */
        class_destroy(hrt_dev_class);

        printk(KERN_DEBUG "HR Timer driver removed.\n");
}

module_init(hrt_driver_init);
module_exit(hrt_driver_exit);
MODULE_LICENSE("GPL");

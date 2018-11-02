#include <linux/fs.h>
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
#include<linux/pci.h>
#include<linux/delay.h>


#define DEVICE_NAME "hrt_1"
#define US_TO_NS(x) (x * 1E3L)

typedef struct{ 
struct cdev cdev;
unsigned int msg[16];
char name[20];
}devp;

static dev_t hrt_dev_number;      /* Allotted device number */
struct class *hrt_dev_class;    /* Tie with the device model */
static struct device *hrt_dev_device;
devp *hrt_devp;

static int hrt_open(struct inode *inode, struct file *file)
{

    /* Get the per-device structure that contains this cdev */
    hrt_devp = container_of(inode->i_cdev, devp, cdev);
    file->private_data = hrt_devp;
    printk(KERN_DEBUG "\n%s is opening \n", hrt_devp->name);
    return 0;
}

static int hrt_close(struct inode *inode, struct file *file)
{

    /* Get the per-device structure that contains this cdev */
    hrt_devp = container_of(inode->i_cdev, devp, cdev);
    pr_info("Sleepy time\n");
    return 0;
}

static ssize_t hrt_write(struct file *file, const char __user *buf,
		       size_t length, loff_t *ppos)
{
	resource_size_t start=0,len=0;
	void __iomem *reg_base,*reg_data;
 	struct pci_dev *pdev;
	uint32_t val_data;
	int r,g,b,n,i,led_count=0;
	bool bit;

	hrt_devp = file->private_data;

	// Writing the GRB pixel data;
	r=255;
	g=255;
	b=255;
	for(i=0;i<16;i++)
	hrt_devp->msg[i]=(g << 24 | r << 16 | b << 8);

	bit=hrt_devp->msg[0]&0x80;
	n=24;

        // Fetching the pointer to pci device structure
	pdev = pci_get_device(0x8086, 0x934, NULL);

	//Fetching the pci Base Address Register info
        start = pci_resource_start(pdev, 1);

        len = pci_resource_len(pdev, 1);

        reg_base = ioremap_nocache(start, len);

         // Port A data register address
         reg_data = reg_base + 0x00;
    
         val_data = ioread32(reg_data);

	for(i=0;i<n;i++)
	{
		if (bit)
		{
			iowrite32(149, reg_data);
			ndelay(700);
			 iowrite32(21, reg_data);
			ndelay(600);
		}
		else
		{
			iowrite32(149, reg_data);
			ndelay(400);
			iowrite32(21, reg_data);
			ndelay(800);
				
		}

		hrt_devp->msg[led_count]=(hrt_devp->msg[led_count] << 1);
		bit=hrt_devp->msg[led_count]&0x80;
		if(i==23)
	{		if(led_count<16)
			{led_count++;
			i=0;
			}
			else
			break;
	}

	}
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

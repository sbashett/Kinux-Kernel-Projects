/*
 * A sample program to show the binding of platform driver and device.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/moduleparam.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/math64.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <asm/uaccess.h>

#include "platform_device.h"
#include "myioctl.h"
#include "pinmux.h"

#define MS_TO_NS(x) (x * 1E6L)


#define LOW 0
#define HIGH 1
#define CLASS_NAME "HCSR"

#define DRIVER_NAME		"platform_driver_0"

static const struct platform_device_id P_id_table[] = {
         { "ultra_1", 0 },
         { "ultra_2", 0 },
         { },
};

static bool class_flag;
static struct class *sys_class;
static dev_t devt;

 static struct HSC_struct {
 struct cdev cdev;
 bool triggerop;
 bool flag1,flag2,busy;
 bool enable;
 int sample_num;
 int irqnum;
 int interval;
 int samples;
 int trigger;
 int echo;
 int minor_num;
 struct device *sys_device;
 unsigned long trig;
 unsigned long tdiff;
 struct timeval ini;
 struct timeval end;
 unsigned long distance[20];
 unsigned long timestamp[20];
 struct hrtimer hr_timer;
 spinlock_t my_lock;
}*struct1[2] ;


static int timer_start(int delay,struct HSC_struct *structp);
static void timer_initialize(struct HSC_struct *structp);
void write_common(struct HSC_struct *structp);
int read_common(struct HSC_struct *structp);
struct HSC_struct * get_structp(struct device *dev);


static irqreturn_t sensor_handler(int irq, void *dev_id, struct pt_regs *regs);

static int sensor_open(struct inode *inode, struct file *file)
{
   printk(KERN_INFO "opened device with major %d and minor %d\n",imajor(inode),iminor(inode));
      
    if(iminor(inode) == 5)
      file->private_data = struct1[0];
    else
      file->private_data = struct1[1];

   return 0;

}

static long sensor_ioctl(struct file* file,unsigned int cmd,unsigned long arg)
{
 
 struct HSC_struct *structp = file->private_data;
 int ret;
  pins p;
  conf c;

      if(cmd == CONFIG_PINS)
  {
          if (copy_from_user( &p, (pins *)arg,sizeof(pins)))
          {
              return -EACCES;
          }

          structp->trigger = p.trigger;
          structp->echo = p.echo;

          gpio_config(structp->trigger , OUT);
          gpio_config(structp->echo , IN);

          structp->irqnum = gpio_to_irq(pin_array[structp->echo][0]);
          printk("interrupt number = %d\n",structp->irqnum);

      ret = request_irq(structp->irqnum,(irq_handler_t)sensor_handler, IRQF_TRIGGER_FALLING,
    "sensor_handler", (void *)structp->minor_num);
   
    printk("interrupt request info = %d\n",ret);

      printk("Trigger is %d and Echo is %d\n",p.trigger,p.echo);
  }
      else if (cmd == SET_PARAMS)
  {
          if (copy_from_user( &c, (conf *)arg,sizeof(conf)))
          {
              return -EACCES;
          }
          structp->samples = c.samples+2;
          structp->interval = c.interval;
          printk("Number of samples is %d and interval is %d\n",c.samples,c.interval);
          }
      else
    {
      printk("Invalid argument\n");
          return -ENOTTY;
  }

  return 0;
}

static ssize_t sensor_write(struct file *file, const char *data,size_t len, loff_t *ppos)
  {
   
    write_common((struct HSC_struct *)file->private_data);
   
    return 0;

   }


  void write_common(struct HSC_struct *structp)
   {

   structp->flag1 = structp->flag2 =1;
   structp->triggerop = 0;
   structp->sample_num = 0;

   timer_initialize(structp);
   timer_start(60,structp);
  
   while(structp->flag1)
   {
    msleep_interruptible(1);
    if(structp->flag2)
    {
        structp->triggerop = HIGH;    
        gpio_set_value_cansleep(pin_array[structp->trigger][0],structp->triggerop);
        do_gettimeofday(&structp->ini);
        structp->trig = (unsigned long)structp->ini.tv_usec;
        udelay(10);
        structp->triggerop = LOW;    
        gpio_set_value_cansleep(pin_array[structp->trigger][0],structp->triggerop);

        structp->flag2 = 0;
        continue;
    }
    else
        continue;
   }

   }


ssize_t sensor_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
	int sum;
	sum = read_common((struct HSC_struct *)file->private_data);
	printk ("value of distance in read is %d",sum);
	return 0;
}


int read_common(struct HSC_struct *structp)
{

int sum=0;
int i;

for(i=1;i<(structp->samples - 1);i++)
  sum=(sum+structp->distance[i]);
  do_div(sum,(structp->samples-2));
return sum;

}

static int sensor_close(struct inode *inode, struct file *file)
{
  int ret;
  struct HSC_struct *structp = file->private_data;
  free_irq (structp->irqnum,(void *)structp->minor_num);
 
  ret = hrtimer_cancel( &(structp->hr_timer));
  if (ret)

  gpio_deregister_out(structp->trigger);//defined in pinmux.h
  gpio_deregister_in(structp->echo);
 
  return 0;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = sensor_open,
	.release = sensor_close,
	.write = sensor_write,
	.unlocked_ioctl = sensor_ioctl
	};

static irqreturn_t sensor_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    int minor;
    unsigned long flags;
    struct HSC_struct *structp;

    minor = (int)dev_id;

    if(minor == 5)
      structp = struct1[0];
    else
      structp = struct1[1];

    spin_lock_irqsave(&(structp->my_lock), flags);
    do_gettimeofday(&structp->end);

    structp->timestamp[structp->sample_num-1] = (jiffies * 1000)/HZ;
    structp->tdiff = ((unsigned long)structp->end.tv_usec) - structp->trig;

    structp->tdiff = 170 * structp->tdiff;
    structp->distance[structp->sample_num-1] = (structp->tdiff)/10000 - 12;
    printk("distance %d = %lu \n", (structp->sample_num), structp->distance[structp->sample_num-1]);
    
   spin_unlock_irqrestore(&(structp->my_lock), flags);
   
   return IRQ_HANDLED;
   
}

static enum hrtimer_restart my_hrtimer_callback( struct hrtimer *timer)
{    
   
   struct HSC_struct *structp = container_of(timer, struct HSC_struct, hr_timer);
  
  ktime_t tim;
   
  if((structp->sample_num) < structp->samples)
  {
    tim = ktime_set(0,MS_TO_NS(60));
    structp->sample_num++;
    structp->flag2 = 1;
    hrtimer_forward_now(timer,tim);
    return HRTIMER_RESTART;
  }
  else
  {   
    structp->flag1 = 0;
      return HRTIMER_NORESTART;
  }
}

static void timer_initialize(struct HSC_struct *structp)
{

 hrtimer_init( &(structp->hr_timer), CLOCK_MONOTONIC, HRTIMER_MODE_REL );
 
 structp->hr_timer.function = &my_hrtimer_callback;
}

static int timer_start(int delay,struct HSC_struct *structp)
{
  ktime_t ktime;
   
  ktime = ktime_set( 0, MS_TO_NS(delay));

  hrtimer_start(&(structp->hr_timer),ktime,HRTIMER_MODE_REL);
  return 0;
}


/***************************defining atributes and their definitions*********/

static ssize_t trigger_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{       
        struct HSC_struct *structp;
        structp = get_structp(dev);

        return sprintf(buf, "IO%d\n", structp->trigger);
}


static ssize_t trigger_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf,
                                  size_t count)
{ 
        struct HSC_struct *structp;
        int tmp;

        structp = get_structp(dev);
        printk("entered trigger store:)\n");
        printk("value of trigger in store:%d\n", structp->trigger);
        sscanf(buf, "%d", &tmp);
        if (tmp > 13 || tmp < 1){
                printk(KERN_ERR "Invalid trigger entry\n");
                return -EINVAL;
            }

        else {

        	if((structp->trigger >= 0)&&(structp->trigger <=19))
        		gpio_deregister_out(structp->trigger); //defined in pinmux.h
        	
  	      		structp->trigger = tmp; 
        		gpio_config(structp->trigger,OUT); //defined in pinmux.h

        }
                        
        return PAGE_SIZE;
}

static ssize_t echo_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{
        struct HSC_struct *structp;
        structp = get_structp(dev);

        return sprintf(buf, "IO%d\n", structp->echo);
}


static ssize_t echo_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf,
                                  size_t count)
{
        struct HSC_struct *structp;
        int tmp,ret;

        structp = get_structp(dev);        
        sscanf(buf, "%d", &tmp);
        if (tmp > 13 || tmp < 1){
                printk(KERN_ERR "Invalid echo entry\n");
                return -EINVAL;
            }

        else {

        	if((structp->echo >= 0)&&(structp->echo <= 13)){
        		gpio_deregister_in(structp->echo); //defined in pinmux.h
        		free_irq(structp->irqnum,(void *)structp->minor_num);
        	}
        	
  	      	structp->echo = tmp; 
        	gpio_config(structp->echo,IN); //defined in pinmux.h
        	
        	structp->irqnum = gpio_to_irq(pin_array[structp->echo][0]);
          	printk("interrupt number = %d",structp->irqnum);

      		ret = request_irq(structp->irqnum,(irq_handler_t)sensor_handler, IRQF_TRIGGER_FALLING,
    			"sensor_handler", (void *)structp->minor_num);
   
    		printk("interrupt request info = %d",ret);

        }    
                        
        return PAGE_SIZE;
}

static ssize_t samples_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{
        struct HSC_struct *structp;
        structp = get_structp(dev);

        return sprintf(buf, "number_samples:%d\n", structp->samples);
}


static ssize_t samples_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf,
                                  size_t count)
{
        struct HSC_struct *structp;
        int tmp;

        structp = get_structp(dev);               
        sscanf(buf, "%d", &tmp);
        if (tmp < 1){
                printk(KERN_ERR "Invalid number_samples entry\n");
                return -EINVAL;
            }

        else {

        	structp->samples = tmp;
        }
                        
        return PAGE_SIZE;
}


static ssize_t speriod_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{
        struct HSC_struct *structp;

        structp = get_structp(dev);

        return sprintf(buf, "sample period:%d\n", structp->interval);
}


static ssize_t speriod_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf,
                                  size_t count)
{
        struct HSC_struct *structp;
        int tmp;

        structp = get_structp(dev);                
        sscanf(buf, "%d", &tmp);
        if (tmp < 50){
                printk(KERN_ERR "Invalid sample period. Enter any value greater than 50\n");
                return -EINVAL;
            }

        else {

        	structp->interval = tmp;
        }
                        
        return PAGE_SIZE;
}


static ssize_t enable_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf,
                                  size_t count)
{
        struct HSC_struct *structp;
        int tmp;

        structp = get_structp(dev); 
        
        sscanf(buf, "%d", &tmp);

        if (tmp == 0){
			structp->enable = 0;               
            }

        else if(tmp ==1){

        	structp->enable = 1;
        	write_common(structp);
        }

        else {
 				printk(KERN_ERR "Invalid enable value. enter either 1 or 0\n");
                return -EINVAL;
        }
                        
        return PAGE_SIZE;
}

static ssize_t distance_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{	
  struct HSC_struct *structp;
  int sum;

  structp = get_structp(dev);  
	sum = structp->distance[2];
        return sprintf(buf, "distance:%d\n", sum);
}



static DEVICE_ATTR(TRIGGER, S_IRUSR | S_IWUSR, trigger_show, trigger_store);
static DEVICE_ATTR(ECHO, S_IRUSR | S_IWUSR, echo_show, echo_store);
static DEVICE_ATTR(NUMBER_SAMPLES, S_IRUSR | S_IWUSR, samples_show, samples_store);
static DEVICE_ATTR(SAMPLE_PERIOD, S_IRUSR | S_IWUSR, speriod_show, speriod_store);
static DEVICE_ATTR(ENABLE, S_IWUSR, NULL, enable_store);
static DEVICE_ATTR(DISTANCE, S_IRUSR , distance_show, NULL);


struct HSC_struct *get_structp(struct device *dev)
{
  if(MINOR(dev->devt) == 5)
    return struct1[0];
  else
    return struct1[1];
  
}
     


/*************************************************************************/

static int P_driver_probe(struct platform_device *dev_found)
{
	struct P_chip *pchip;
	int comp1,comp2,rval,ret;
  struct HSC_struct *structp;
	
	pchip = container_of(dev_found, struct P_chip, plf_dev);
	
	printk(KERN_ALERT "Found the platfrom device -- %s  %d \n", pchip->name, pchip->dev_no);

	comp1 = strcmp(pchip->name,"HSC_1");
	
	if(comp1 != 0){

		comp2 = strcmp(pchip->name,"HSC_2");

		if(comp2 !=0){
		printk("device not found cant register\n");
		return -1;
		}

		else
		{
      struct1[1] = kmalloc(sizeof(struct HSC_struct),GFP_KERNEL);
      structp = struct1[1];
      structp->minor_num = 6;
		}
	}

	else{
    struct1[0] = kmalloc(sizeof(struct HSC_struct),GFP_KERNEL);
    structp = struct1[0];
    structp->minor_num = 5; 
	}

	/***creating a class under sysfs and dynamically fetching a major number only once***/
	if(class_flag == 0)
	{

		sys_class = class_create(THIS_MODULE, CLASS_NAME);
        if (IS_ERR(sys_class)) {
            printk(KERN_ERR " cant create class %s\n", CLASS_NAME);
            class_unregister(sys_class);
       		class_destroy(sys_class);
       		return -EFAULT;
        }


  /* Request dynamic allocation of a device major number */
      if (alloc_chrdev_region(&devt, 0, 1, "HSC_1") < 0) {
          printk(KERN_DEBUG "Can't register device\n"); 
          return -1;
        }  
        class_flag = 1;

	} 
        /* Connect the file operations with the cdev */
        cdev_init(&structp->cdev, &fops);
        structp->cdev.owner = THIS_MODULE;

  /* Connect the major/minor number to the cdev */
        ret = cdev_add(&structp->cdev, MKDEV(MAJOR(devt),structp->minor_num) , 1);

        if (ret) {
                printk(KERN_DEBUG "Bad cdev\n");
                return ret;
        }
 
    /* Send uevents to udev, so it'll create /dev nodes */

     if(structp->minor_num == 5)
       structp->sys_device = device_create(sys_class, NULL, MKDEV(MAJOR(devt), 5), NULL, "HSC_1");
     else
       structp->sys_device = device_create(sys_class, NULL, MKDEV(MAJOR(devt), 6), NULL, "HSC_2");



/**********************creating device entries in class**********************/

  	 	rval = device_create_file(structp->sys_device, &dev_attr_TRIGGER);
        if (rval < 0) {
                printk(KERN_ERR  " cant create device attribute HSC_1 %s\n", 
                       dev_attr_TRIGGER.attr.name);
        }

  
  	 	rval = device_create_file(structp->sys_device, &dev_attr_ECHO);
        if (rval < 0) {
                printk(KERN_ERR  " cant create device attribute HSC_1 %s\n", 
                       dev_attr_ECHO.attr.name);
        }


  	 	rval = device_create_file(structp->sys_device, &dev_attr_NUMBER_SAMPLES);
        if (rval < 0) {
                printk(KERN_ERR  " cant create device attribute HSC_1 %s\n", 
                       dev_attr_NUMBER_SAMPLES.attr.name);
        }


  	 	rval = device_create_file(structp->sys_device, &dev_attr_SAMPLE_PERIOD);
        if (rval < 0) {
                printk(KERN_ERR  " cant create device attribute HSC_1 %s\n", 
                       dev_attr_SAMPLE_PERIOD.attr.name);
        }


  	 	rval = device_create_file(structp->sys_device, &dev_attr_ENABLE);
        if (rval < 0) {
                printk(KERN_ERR " cant create device attribute HSC_1 %s\n", 
                       dev_attr_ENABLE.attr.name);
        }


  	 	rval = device_create_file(structp->sys_device, &dev_attr_DISTANCE);
        if (rval < 0) {
                printk(KERN_ERR " cant create device attribute HSC_1 %s\n", 
                       dev_attr_DISTANCE.attr.name);
        }
       /*****************************************************************/			

	return 0;
};


static int __exit P_driver_remove(struct platform_device *pdev)
{
	struct P_chip *pchip;
  int comp;
  struct HSC_struct *structp;
  
  pchip = container_of(pdev, struct P_chip, plf_dev);
  
  printk(KERN_ALERT "Removing device -- %s \n", pchip->name);

  comp = strcmp(pchip->name,"HSC_1");
  
  if(comp == 0)
    structp = struct1[0];
  else
    structp = struct1[1];

 	free_irq (structp->irqnum,(void *)structp->minor_num); 
	hrtimer_cancel( &(structp->hr_timer));  	
  	
  if((structp->trigger >= 0)&&(structp->trigger <=19))
  	gpio_deregister_out(structp->trigger);//defined in pinmux.h

  if((structp->echo >= 0)&&(structp->echo <=19)) 
  	gpio_deregister_in(structp->echo);

  device_destroy(sys_class, MKDEV(MAJOR(devt), structp->minor_num));
  structp->minor_num = -1;

  cdev_del(&structp->cdev);

  if((struct1[0]->minor_num != 5)&&(struct1[1]->minor_num != 6))
  {
    unregister_chrdev_region(devt, 1);
    class_unregister(sys_class);
    class_destroy(sys_class);
    kfree(struct1[0]);
    kfree(struct1[1]);
  }  
    
    return 0;
};


static struct platform_driver P_driver = {
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
	.probe		= P_driver_probe,
	.remove		= P_driver_remove,
	.id_table	= P_id_table,
};


module_platform_driver(P_driver);
MODULE_LICENSE("GPL");

#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/math64.h>

#include "myioctl.h"
#include "pinmux.h"

#define MS_TO_NS(x) (x * 1E6L)


#define LOW 0
#define HIGH 1

#define dev_name "HSC_1"


struct HSC_struct{
bool triggerop;
bool flag1,flag2,busy;
int sample_num;
int irqnum;
int interval;
int samples;
int trigger;
int echo;
unsigned long trig;
unsigned long tdiff;
struct timeval ini;
struct timeval end;
unsigned long distance[20];
unsigned long timestamp[20];
struct hrtimer hr_timer;
spinlock_t my_lock;
}struct1;

struct buffer{
  unsigned long times[20];
  int dist;
};

static int timer_start(int);
static void timer_initialize(void);


static irqreturn_t sensor_handler(int irq, void *dev_id, struct pt_regs *regs);

static int sensor_open(struct inode *inode, struct file *file)
{
   //struct1 = kmalloc(sizeof(struct HSC_struct),GFP_KERNEL);
   printk(KERN_INFO "opened device with major %d and minor %d",imajor(inode),iminor(inode));
   
   
  struct1.irqnum = gpio_to_irq(pin_array[struct1.echo][0]);

   
   printk(KERN_INFO"finished operations in open");

   return 0;

}

static long sensor_ioctl(struct file* file,unsigned int cmd,unsigned long arg)
{
 
 int ret;
  pins p;
  conf c;

      if(cmd == CONFIG_PINS)
  {
          if (copy_from_user( &p, (pins *)arg,sizeof(pins)))
          {
              return -EACCES;
          }

          struct1.trigger = p.trigger;
          struct1.echo = p.echo;

          gpio_config(struct1.trigger , OUT);
          gpio_config(struct1.echo , IN);

          struct1.irqnum = gpio_to_irq(pin_array[struct1.echo][0]);
          printk("interrupt number = %d",struct1.irqnum);

      ret = request_irq(struct1.irqnum,(irq_handler_t)sensor_handler, IRQF_TRIGGER_RISING,
    "sensor_handler", NULL);
   
    printk("interrupt request info = %d",ret);

      printk("Trigger is %d and Echo is %d\n",p.trigger,p.echo);
  }
      else if (cmd == SET_PARAMS)
  {
          if (copy_from_user( &c, (conf *)arg,sizeof(conf)))
          {
              return -EACCES;
          }
          struct1.samples = c.samples+2;
          struct1.interval = c.interval;
          printk("Number of samples is %d and interval is %d",c.samples,c.interval);
          }
      else
    {
      printk("Invalid argument");
          return -ENOTTY;
  }

  return 0;
}

static ssize_t sensor_write(struct file *file, const char *data,size_t len, loff_t *ppos)
  {
   
   printk("I am in write function :)");

   struct1.flag1 = struct1.flag2 =1;
   struct1.triggerop = 0;
   struct1.sample_num = 0;

   //struct1.delta = 25;
   
   timer_initialize();
   //timer_start(struct1.delta);

   timer_start(60);
   printk("Initialization and start of timer finshed\n");

   while(struct1.flag1)
   {
    //for(j=0;j<5;j++);
    msleep_interruptible(1);
    //printk("in while loop\n");
    if(struct1.flag2)
    {
        //function();
        printk("performing while loop ops\n");
        struct1.triggerop = HIGH;    
        gpio_set_value_cansleep(pin_array[struct1.trigger][0],struct1.triggerop);
        do_gettimeofday(&struct1.ini);
        struct1.trig = (unsigned long)struct1.ini.tv_usec;
        udelay(10);
        struct1.triggerop = LOW;    
        gpio_set_value_cansleep(pin_array[struct1.trigger][0],struct1.triggerop);

        struct1.flag2 = 0;
        continue;
    }
    else
        continue;
   }
    return 0;


   }

ssize_t sensor_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{

int sum=0;
int i;

//struct buffer buff;

// for(i=0;i<(struct1.samples-1);i++)
// {
//   buff.times[i] = struct1.timestamp[i];
// }

for(i=1;i<(struct1.samples - 1);i++)
  sum=(sum+struct1.distance[i]);
//sum=sum/((struct1.samples-2));
do_div(sum,(struct1.samples-2));

//buff.dist = sum;

copy_to_user((int*)buf,&sum,sizeof(int));
return 0;
}




static int sensor_close(struct inode *inode, struct file *file)
{
  int ret;

  free_irq (struct1.irqnum,NULL);
 
  ret = hrtimer_cancel( &(struct1.hr_timer));
  if (ret)
  printk("The timer was still in use...\n");

  gpio_deregister(struct1.trigger,struct1.echo);
 
  return 0;
}


struct file_operations HSC = {
.owner = THIS_MODULE,
.open = sensor_open,
.release = sensor_close,
.write = sensor_write,
.unlocked_ioctl = sensor_ioctl
};

static struct miscdevice HSC_N= {
  .minor = MISC_DYNAMIC_MINOR,
  .name = dev_name,
  .fops = &HSC
};

static irqreturn_t sensor_handler(int irq, void *dev_id, struct pt_regs *regs)
{
   unsigned long flags;

   spin_lock_irqsave(&(struct1.my_lock), flags);
    do_gettimeofday(&struct1.end);
    struct1.timestamp[struct1.sample_num-1] = (jiffies * 1000)/HZ;
    struct1.tdiff = ((unsigned long)struct1.end.tv_usec) - struct1.trig;
    printk("tdiff = %lu, trig = %lu",struct1.tdiff,struct1.trig);
    struct1.tdiff = 170 * struct1.tdiff;
    struct1.distance[struct1.sample_num-1] = (struct1.tdiff)/10000;
    printk("distance %d = %lu", (struct1.sample_num), struct1.distance[struct1.sample_num-1]);
   spin_unlock_irqrestore(&(struct1.my_lock), flags);
   
   return IRQ_HANDLED;
   
}

static enum hrtimer_restart my_hrtimer_callback( struct hrtimer *timer )
{    
   ktime_t tim;
   printk("entered timer callback\n");
  if((struct1.sample_num) < struct1.samples)
  {
    printk("entered if block\n");
    //interval = ktime_set(0,MS_TO_NS(struct1.interval));
    tim = ktime_set(0,MS_TO_NS(60));
    struct1.sample_num++;
    struct1.flag2 = 1;
    hrtimer_forward_now(timer,tim);
    return HRTIMER_RESTART;
  }
  else
  {   
    printk("entered else block\n");
    struct1.flag1 = 0;
      return HRTIMER_NORESTART;
    //printk(KERN_DEBUG"HrTimer Over");
  }
}

static void timer_initialize(void)
{
 //printk("HR Timer module installing\n");

 hrtimer_init( &(struct1.hr_timer), CLOCK_MONOTONIC, HRTIMER_MODE_REL );
 
 struct1.hr_timer.function = &my_hrtimer_callback;
}

static int timer_start(int delay)
{
  ktime_t ktime;
   
  ktime = ktime_set( 0, MS_TO_NS(delay));
  //ktime = ktime_set( 0, MS_TO_NS(delay_in_ms) );
  //printk(KERN_DEBUG"Starting timer to fire in %ldms (%ld)\n",delay_in_ms,jiffies);
  hrtimer_start(&(struct1.hr_timer),ktime,HRTIMER_MODE_REL);
  return 0;
}



/* Module Initialization */
static int __init device_init(void)
{
   int error;
  error = misc_register(&HSC_N);
  if (error)
  {
    printk("can't misc_register of DEVICE \n");
    return error;
   }

   //struct1 = kmalloc(sizeof(HSC_struct),GFP_KERNEL);
   return 0;
}


/* Module Exit */
static void __exit device_exit(void)
{
   
 misc_deregister(&HSC_N);

 printk(KERN_INFO"EXTING MODULE");
}

module_init(device_init);
module_exit(device_exit);
MODULE_LICENSE("GPL");

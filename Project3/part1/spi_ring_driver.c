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
#include <linux/spi/spi.h>

#include "myioctl.h"

#define DRIVER_NAME   "spi_ring_driver_0"
#define CLASS_NAME "ws2812_class"

//device id table for spi devices
static const struct spi_device_id spi_id_table[] = {
  {"WS2812",0},
  {}
};


static struct class *sys_class;
static dev_t char_devt;

struct ws2812_struct{
struct cdev cdev;
struct device *char_dev;
struct spi_device *spi;
dev_t *spi_devt;
u32 speed_hz;
u8 *tx_buffer;
u8 *encode_buffer;
int bytes_transfer;
int data_buff_size;
spinlock_t spi_lock;
}*ws2812_struct;


static int ws2812_write_sync(struct ws2812_struct *);
int gpio_init(void);
int bits_encode(struct ws2812_struct*);

/*********************device file interface**********************************************/

static int ws2812_open(struct inode *inode, struct file *file)
{ 
  printk(KERN_INFO "opened character device with major %d and minor %d\n",imajor(inode),iminor(inode));

   file->private_data = container_of(inode->i_cdev,struct ws2812_struct,cdev);
   //allocating memory for write buffer and there is no need for read buffer since asynchronus
   ws2812_struct->data_buff_size = 48;
   ws2812_struct->bytes_transfer = 48*8;

   ws2812_struct->tx_buffer = kzalloc(ws2812_struct->data_buff_size,GFP_KERNEL);
   ws2812_struct->encode_buffer = kzalloc(ws2812_struct->bytes_transfer,GFP_KERNEL);

   printk("finished operations in open\n");
   return 0;
}

//function for encoding the bits using 8 bit encoding and 0XF0 for 1 and 0XC0 for 0
int bits_encode(struct ws2812_struct *ws2812_struct)
{ 
  int i,k;
  u8 num;
  
  for(i=0;i<ws2812_struct->data_buff_size;i++)
  {
    num = *(ws2812_struct->tx_buffer + i);

    for(k=0;k<8;k++)
    {
      if(num & 128)
        *(ws2812_struct->encode_buffer + i*8 + k) = 0XF0;
      else
        *(ws2812_struct->encode_buffer + i*8 + k) = 0XC0;

      num = num << 1 ;

    }
  }

  return 0;
}

//write function of device file interface
static ssize_t ws2812_write(struct file *file, const char *data,size_t len, loff_t *ppos)
{
    int ret;
    struct ws2812_struct *ws2812_struct;

    ws2812_struct = file->private_data;

       ret = copy_from_user(ws2812_struct->tx_buffer,(u8 *)data,len);

    bits_encode(ws2812_struct);

    if(ret)
      return -EFAULT;

    ret = ws2812_write_sync(ws2812_struct);
    
   return ret;
}

//function for sending data into spi device
int ws2812_write_sync( struct ws2812_struct *ws2812_struct)
{
  int ret;
  struct spi_transfer t ={
    .tx_buf = ws2812_struct->encode_buffer,
    .len = ws2812_struct->bytes_transfer,
    .speed_hz = ws2812_struct->speed_hz,
    .bits_per_word = 32,
  };

  struct spi_message m;

  spi_message_init(&m);
  m.spi = ws2812_struct->spi;
  spi_message_add_tail(&t,&m);

  if(ws2812_struct->spi == NULL)
    return -ESHUTDOWN;
  else{
    
    spin_lock_irq(&ws2812_struct->spi_lock);
    ret = spi_sync_locked(ws2812_struct->spi,&m);
    spin_unlock_irq(&ws2812_struct->spi_lock);
  }

  if(!ret)
    return ret;
  else
    return 0;
}

/********************ioctl command to reset the parameters of spi device***********/

static long ws2812_ioctl(struct file* file,unsigned int cmd,unsigned long arg)
{
  struct ws2812_struct *ws2812_struct = file->private_data;
  setup set;

  if(cmd == RESET)
  {
    if(copy_from_user(&set,(setup*)arg,sizeof(setup)))
      return -EACCES;

    ws2812_struct->spi->max_speed_hz = set.speed;
    ws2812_struct->spi->chip_select = set.cs;
    ws2812_struct->spi->bits_per_word = set.bits_word;

    if(set.mode == 0)
      ws2812_struct->spi->mode = SPI_MODE_0;
    else if(set.mode == 1)
      ws2812_struct->spi->mode = SPI_MODE_1;
    else if(set.mode == 2)
      ws2812_struct->spi->mode = SPI_MODE_2;
    else if(set.mode == 3)
      ws2812_struct->spi->mode = SPI_MODE_3;
    else 
    {
      printk("enter correct mode\n");
      return -1;
    }

    spi_setup(ws2812_struct->spi);
    udelay(50);
  }

  else
  {
    printk("ioctl command not found\n");
    return -1;
  }

  return 0;

}


static int ws2812_close(struct inode *inode, struct file *file)
{
  kfree(ws2812_struct->tx_buffer);
  return 0;
}

struct file_operations fops = {
  .owner = THIS_MODULE,
  .open = ws2812_open,
  .release = ws2812_close,
  .write = ws2812_write,
  .unlocked_ioctl = ws2812_ioctl
  };
/*************************************************************************************/



/**************************spi driver probe and remove*****************************/
int gpio_init()
{
  int ret;

  ret = gpio_request(24,"MOSI_1");
  ret = gpio_request(44,"MOSI_2");
  ret = gpio_request(72,"MOSI_3");

  ret = gpio_direction_output(24,0);
  ret = gpio_direction_output(44,1);
    
  gpio_set_value_cansleep(72,0);

  return ret;
}


static int ws2812_probe(struct spi_device *spi)
{
  int ret;

  ws2812_struct = kzalloc(sizeof(struct ws2812_struct),GFP_KERNEL);
    
  sys_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(sys_class)) {
      printk(KERN_ERR " cant create class %s\n", CLASS_NAME);
      class_unregister(sys_class);
      class_destroy(sys_class);
      return -EFAULT;
    }


  /* Request dynamic allocation of a device major number */
  if (alloc_chrdev_region(&char_devt, 0, 1, "ws2812_char_dev") < 0) {
      printk(KERN_DEBUG "Can't register device\n"); 
      return -1;
    }  

  /* Connect the file operations with the cdev */
  cdev_init(&ws2812_struct->cdev, &fops);
  ws2812_struct->cdev.owner = THIS_MODULE;

  /* Connect the major/minor number to the cdev */
  ret = cdev_add(&ws2812_struct->cdev, MKDEV(MAJOR(char_devt),0) , 1);

  if (ret) {
      printk(KERN_DEBUG "Bad cdev\n");
      return ret;
  }
 
  /* Send uevents to udev, so it'll create /dev nodes */

  ws2812_struct->char_dev = device_create(sys_class, NULL, MKDEV(MAJOR(char_devt), 0), NULL, "ws2812_char_dev");

  ws2812_struct->spi = spi;
  ws2812_struct->spi_devt = &spi->dev.devt;
  ws2812_struct->speed_hz = spi->max_speed_hz;

  spi_setup(ws2812_struct->spi);

  ret = gpio_init();

  if(ret)
    return -1;

  spi_set_drvdata(spi,ws2812_struct);

  return 0;

}

static int ws2812_remove(struct spi_device *spi)
{ 

  gpio_direction_output(44,0);

  gpio_free(24);
  gpio_free(44);
  gpio_free(72);

  //Release the major number
  unregister_chrdev_region(char_devt, 1);

  //Destroy device
  device_destroy(sys_class, MKDEV(MAJOR(char_devt), 0));
  cdev_del(&ws2812_struct->cdev);
  kfree(ws2812_struct);

  //Destroy the driver class
  class_destroy(sys_class);
  
  return 0;

}

static struct spi_driver ws2812_spi_driver = {
  .driver = {
    .name =   "WS2812",
    .owner =  THIS_MODULE,
  },
  .probe =  ws2812_probe,
  .remove = ws2812_remove,
  .id_table = spi_id_table,
};

/**********************************************************************************/


module_spi_driver(ws2812_spi_driver);
MODULE_LICENSE("GPL");

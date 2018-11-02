#include <linux/gpio.h>

// Ist element is gpio number 
// IInd element is level shifter pin
// IIIrd element is mux1 pin
// IVth element is mux2 pin


#define OUT 0
#define IN 1

void gpio_deregister_out(int io_trigger);
void gpio_deregister_in(int io_echo);
bool gpio_validate(int io_pin, bool dir);
int gpio_config(int io_pin, bool dir);

unsigned int pin_array[19][4]={
{11,32,-1,-1},
{12,28,45,-1},
{13,34,77,-1},
{14,16,76,64},
{6,36,-1,-1},
{0,18,66,-1},
{1,20,68,-1},
{38,-1,-1,-1},
{40,-1,-1,-1},
{4,22,70,-1},
{10,26,74,-1},
{5,24,44,72},
{15,42,-1,-1},
{7,30,46,-1}};

//pins p;
//conf c;


void gpio_deregister_out(int io_trigger)
{

  //deregistering the trigger pins 
  int i;
  gpio_set_value_cansleep(pin_array[io_trigger][0],0);
  for(i=0;i<4;i++)
  {
       if(pin_array[io_trigger][i] != -1)
       gpio_free(pin_array[io_trigger][i]);
  }

}

  void gpio_deregister_in(int io_echo)
  {
  // deregistering the echo pins 
    int i;
 for(i=0;i<4;i++)
  {
       if(pin_array[io_echo][i] != -1)
       gpio_free(pin_array[io_echo][i]);
  }

}

bool gpio_validate(int io_pin, bool dir)
{

if ( !((io_pin > 13) || (io_pin < 0)))
{
      if(dir == IN)
    {
        if ((io_pin == 7) || (io_pin == 8))
            return 1;
    }
}
else
{
    printk("Invalid pin number %d\n",io_pin);
    return 1;
}

return 0;

}

int gpio_config(int io_pin, bool dir){

int i;
char label[10];
for(i=0;i<4;i++)
{
    if((pin_array[io_pin][i] != -1))
    {
        sprintf(label,"random_%d",i);
        if ( gpio_request(pin_array[io_pin][i],label)<0)
        {
            printk(KERN_INFO"Can't request gpio pin %d of column %d\n",pin_array[io_pin][i],i);
            return -EINVAL;
        }
        else
            printk(KERN_INFO"\nGPIO pin %d requested succesfully\n",pin_array[io_pin][i]);
    }
}

// selecting the pinmux gpio
for(i=2;i<4;i++)
{
    if((pin_array[io_pin][i] != -1))
    {
           gpio_set_value_cansleep(pin_array[io_pin][i],0);
    printk(KERN_INFO"\nPIN MUX %d SET SUCCESSFULLY\n",pin_array[io_pin][i]);
    }
}

// setting the direction (level shifter) 
if((pin_array[io_pin][1] != -1))
{
      if(dir == OUT)
        {
            if((gpio_direction_output(pin_array[io_pin][1],0) != -1))
            printk(KERN_INFO"\nLevel shifter set as output successfully\n");
        }
        else
        {
            if((gpio_direction_input(pin_array[io_pin][1]) != -1))
            printk(KERN_INFO"\nLevel shifter set as input successfully\n");
        }
}

// setting the input/output to the linux gpio pin
if(dir == OUT)
{    if((gpio_direction_output(pin_array[io_pin][0],0) != -1))
    printk(KERN_INFO"GPIO pin %d set as o/p successfully\n",pin_array[io_pin][0]);
}
else
{    if((gpio_direction_input(pin_array[io_pin][0]) != -1))
    printk(KERN_INFO"GPIO pin %d set as i/p successfully\n",pin_array[io_pin][0]);
    //gpio_set_debounce(pin_array[io_pin][0],200);
}

return 0;
}

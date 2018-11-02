#ifndef MYCTL_H
#define MYCTL_H

#include <linux/ioctl.h>

typedef struct{
unsigned int echo;
unsigned int trigger;
}pins;

typedef struct{
unsigned int interval;
unsigned int samples;
}conf;

#define magic 0xFF
#define CONFIG_PINS _IOWR(magic, 1,pins *)
#define SET_PARAMS _IOWR(magic, 2, conf *)

#endif

#ifndef MYCTL_H
#define MYCTL_H

#include <linux/ioctl.h>

typedef struct{
uint32_t speed;
uint8_t cs;
uint8_t mode;
uint8_t bits_word;
}setup;

#define magic 0xFF
#define RESET _IOWR(magic, 1,setup *)

#endif

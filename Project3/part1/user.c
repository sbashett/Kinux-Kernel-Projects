#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include "myioctl.h"

int main()
{
	int fd,i,j,k;	
	int temp,ret;
	setup set;

	uint8_t buffer[] = { 200,0,0,
				0,200,0,
				0,0,200,
			   200,200,0,
			   200,0,200,
			   0,200,200,
			   200,200,200,
			   200,0,0,
			   0,200,0,
			   0,0,200,
			   200,200,0,
			   200,0,200,
			   0,200,200,
			   200,200,200,
			   50,150,200,
			   200,50,150
			   };


	fd = open("/dev/ws2812_char_dev", O_RDWR);

	if(fd<0){
		printf("could not open the device\n");
		return -1;
	}

	//setting the parameters of spi device
	set.speed = 6400000;
	set.cs = 1;
	set.mode = 3;
	set.bits_word = 32;

	ret= ioctl(fd,RESET,&set); 
	printf("%d",ret);

	for(k=0;k<20;k++)
	{
		for(j= 0 ; j<3; j++)
		{
		temp = buffer[0];
		for(i=0;i<47;i++){	
			buffer[i] = buffer[i+1];
		}
		buffer[47] = temp;
	}
		
		write(fd,(char *)buffer,48);
		sleep(1);
	}

	for (i=0;i<48;i++)
		buffer[i] = 0;
	write(fd,(char *)buffer,48);

	sleep(1);
	close(fd);
	return 0;
}
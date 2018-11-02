#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "myioctl.h"

int main()
{
	/* code */

	pins p;
	conf c;
	int fd1,fd2,ret;

	fd1 = open("/dev/HSC_1", O_RDWR);
	fd2 = open("/dev/HSC_2", O_RDWR);

	if((fd1<0) || (fd2<0))
	{

		printf("error opening file");
		return -1;
	}

	p.trigger = 8;
	p.echo = 3;

	c.interval=5;
	c.samples=5;

	
	ret= ioctl(fd1,CONFIG_PINS,&p); 
	printf("%d",ret);

	ret=ioctl(fd1,SET_PARAMS,&c);
	printf("%d",ret);

	p.trigger = 10;
	p.echo = 2;

	c.interval=5;
	c.samples=5;

	ret= ioctl(fd2,CONFIG_PINS,&p); 
	printf("%d",ret);

	ret=ioctl(fd2,SET_PARAMS,&c);
	printf("%d",ret);


	write(fd1,'\0',10);
	write(fd2,'\0',10);
	close(fd1);
	close(fd2);
	return 0;
}		

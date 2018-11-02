#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include "myioctl.h"

struct buffer{
  unsigned long times[20];
  int dist;
};

int main()
{
    /* code */

    pins p;
    conf c;
    int fd1,ret;
    fd1 = open("/dev/HSC_1",O_RDWR);
    //fd2 = open("/dev/HSC_2",O_RDWR);
    // struct buffer buf;
    char buf[20];

    if(fd1<0)
    {

   	 printf("error opening file\n");
    }


    printf("\nEnter the trigger pin :");
    scanf("%d",&p.trigger);
    printf("\nEnter the echo pin :");
    scanf("%d",&p.echo);
    
    printf("Enter the number of samples:");
    scanf("%d",&c.samples);
    printf("Enter the interval in ms");
    scanf("%d",&c.interval);

    printf("Calling IOCTL with ECHO PIN %d, TRIGGER PIN %d\nThe number of samples are %d, interval being %d\n",p.trigger,p.echo,c.samples,c.interval);
    ret= ioctl(fd1,CONFIG_PINS,&p);
    printf("%d",ret);

    ret=ioctl(fd1,SET_PARAMS,&c);
    printf("%d",ret);

    write(fd1,'\0',10);
  //  write(fd2,'\0',10);
    read(fd1,&buf,sizeof(int));
    printf("The distance value is %d",(int)buf);
    close(fd1);
    // close(fd2);
    return 0;
}    	


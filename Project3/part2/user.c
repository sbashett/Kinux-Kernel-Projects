#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>

int main() {

int fd,ret;
char b[10];
memset(b,0,sizeof(b));

fd=open("/dev/hrt_1",O_RDWR);
printf("%d",fd);
if (fd < 0)
{
	printf("Open Errrrr");
	return -1;
}

ret=write(fd,&b,sizeof(b));
printf("%d",ret);

if (ret < 0 )
{	printf("write Errr");
	return ret;
}

sleep(1);

close(fd);
return 0;
}

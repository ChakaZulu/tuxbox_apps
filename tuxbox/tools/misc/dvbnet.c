#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ost/net.h>

main(int argc, char **argv)
{
	int fd;
	int x,y;
	struct dvb_net_if net_if;

	if((fd = open("/dev/ost/net0",O_RDWR|O_NONBLOCK)) < 0)
	{
		perror("NET DEVICE: ");
		return -1;
	}

	if ( (ioctl(fd,NET_ADD_IF,&net_if) < 0))
	{
		perror("NET ADD: ");
		return -1;
	}

	if ( (ioctl(fd,NET_REMOVE_IF,&net_if) < 0))
	{
		perror("NET REMOVE: ");
		return -1;
	}

	close(fd);
	return 0;
}

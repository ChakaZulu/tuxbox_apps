#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
//#include <stdio.h>
#include "dbox/fb.h"

#define VERSION 1.0

void help (char *prog_name)
{
	printf("Version %s\n",VERSION);
	printf("Usage: %s <switches>\n\n",prog_name);
	printf("Switches:\n"
	"-h,     --help             help\n"
	"-p,     --pos <x> <y>      set position of fb\n"
	"-b,     --blev <val> <val> set the blev\n"
	"-s,     --show             show fb\n"
	"-u,     --hide             hide fb\n");
	exit(0);
}

int main(int argc, char **argv)
{
	int x, y, fd, count;
	if ((fd = open("/dev/fb/0", O_RDWR)) < 0)
	{
		perror("open /dev/fb/0");
		return -1;
	}

	for(count=1;count<argc;count++)
	{

		/* -h or --help */
		if ((strcmp("-h",argv[count]) == 0) || (strcmp("--help",argv[count]) == 0))
		{
			help(argv[0]);
		}
		else if ((strcmp("-p",argv[count]) == 0) || (strcmp("--pos",argv[count]) == 0))
		{
			if (argc < count+3)
			{
				printf("No coordinates given\n");
				exit(1);
			}
			else
			{
				count+=2;
				x = atoi(argv[count-1]);
				y = atoi(argv[count]);
				if (ioctl(fd, AVIA_GT_GV_SET_POS, (x << 16) | y) < 0)
				{
					perror("AVIA_GT_GV_SET_POS");
					return -1;
				}
			}
		}
		else if ((strcmp("-b",argv[count]) == 0) || (strcmp("--blev",argv[count]) == 0))
                {
                        if (argc < count+3)
                        {
                                printf("No blev given\n");
                                exit(1);
                        }
                        else
                        {
                                count+=2;
				x = atoi(argv[count - 1]);
				y = atoi(argv[count]);
				
				if (ioctl(fd, AVIA_GT_GV_SET_BLEV, (x << 8) | y) < 0)
				{
					perror("AVIA_GT_GV_SET_BLEV");
					return -1;
				}
			}
		}
		else if ((strcmp("-s",argv[count]) == 0) || (strcmp("--show",argv[count]) == 0))
		{
			if (ioctl(fd, AVIA_GT_GV_SHOW, 0) < 0)
			{
				perror("AVIA_GT_GV_SHOW");
				return -1;
			}
		}
		else if ((strcmp("-u",argv[count]) == 0) || (strcmp("--hide",argv[count]) == 0))
		{
			if (ioctl(fd, AVIA_GT_GV_HIDE, 0) < 0)
			{
				perror("AVIA_GT_GV_HIDE");
				return -1;
			}
		}
		else
		{
			help(argv[0]);
		}
	}

	close (fd);

	return 0;
}

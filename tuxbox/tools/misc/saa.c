#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/poll.h>

#include "../../driver/saa7126/saa7126_core.h"

#define VERSION "0.2pre"
void help(char *prog_name) {
  printf("Version %s\n",VERSION);
  printf("Usage: %s <options>\n\n",prog_name);
  printf("Switches:\n
      -h, --help            help
      -o, --power-save <X>  power save mode
                            0 power save off
                            1 power save on
      -r, --rgb             rgb mode
      -f, --fbas            fbas mode
      -s, --svideo          svideo mode
      -p, --pal             pal mode
      -n, --ntsc            ntsc mode
      -i, --input <X>       input control
                            MP1      = 1
                            MP2      = 2
                            CSYNC    = 4
                            DEMOFF   = 8
                            SYMP     = 16
                            COLORBAR = 128
\n");
  exit(0);
}

/** */
int vps()
{
	char data[6];

	memset(data,0,6);

	/* vps on */
	data[0] = 1;

	/* 0: ??? 1: mono 2: stereo 3: dual sound */
	data[1] |= 2;
}

/** */
main(int argc, char **argv)
{
	int fd;
	int count=1;
    int mode;
	int arg;

	if (argc < 2)
	{
		help(argv[0]);
	}

    if ((strcmp("-h",argv[count]) == 0) || (strcmp("--help",argv[count]) == 0)) {
      help(argv[0]);
	  return 0;
    }
    else if ((strcmp("-r",argv[count]) == 0) || (strcmp("--rgb",argv[count]) == 0)) {
	  arg = SAA_MODE_RGB;
	  mode = SAAIOSMODE;
    }
    else if ((strcmp("-f",argv[count]) == 0) || (strcmp("--fbas",argv[count]) == 0)) {
	  arg = SAA_MODE_FBAS;
	  mode = SAAIOSMODE;
    }
    else if ((strcmp("-s",argv[count]) == 0) || (strcmp("--svideo",argv[count]) == 0)) {
	  arg = SAA_MODE_SVIDEO;
	  mode = SAAIOSMODE;
    }
    else if ((strcmp("-p",argv[count]) == 0) || (strcmp("--pal",argv[count]) == 0)) {
	  arg = SAA_PAL;
	  mode = SAAIOSENC;
    }
    else if ((strcmp("-n",argv[count]) == 0) || (strcmp("--ntsc",argv[count]) == 0)) {
	  arg = SAA_NTSC;
	  mode = SAAIOSENC;
    }
    else if ((strcmp("-i",argv[count]) == 0) || (strcmp("--input",argv[count]) == 0)) {
      if(argc<3)
	  {
		help(argv[0]);
		return;
	  }

	  arg = atoi(argv[count+1]);
	  mode = SAAIOSINP;
    }
    else if ((strcmp("-o",argv[count]) == 0) || (strcmp("--power-save",argv[count]) == 0)) {
      if(argc<3)
	  {
		help(argv[0]);
		return;
	  }

	  arg = atoi(argv[count+1]);
	  mode = SAAIOSPOWERSAVE;
    }
	else {
      help(argv[0]);
	  return 0;
	}

	if((fd = open("/dev/dbox/saa0",O_RDWR|O_NONBLOCK)) < 0){
		perror("SAA DEVICE: ");
		return -1;
	}

	if ( (ioctl(fd,mode,&arg) < 0)){
		perror("IOCTL: ");
		return -1;
	}

	close(fd);
	return 0;
}

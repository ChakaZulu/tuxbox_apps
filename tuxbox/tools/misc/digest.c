#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
//#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include <ost/dmx.h>
#include <ost/frontend.h>
#include <ost/sec.h>
#include <ost/video.h>
#include <sys/poll.h>

#define VERSION 0.1
void help(char *prog_name) {
  printf("Version %s\n",VERSION);
  printf("Usage: %s <switches> <number/name/keyword>\n\n",prog_name);
  printf("Switches:\n
      -h, --help             help
      -p, --pan-scan         pan scan mode
      -l, --letter-box       letter box mode
      -c, --center-cut-out   center cut out mode
      -1                     4:3 mode
      -2                     16:9 mode
      -3                     20:9 mode\n");
  exit(0);
}

main(int argc, char **argv)
{
	int fd;
	int x,y;
	struct videoDigest digest;

	if((fd = open("/dev/ost/video0",O_RDWR|O_NONBLOCK)) < 0){
		perror("VIDEO DEVICE: ");
		return -1;
	}

	digest.skip = 0;
	digest.decimation = 4;
	digest.threshold = 0x0;
	digest.pictureID = 1;

	for(y=0;y<4;y++){
	for(x=0;x<4;x++){

		digest.x = 180*x;
		digest.y = 144*y;

		if ( (ioctl(fd,VIDEO_DIGEST,&digest) < 0)){
			perror("VIDEO_DIGEST: ");
			return -1;
		}

	}}

	close(fd);
	return 0;
}

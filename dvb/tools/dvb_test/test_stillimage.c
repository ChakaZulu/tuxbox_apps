/* display single iframes as stillimages
 * iframes can be created with the 'convert' tool from imagemagick
 * and mpeg2encode from ftp.mpeg.org, and must have a supported
 * size, e.g. 702x576:
 *   $ convert -sample 702x576\! test.jpg test.mpg
 */

#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include <linux/dvb/video.h>


static
const char *usage_string = "\n\tusage: %s <still.mpg> ...\n\n";


int main (int argc, char **argv)
{
	int fd;
	int filefd;
	struct stat st;
	struct video_still_picture sp;
	char *videodev = "/dev/dvb/adapter0/video0";
	int i = 1;

	if (argc < 2) {
		fprintf (stderr, usage_string, argv[0]);
		return -1;
	}

	if (getenv ("VIDEODEV"))
		videodev = getenv("VIDEODEV");

	if ((fd = open(videodev, O_RDWR)) < 0) {
		perror(videodev);
		return -1;
	}

next_pic:
	printf("I-frame     : '%s'\n", argv[i]);
	if ((filefd = open(argv[i], O_RDONLY)) < 0) {
		perror(argv[i]);
		return -1;
	}

	fstat(filefd, &st);

	sp.iFrame = (char *) malloc (st.st_size);
	sp.size = st.st_size;
	printf("I-frame size: %d\n", sp.size);

	if (!sp.iFrame) {
		fprintf (stderr, "No memory for I-Frame\n");
		return -1;
	}

	printf ("read: %d bytes\n", read(filefd, sp.iFrame, sp.size));
	close(filefd);

	if ((ioctl(fd, VIDEO_STILLPICTURE, &sp) < 0)) {
		perror("ioctl VIDEO_STILLPICTURE");
		return -1;
	}
	free(sp.iFrame);

	printf("Display image 10 seconds ...\n");
	sleep(10);
	printf("Done.\n");
	if (argc > ++i)
		goto next_pic;

	return 0;
}



/******************************************************************************
 *	Vgrab - vgrab.c
 *                                                                            
 *	Small test application to test the tuxbox Video4Linux-driver
 *
 *	(c) 2003 Carsten Juttner (carjay@gmx.net)
 *  									      
 * 	This program is free software; you can redistribute it and/or modify
 * 	it under the terms of the GNU General Public License as published by
 * 	The Free Software Foundation; either version 2 of the License, or
 * 	(at your option) any later version.
 *
 * 	This program is distributed in the hope that it will be useful,
 * 	but WITHOUT ANY WARRANTY; without even the implied warranty of
 * 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * 	GNU General Public License for more details.
 *
 * 	You should have received a copy of the GNU General Public License
 * 	along with this program; if not, write to the Free Software
 * 	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ******************************************************************************
 * $Id: vgrab.c,v 1.3 2005/01/25 01:49:59 carjay Exp $
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <linux/videodev.h>

#include <error.h>

#define VIDEODEV "/dev/v4l/video0"

int set_capture(int vfd, int left, int top, int width, int height, int scale_x, int scale_y){
	int stat;

	struct v4l2_format format;
	struct v4l2_crop crop;
	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;	// get existing capture crop
	stat = ioctl(vfd, VIDIOC_G_CROP, &crop);
	if (stat<0){
		perror ("error VIDIOC_G_CROP");
		return 1;
	}
	crop.c.left = left;	
	crop.c.top = top;
	crop.c.width = width;
	crop.c.height = height;
	stat = ioctl(vfd, VIDIOC_S_CROP, &crop);	// apply new capture crop
	if (stat<0){
		perror ("error VIDIOC_S_CROP");
		close (vfd);
		return 2;
	}

	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;	// get existing capture format
	stat = ioctl(vfd, VIDIOC_G_FMT, &format);
	if (stat<0){
		perror ("error VIDIOC_G_FMT");
		close (vfd);
		return 2;
	}
	format.fmt.pix.width = width/scale_x;
	format.fmt.pix.height = height/scale_y;

	stat = ioctl(vfd, VIDIOC_S_FMT, &format);	// set new capture format
	if (stat<0){
		perror ("error VIDIOC_S_FMT");
		close (vfd);
		return 2;
	}
	return 0;
}

void usage (char *name){
	printf ("Usage: %s <filename> <left> <top> <width> <height> <scale_x> <scale_y> [<opt>]\n"
		"       captures image and writes it to file. The first 4 bytes consist of\n"
		"        width and height of the image (bigendian)\n"
		"       parameters define the cropping window and the scaling that is\n"
		"        applied to it. There are several restrictions on the\n"
		"        parameters:\n"
		"         <top> must be even\n"
		"         <left> must be even\n"
		"         <width> must be even\n"
		"         <scale> must be an integer value 1..30\n"
		"         <opt> TODO\n", name);
}

int main(int argc, char **argv){
	struct v4l2_format format;

	int stat;
	int vfd, ofd;
	char fbuffer[720*576*2];
	char header[6];

	if (argc!=8){
		usage (argv[0]);
		return 1;
	}
	
	memset (fbuffer, 0, sizeof (fbuffer));
	
	vfd = open (VIDEODEV, O_RDONLY);
	if (vfd<0){
		perror ("Error opening video device");
		return 1;
	}

	ofd = open (argv[1],O_RDWR|O_CREAT|O_TRUNC, 00644 );
	if (ofd<0){
		perror ("Error opening output file");
		close (vfd);
		return 1;
	}

	// fd, left, top, width, height, scale_x, scale_y
	set_capture (vfd, atoi(argv[2]), atoi(argv[3]), 
			atoi(argv[4]), atoi(argv[5]), 
			atoi(argv[6]), atoi(argv[7]));

	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;	// get capture information
	stat = ioctl(vfd, VIDIOC_G_FMT, &format);
	if (stat<0){
		perror ("Error getting info with VIDIOC_G_FMT");
		close (vfd);
		close (ofd);
		return 1;
	}

	printf ("reported bytesperline: %d, height: %d, sizeimage: %d\n",
				format.fmt.pix.bytesperline, 
				format.fmt.pix.height,
				format.fmt.pix.sizeimage);

	stat = read (vfd, fbuffer, format.fmt.pix.bytesperline*format.fmt.pix.height);
	if (stat<0){
		perror ("Error reading buffer");
		close (vfd);
		close (ofd);
		return 1;
	}

	header[0] = (format.fmt.pix.bytesperline&0xff00)>>8;
	header[1] = format.fmt.pix.bytesperline&0xff;
	header[2] = (format.fmt.pix.height&0xff00)>>8;
	header[3] = format.fmt.pix.height&0xff;
	header[4] = 0x00;
	header[5] = 0x00;
	
	write (ofd, header, sizeof(header));
	stat = write (ofd, fbuffer, format.fmt.pix.bytesperline*format.fmt.pix.height);

	if (stat<0){
		perror ("Error writing buffer");
		close (vfd);
		close (ofd);
		return 1;
	}
	
	stat = close (vfd);
	if (stat<0){
		perror ("Error closing video device");
		close (ofd);
		return 1;
	}

	stat = close (ofd);
	if (stat<0){
		perror ("Error closing output file");
		return 1;
	}

	return 0;
}

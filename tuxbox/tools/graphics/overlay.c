/******************************************************************************
 *	Overlay - overlay.c
 *	small overlay demonstration and tester
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
 * $Id: overlay.c,v 1.1 2003/11/14 00:16:29 carjay Exp $
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev.h>
#include <fcntl.h>

#include <error.h>

#define V4L2 "/dev/v4l/video0"

int set_overlay(int vfd, int left, int top, int width, int height){
	int stat;

	struct v4l2_format format;
	struct v4l2_crop crop;
	crop.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;	// get existing overlay crop
	stat = ioctl(vfd, VIDIOC_G_CROP, &crop);
	if (stat<0){
		perror ("error VIDIOC_G_CROP ");
		return 1;
	}
	crop.c.left = 0;
	crop.c.top = 0;
	crop.c.width = 720;
	crop.c.height = 576;
	stat = ioctl(vfd, VIDIOC_S_CROP, &crop);	// apply new overlay crop
	if (stat<0){
		perror ("error VIDIOC_S_CROP ");
		close (vfd);
		return 2;
	}

	format.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;	// get existing overlay format
	stat = ioctl(vfd, VIDIOC_G_FMT, &format);
	if (stat<0){
		perror ("error VIDIOC_G_FMT ");
		close (vfd);
		return 2;
	}
	format.fmt.win.w.left   = left;
	format.fmt.win.w.top    = top;
	format.fmt.win.w.width  = width;
	format.fmt.win.w.height = height;

	stat = ioctl(vfd, VIDIOC_S_FMT, &format);	// set new overlay format
	if (stat<0){
		perror ("error VIDIOC_S_FMT ");
		close (vfd);
		return 2;
	}
	return 0;
}

void usage (char *name){
	printf ("Usage: %s <left> <top> <width> <height> <secs>\n"
		"          test tool for video overlay, displays an overlay of the\n"
		"           actual video image at the given position and parameters\n"
		"           driver corrects invalid settings\n"
		"          input size is always full screen\n"
		"           <secs> is time in seconds to display the image\n",name);

}

int main (int argc, char **argv){
	struct v4l2_format format;
	
	int vfd;
	int stat;
	int overlay;

	if (argc!=6){
		usage (argv[0]);
		return 1;
	}
	
	vfd = open (V4L2,O_RDWR);
	if (vfd<0){
		perror ("error opening v4l2-device ");
		return 1;
	}

	set_overlay(vfd, atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));

	format.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
	stat = ioctl(vfd, VIDIOC_G_FMT, &format);	// get format information
	if (stat<0){
		perror ("error VIDIOC_G_FMT ");
		close (vfd);
		return 2;
	}

	printf ("output overlay set to x:%d y:%d width:%d height:%d\n",
			format.fmt.win.w.left,
			format.fmt.win.w.top,
			format.fmt.win.w.width,
			format.fmt.win.w.height);

	overlay=1;
	stat = ioctl(vfd, VIDIOC_OVERLAY, &overlay);		// make overlay visible
	if (stat<0){
		perror ("error VIDIOC_OVERLAY \"on\" ");
		close (vfd);
		return 2;
	}

//	usleep (250000);
	sleep (5);
		overlay=0;
	stat = ioctl(vfd, VIDIOC_OVERLAY, &overlay);		// turn off overlay
	if (stat<0){
		perror ("error VIDIOC_OVERLAY \"off\" ");
		close (vfd);
		return 2;
	}

	close (vfd);
	return 0;
}

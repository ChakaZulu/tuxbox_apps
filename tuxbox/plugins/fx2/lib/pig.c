/*
** initial coding by fx2
*/

#include <fcntl.h>
#include <stdio.h>
#include <stropts.h>
#include <unistd.h>
#include <sys/ioctl.h>

#ifndef i386
//Narf ... sucks
#define _LINUX_TIME_H
#include <linux/videodev.h>

static	struct v4l2_format format;
static	int			fd = -1;
		int			fx2_use_pig = 1;

void	Fx2SetPig( int x, int y, int width, int height )
{
	int overlay;

	if ( fd==-1 )
		return;
	if (( x == format.fmt.win.w.left ) && ( y == format.fmt.win.w.top ) &&
		( width == format.fmt.win.w.width ) && ( height == format.fmt.win.w.height ))
			return;
			
	overlay = 0;
			
	ioctl(fd, VIDIOC_OVERLAY, &overlay);

	format.fmt.win.w.left=x;
	format.fmt.win.w.top=y;
	format.fmt.win.w.width=width;
	format.fmt.win.w.height=height;

	ioctl(fd, VIDIOC_S_FMT, &format);

	overlay = 1;
	
	ioctl(fd, VIDIOC_OVERLAY, &overlay);
}

void	Fx2ShowPig( int x, int y, int width, int height )
{
	int overlay;
	
	if ( fd != -1 )
	{
		Fx2SetPig(x,y,width,height);
		return;
	}
	if (( fd == -1 ) && fx2_use_pig )
		fd = open( "/dev/v4l/video0", O_RDONLY );
	if ( fd == -1 )
		return;

	format.fmt.win.w.left=x;
	format.fmt.win.w.top=y;
	format.fmt.win.w.width=width;
	format.fmt.win.w.height=height;

	ioctl(fd, VIDIOC_S_FMT, &format);

//FIXME	avia_pig_set_stack(fd,2);

	overlay = 1;
	
	ioctl(fd, VIDIOC_OVERLAY, &overlay);
}

void	Fx2StopPig( void )
{
	int overlay;

	if ( fd == -1 )
		return;

	overlay = 0;
	
	ioctl(fd, VIDIOC_OVERLAY, &overlay);

	close(fd);
	fd=-1;
}

void	Fx2PigPause( void )
{
	int overlay;

	if ( fd != -1 ) {
		overlay = 0;
		ioctl(fd, VIDIOC_OVERLAY, &overlay);
	}
}

void	Fx2PigResume( void )
{
	int overlay;

	if ( fd != -1 ) {
		overlay = 1;
		ioctl(fd, VIDIOC_OVERLAY, &overlay);
	}
}

#else

#include "draw.h"

	int			fx2_use_pig = 1;
static	int			l_x = 0;
static	int			l_y = 0;
static	int			l_width = 0;
static	int			l_height = 0;

void	Fx2SetPig( int x, int y, int width, int height )
{
	return;
}

void	Fx2ShowPig( int x, int y, int width, int height )
{
	FBFillRect( x, y, width, height, 4 );
	l_x=x;
	l_y=y;
	l_width=width;
	l_height=height;
	return;
}
void	Fx2StopPig( void )
{
	FBFillRect( l_x, l_y, l_width, l_height, 1 );
	return;
}
void	Fx2PigPause( void )
{
	return;
}
void	Fx2PigResume( void )
{
	return;
}
#endif

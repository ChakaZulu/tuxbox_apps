/*
** initial coding by fx2
*/

#include <fcntl.h>
#include <stdio.h>
#include <stropts.h>
#include <unistd.h>
#include <sys/ioctl.h>

#ifndef i386
#include <linux/videodev.h>

static	struct v4l2_window window = {

	.x = 0,
	.y = 0,
	.width = 0,
	.height = 0,
	.chromakey = 0,
	.clips = NULL,
	.clipcount = 0,
	.bitmap = NULL,
	
};
	
static	int			fd = -1;
		int			fx2_use_pig = 1;

void	Fx2SetPig( int x, int y, int width, int height )
{
	int preview;

	if ( fd==-1 )
		return;
	if (( x == window.x ) && ( y == window.y ) &&
		( width == window.height ) && ( height == window.height ))
			return;
			
	preview = 0;
			
	ioctl(fd, VIDIOC_PREVIEW, &preview);

	window.x=x;
	window.y=y;
	window.width=width;
	window.height=height;

	ioctl(fd, VIDIOC_S_WIN, &window);

	preview = 1;
	
	ioctl(fd, VIDIOC_PREVIEW, &preview);
}

void	Fx2ShowPig( int x, int y, int width, int height )
{
	int preview;
	
	if ( fd != -1 )
	{
		Fx2SetPig(x,y,width,height);
		return;
	}
	if (( fd == -1 ) && fx2_use_pig )
		fd = open( "/dev/v4l2/capture0", O_RDONLY );
	if ( fd == -1 )
		return;

	window.x=x;
	window.y=y;
	window.width=width;
	window.height=height;

	ioctl(fd, VIDIOC_S_WIN, &window);

//FIXME	avia_pig_set_stack(fd,2);

	preview = 1;
	
	ioctl(fd, VIDIOC_PREVIEW, &preview);
}

void	Fx2StopPig( void )
{
	int preview;

	if ( fd == -1 )
		return;

	preview = 0;
	
	ioctl(fd, VIDIOC_PREVIEW, &preview);

	close(fd);
	fd=-1;
}

void	Fx2PigPause( void )
{
	int preview;

	if ( fd != -1 ) {
		preview = 0;
		ioctl(fd, VIDIOC_PREVIEW, &preview);
	}
}

void	Fx2PigResume( void )
{
	int preview;

	if ( fd != -1 ) {
		preview = 1;
		ioctl(fd, VIDIOC_PREVIEW, &preview);
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

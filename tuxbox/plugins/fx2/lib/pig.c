/*
** initial coding by fx2
*/

#include <stdio.h>
#include <fcntl.h>
#include <stropts.h>

#ifndef i386
#include <dbox/avia_gt_pig.h>

static	int			fd = -1;
static	int			l_x = 0;
static	int			l_y = 0;
static	int			l_width = 0;
static	int			l_height = 0;
		int			fx2_use_pig = 1;

void	Fx2SetPig( int x, int y, int width, int height )
{
	if ( fd==-1 )
		return;
	if (( x == l_x ) && ( y == l_y ) &&
		( width == l_height ) && ( height == l_height ))
			return;
	avia_pig_hide(fd);
	if (( x != l_x ) || ( y != l_y ))
		avia_pig_set_pos(fd,x,y);
	if (( width != l_height ) || ( height != l_height ))
		avia_pig_set_size(fd,width,height);
	l_x=x;
	l_y=y;
	l_width=width;
	l_height=height;
	avia_pig_show(fd);
}

void	Fx2ShowPig( int x, int y, int width, int height )
{
	if ( fd != -1 )
	{
		Fx2SetPig(x,y,width,height);
		return;
	}
	if (( fd == -1 ) && fx2_use_pig )
		fd = open( "/dev/dbox/pig0", O_RDONLY );
	if ( fd == -1 )
		return;

	l_x=x;
	l_y=y;
	l_width=width;
	l_height=height;
	avia_pig_set_pos(fd,x,y);
	avia_pig_set_size(fd,width,height);
	avia_pig_set_stack(fd,2);

	avia_pig_show(fd);
}

void	Fx2StopPig( void )
{
	if ( fd == -1 )
		return;

	avia_pig_hide(fd);

	close(fd);
	fd=-1;
}

void	Fx2PigPause( void )
{
	if ( fd != -1 )
		avia_pig_hide(fd);
}

void	Fx2PigResume( void )
{
	if ( fd != -1 )
		avia_pig_show(fd);
}

#else

#include <draw.h>

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

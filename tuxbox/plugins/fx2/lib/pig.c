/*
** initial coding by fx2
*/

#include <stdio.h>
#include <fcntl.h>
#include <stropts.h>

#ifndef i386
#include <dbox/avia_pig.h>

static	int							fd = -1;

void	Fx2SetPig( int x, int y, int width, int height )
{
	avia_pig_set_pos(fd,x,y);
	avia_pig_set_size(fd,width,height);
}

void	Fx2ShowPig( int x, int y, int width, int height )
{
	if ( fd != -1 )
	{
		Fx2SetPig(x,y,width,height);
		return;
	}
	if ( fd == -1 )
		fd = open( "/dev/dbox/pig0", O_RDONLY );
	if ( fd == -1 )
		return;

	Fx2SetPig( x, y, width, height );
	avia_pig_set_stack(fd,0);

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

#else
void	Fx2SetPig( int x, int y, int width, int height )
{
	return;
}

void	Fx2ShowPig( int x, int y, int width, int height )
{
	return;
}
void	Fx2StopPig( void )
{
	return;
}
#endif

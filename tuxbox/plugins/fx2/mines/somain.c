/*
** initial coding by fx2
*/

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

#include <rcinput.h>
#include <draw.h>
#include <board.h>
#include <colors.h>

#include <../../mczap/neutrinoNG/plugins/gameplugins.h>

extern	int	doexit;
extern	int	debug;
extern	unsigned short	actcode;

static	void	setup_colors( void )
{
	FBSetColor( YELLOW, 255, 255, 0 );
	FBSetColor( GREEN, 0, 255, 0 );
	FBSetColor( RED, 255, 0, 0 );
	FBSetColor( STEELBLUE, 0, 0, 180 );
	FBSetColor( BLUE, 130, 130, 255 );
	FBSetColor( GRAY, 130, 130, 130 );
	FBSetColor( DARK, 1, 1, 1 );

	FBSetupColors( );
}

int mines_exec( int fdfb, int fdrc, int fdlcd, char *cfgfile )
{
	struct timeval	tv;
	int				x;

	if ( FBInitialize( 720, 576, 8, fdfb ) < 0 )
		return -1;

	setup_colors();

	if ( RcInitialize( fdrc ) < 0 )
		return -1;

	while( doexit != 3 )
	{
		BoardInitialize();
		DrawBoard( 0 );

		doexit=0;
		while( !doexit )
		{
			tv.tv_sec = 0;
			tv.tv_usec = 10000;
			x = select( 0, 0, 0, 0, &tv );		/* 100ms pause */
	
			RcGetActCode( );
			MoveMouse();
		}

		if ( doexit != 3 )
		{
			actcode=0xee;
			if ( doexit ==2 )
				DrawScore();
			else
				DrawGameOver();
			doexit=0;
			while(( actcode != RC_OK ) && !doexit )
			{
				tv.tv_sec = 0;
				tv.tv_usec = 100000;
				x = select( 0, 0, 0, 0, &tv );		/* 100ms pause */
				RcGetActCode( );
			}
		}
	}

	RcClose();
	FBClose();

	return 0;
}

int	mines_getInfo( struct SPluginInfo *info )
{
	info->pluginversion = 1;
	strcpy(info->name,"Minesweeper");
	strcpy(info->desc,"classic Minesweeper");
	strcpy(info->depend,"libfx2.so");
	info->type=1;
	info->needfb=1;
	info->needrc=1;
	info->needlcd=0;

	return 0;
}

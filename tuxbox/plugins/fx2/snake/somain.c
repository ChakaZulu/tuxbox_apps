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
#include <pig.h>
#include <colors.h>
#include <snake.h>

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
	FBSetColor( DARK, 60, 60, 60 );
	FBSetColor( GREEN2, 0, 200, 0 );

	FBSetupColors( );
}

int snake_exec( int fdfb, int fdrc, int fdlcd, char *cfgfile )
{
	struct timeval	tv;
	int				x;

	if ( FBInitialize( 720, 576, 8, fdfb ) < 0 )
		return -1;

	setup_colors();

	if ( RcInitialize( fdrc ) < 0 )
		return -1;

	Fx2ShowPig( 34*16, 28*16, 128, 96 );

	while( doexit != 3 )
	{
		DrawMaze( );	/* 0 = all */

		doexit=0;
		while( !doexit )
		{
			tv.tv_sec = 0;
			tv.tv_usec = 100000;
			x = select( 0, 0, 0, 0, &tv );		/* 10ms pause */
	
			RcGetActCode( );
			MoveSnake();
		}

		FreeSnake();

		if ( doexit != 3 )
		{
			actcode=0xee;
			DrawFinalScore();
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

	Fx2StopPig();

	RcClose();
	FBClose();

	return 0;
}

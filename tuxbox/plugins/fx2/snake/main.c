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
#include <snake.h>
#include <colors.h>

extern	int	doexit;
extern	int	debug;
extern	unsigned short	actcode;

#define Debug	if(debug)printf

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

static	void	sigproc( int snr )
{
	doexit = 3;
}

int main( int argc, char ** argv )
{
	struct timeval	tv;
	int				x;
	int				i;

	for( i=1; i < argc; i++ )
	{
		if ( !strcmp(argv[i],"-debug") )
			debug=1;
		else
			printf("%s [-debug]\n",*argv);
	}

	signal( SIGINT, sigproc );
	signal( SIGHUP, sigproc );
	signal( SIGTERM, sigproc );

	Debug("initialize framebuffer...\n");
	if ( FBInitialize( 720, 576, 8, -1 ) < 0 )
		return -1;

	setup_colors();

	Debug("initialize input-device...\n");
	if ( RcInitialize(-1) < 0 )
		return -1;

#ifdef i386
	FBDrawRect( 0, 0, 720, 576, 1 );
#endif

	Debug("mainloop...\n");

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
#ifdef USEX
			FBFlushGrafic();
#endif
		}

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

	RcClose();
	FBClose();

	return 0;
}

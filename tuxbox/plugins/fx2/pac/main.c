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
#include <maze.h>
#include <colors.h>

	int		doexit = 0;
	int		debug = 0;
extern	int	gametime;
extern	int	pices;
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

	while( doexit != 3 )
	{
		MazeInitialize();
		DrawMaze( );	/* 0 = all */
		DrawFill();
		DrawGhosts( );
		DrawPac( );

		doexit=0;
		while( !doexit )
		{
			tv.tv_sec = 0;
			tv.tv_usec = 1000;
			x = select( 0, 0, 0, 0, &tv );		/* 10ms pause */
	
			MovePac( );
			MoveGhosts( );
			DrawGhosts( );
			DrawPac( );
			RcGetActCode( );
			CheckGhosts( );
		}

		if ( doexit != 3 )
		{
//			printf("\nscore : %d\n",gametime);
			actcode=0xee;
			if ( gametime )
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
if ( actcode != 0xee )
Debug("warte aus OK - code is aber : %04x\n",actcode);
			}
		}
	}

	RcClose();
	FBClose();

	return 0;
}

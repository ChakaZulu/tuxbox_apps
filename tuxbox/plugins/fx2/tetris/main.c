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

	int		doexit = 0;
	int		debug = 0;
extern	unsigned short	actcode;

#define Debug	if(debug)printf


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

	Debug("initialize input-device...\n");
	if ( RcInitialize(-1) < 0 )
		return -1;

#ifdef i386
	FBDrawRect( 0, 0, 720, 576, WHITE );
#endif

	while( doexit != 3 )
	{
		BoardInitialize();
		DrawBoard( );	/* 0 = all */

		NextItem();

		doexit=0;
		while( !doexit )
		{
			tv.tv_sec = 0;
			tv.tv_usec = 1000;
			x = select( 0, 0, 0, 0, &tv );		/* 10ms pause */
			MoveSide();
			if ( !FallDown() )
			{
				RemoveCompl();
				if ( !NextItem() )
					doexit=1;
			}

			RcGetActCode( );
		}

		if ( doexit != 3 )
		{
			actcode=0xee;
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

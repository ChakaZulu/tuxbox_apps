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

	int		debug = 0;
	int		doexit = 0;
extern	unsigned short	actcode;

typedef struct _SPluginInfo
{
	char			name[20];
	char			desc[100];
	int				type;
	unsigned char	needfb;
	unsigned char	needrc;
	unsigned char	needlcd;
} SPluginInfo;

int tetris_exec( int fdfb, int fdrc, int fdlcd )
{
	struct timeval	tv;
	int				x;

	if ( FBInitialize( 720, 576, 8, fdfb ) < 0 )
		return -1;

	if ( RcInitialize( fdrc ) < 0 )
		return -1;

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

int	tetris_getInfo( SPluginInfo *info )
{
	strcpy(info->name,"Tetris");
	strcpy(info->desc,"i break together - tetris  :)");
	info->type=1;
	info->needfb=1;
	info->needrc=1;
	info->needlcd=0;

	return 0;
}

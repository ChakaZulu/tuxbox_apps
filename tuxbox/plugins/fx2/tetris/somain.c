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
#include <colors.h>
#include <board.h>

#include <../../mczap/neutrinoNG/plugins/gameplugins.h>

	int		debug = 0;
	int		doexit = 0;
extern	unsigned short	actcode;

static	void	setup_colors(void)
{
	FBSetColor( YELLOW, 255, 255, 0 );
	FBSetColor( GREEN, 0, 255, 0 );
	FBSetColor( RED, 255, 0, 0 );
	FBSetColor( STEELBLUE, 0, 0, 180 );
	FBSetColor( BLUE, 130, 130, 255 );
	FBSetColor( GRAY, 130, 130, 130 );
	FBSetColor( DARK, 60, 60, 60 );
	FBSetColor( RED1, 198, 131, 131 );
	FBSetColor( RED2, 216, 34, 49 );

/* magenta */
	FBSetColor( 30, 216, 175, 216);
	FBSetColor( 31, 205, 160, 207);
	FBSetColor( 32, 183, 131, 188);
	FBSetColor( 33, 230, 196, 231);
	FBSetColor( 34, 159, 56, 171);
	FBSetColor( 35, 178, 107, 182);
	FBSetColor( 36, 172, 85, 180);
	FBSetColor( 37, 180, 117, 184);
	FBSetColor( 38, 120, 1, 127);
	FBSetColor( 39, 89, 1, 98);
/* blue */
	FBSetColor( 40, 165, 172, 226);
	FBSetColor( 41, 148, 156, 219);
	FBSetColor( 42, 119, 130, 200);
	FBSetColor( 43, 189, 196, 238);
	FBSetColor( 44, 81, 90, 146);
	FBSetColor( 45, 104, 114, 185);
	FBSetColor( 46, 91, 103, 174);
	FBSetColor( 47, 109, 119, 192);
	FBSetColor( 48, 46, 50, 81);
	FBSetColor( 49, 34, 38, 63);
/* cyan */
	FBSetColor( 50, 157, 218, 234);
	FBSetColor( 51, 140, 208, 227);
	FBSetColor( 52, 108, 186, 211);
	FBSetColor( 53, 184, 233, 243);
	FBSetColor( 54, 55, 143, 172);
	FBSetColor( 55, 92, 171, 197);
	FBSetColor( 56, 78, 160, 187);
	FBSetColor( 57, 98, 177, 203);
	FBSetColor( 58, 7, 98, 120);
	FBSetColor( 59, 1, 78, 98);
/* green */
	FBSetColor( 60, 173, 218, 177);
	FBSetColor( 61, 158, 209, 165);
	FBSetColor( 62, 130, 189, 140);
	FBSetColor( 63, 195, 232, 199);
	FBSetColor( 64, 89, 138, 98);
	FBSetColor( 65, 115, 174, 122);
	FBSetColor( 66, 102, 163, 112);
	FBSetColor( 67, 121, 180, 129);
	FBSetColor( 68, 50, 77, 55);
	FBSetColor( 69, 38, 59, 41);
/* red */
	FBSetColor( 70, 239, 157, 152);
	FBSetColor( 71, 231, 141, 136);
	FBSetColor( 72, 210, 112, 109);
	FBSetColor( 73, 246, 184, 181);
	FBSetColor( 74, 153, 76, 74);
	FBSetColor( 75, 197, 97, 92);
	FBSetColor( 76, 184, 86, 81);
	FBSetColor( 77, 202, 101, 99);
	FBSetColor( 78, 95, 33, 32);
	FBSetColor( 79, 78, 20, 19);
/* yellow */
	FBSetColor( 80, 238, 239, 152);
	FBSetColor( 81, 230, 231, 136);
	FBSetColor( 82, 207, 214, 105);
	FBSetColor( 83, 246, 246, 181);
	FBSetColor( 84, 148, 157, 70);
	FBSetColor( 85, 194, 200, 89);
	FBSetColor( 86, 180, 189, 76);
	FBSetColor( 87, 199, 206, 95);
	FBSetColor( 88, 88, 93, 34);
	FBSetColor( 89, 69, 75, 22);
/* orange */
	FBSetColor( 90, 243, 199, 148);
	FBSetColor( 91, 237, 185, 130);
	FBSetColor( 92, 220, 159, 99);
	FBSetColor( 93, 249, 220, 178);
	FBSetColor( 94, 184, 113, 43);
	FBSetColor( 95, 208, 144, 81);
	FBSetColor( 96, 198, 132, 67);
	FBSetColor( 97, 213, 150, 88);
	FBSetColor( 98, 127, 63, 1);
	FBSetColor( 99, 98, 46, 1);

	FBSetupColors();
}

int tetris_exec( int fdfb, int fdrc, int fdlcd )
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

int	tetris_getInfo( struct SPluginInfo *info )
{
	info->pluginversion=1;
	strcpy(info->name,"Tetris");
	strcpy(info->desc,"i break together - tetris  :)");
	*info->depend=0;
	info->type=1;
	info->needfb=1;
	info->needrc=1;
	info->needlcd=0;

	return 0;
}

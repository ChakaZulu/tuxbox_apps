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
#include <plugin.h>
#include <fx2math.h>
#include <sprite.h>

extern	int	doexit;
extern	int	debug;
extern	int	gametime;
extern	int	pices;
extern	int	score;
extern	unsigned short	actcode;

extern	void	RemovePics( void );
extern	int		InitLemm( void );
extern	void	InitLevel( void );
extern	void	PicSetupColors( void );

/* special */
extern	void	ModifyColor( char picid, unsigned char cfrom, unsigned char cto );
extern	void	AnimateDeko( void );
extern	void	RunKey( void );
extern	void	RunLemm( void );
extern	void	RemoveBg( void );

static	void	setup_colors( void )
{
	FBSetColor( YELLOW, 255, 255, 20 );
	FBSetColor( GREEN, 20, 255, 20 );
	FBSetColor( STEELBLUE, 0, 0, 51 );
	FBSetColor( BLUE, 30, 30, 220 );
	FBSetColor( GRAY, 130, 130, 130 );
	FBSetColor( DARK, 60, 60, 60 );

	PicSetupColors();

	FBSetupColors( );
}

int lemmings_exec( int fdfb, int fdrc, int fdlcd, char *cfgfile )
{
	struct timeval	tv;
	int				x;
	int				rc=0;

	if ( FBInitialize( 720, 576, 8, fdfb ) < 0 )
		return -1;

	setup_colors();

	if ( RcInitialize( fdrc ) < 0 )
		return -1;

	while( doexit != 3 )
	{
		if ( InitLemm() != 0 )
			break;

		InitLevel();

		doexit=0;
		while( !doexit )
		{
			tv.tv_sec = 0;
			tv.tv_usec = 100000;
			x = select( 0, 0, 0, 0, &tv );		/* 10ms pause */

			RcGetActCode( );
			RunKey();

			AnimateDeko();
			RunLemm();
#ifdef USEX
			FBFlushGrafic();
#endif
		}

		FreeSprites();

		if ( doexit != 3 )
		{
			actcode=0xee;
#ifdef USEX
			FBFlushGrafic();
#endif
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

	RemoveBg();

	RemovePics();

	Fx2StopPig();

	RcClose();
	FBClose();

	return 0;
}

int plugin_exec( PluginParam *par )
{
	int		fd_fb=-1;
	int		fd_rc=-1;

	for( ; par; par=par->next )
	{
		if ( !strcmp(par->id,P_ID_FBUFFER) )
			fd_fb=_atoi(par->val);
		else if ( !strcmp(par->id,P_ID_RCINPUT) )
			fd_rc=_atoi(par->val);
		else if ( !strcmp(par->id,P_ID_NOPIG) )
			fx2_use_pig=!_atoi(par->val);
	}
	return lemmings_exec( fd_fb, fd_rc, -1, 0 );
}

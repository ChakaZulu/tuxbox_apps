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
#include <pig.h>
#include <plugin.h>
#include <fx2math.h>

#ifndef __i386__
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#endif

extern	int				debug;
extern	int				doexit;
extern	unsigned short	actcode;
extern	unsigned short	realcode;
extern	long			score;

static	char			*proxy_addr=0;
static	char			*proxy_user=0;
static	char			*hscore=0;

typedef struct _HScore
{
	char	name[12];
	long	points;
} HScore;

static	HScore	hsc[8];

static	void	LocalSave( void )
{
	int		x;
	char	*user;
	int		i;

	for( i=0; i < 8; i++ )
		if ( score > hsc[i].points )
			break;
	if ( i==8 )
		return;

	Fx2PigPause();

	FBFillRect( 500,32,3*52,4*52+4,BLACK );

	FBFillRect( 150,420,470,64,BLACK );
	FBDrawRect( 149,419,472,66,WHITE );
	FBDrawRect( 148,418,474,68,WHITE );
	x=FBDrawString( 154,420,64,"name : ",WHITE,0);
	user=FBEnterWord(154+x,420,64,9,WHITE);

	Fx2PigResume();

	if ( i < 7 )
		memmove( hsc+i+1,hsc+i,sizeof(HScore)*(7-i) );
	strcpy(hsc[i].name,user);
	hsc[i].points=score;
}

static	void	SaveGame( void )
{
#ifndef __i386__
	CURL		*curl;
	CURLcode	res;
#else
	int			res;
#endif
	FILE		*fp;
	char		url[ 512 ];
	char		*user="nobody";
	int			x;
	char		*p;
	struct timeval	tv;

	doexit=0;

	if ( score < 31 )
		return;

	if ( !hscore )
	{
		LocalSave();
		return;
	}

	FBDrawString( 150,350,64,"Save Highscore ? (OK/BLUE)",WHITE,0);

	while( realcode != 0xee )
		RcGetActCode();

	actcode=0xee;
	while( !doexit )
	{
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		select( 0,0,0,0, &tv );
		RcGetActCode();
		if ( actcode == RC_BLUE )
			return;
		if ( actcode == RC_OK )
			break;
	}
	if ( doexit )
		return;

	Fx2PigPause();

	FBFillRect( 150,350,570,64,BLACK );
	x=FBDrawString( 150,350,64,"name : ",WHITE,0);
	user=FBEnterWord(150+x,350,64,9,WHITE);

	Fx2PigResume();

/* clean name */
	x = strlen(user);
	p=user;
	for( p=user; *p; x--, p++ )
	{
		if (( *p == ' ' ) || ( *p == '&' ) || ( *p == '/' ))
			memcpy(p,p+1,x);
	}

#ifndef __i386__
	sprintf(url,"%s/games/tetris.php?action=put&user=%s&score=%d",
		hscore,user,score);

	curl = curl_easy_init();
	if ( !curl )
		return;
	fp = fopen( "/var/tmp/trash", "w");
	if ( !fp )
	{
		curl_easy_cleanup(curl);
		return;
	}
	curl_easy_setopt( curl, CURLOPT_URL, hscore );
	curl_easy_setopt( curl, CURLOPT_FILE, fp );
	curl_easy_setopt( curl, CURLOPT_NOPROGRESS, TRUE );
	if ( proxy_addr )
	{
		curl_easy_setopt( curl, CURLOPT_PROXY, proxy_addr );
		if ( proxy_user )
			curl_easy_setopt( curl, CURLOPT_PROXYUSERPWD, proxy_user );
	}
	res = curl_easy_perform(curl);
#else
	res=1;
#endif

	if ( !res )
		FBDrawString( 170,415,64,"success",WHITE,0);
	else
		FBDrawString( 170,415,64,"failed",WHITE,0);

#ifndef __i386__
	curl_easy_cleanup(curl);
	fclose( fp );
	unlink( "/var/tmp/trash" );
#endif

	tv.tv_sec = 2;
	tv.tv_usec = 0;
	select( 0,0,0,0, &tv );

	return;
}

static	void	ShowHScore( void )
{
	int				i;
	int				x;
	char			pp[64];

	FBFillRect( 0, 0, 720, 576, BLACK );

	FBDrawString( 190, 32, 64, "HighScore", RED, BLACK );
	for( i=0; i < 8; i++ )
	{
		FBDrawString( 100, 100+i*48, 48, hsc[i].name, WHITE, 0 );
		sprintf(pp,"%d",hsc[i].points);
		x = FBDrawString( 400, 100+i*48, 48, pp, BLACK, BLACK );
		FBDrawString( 500-x, 100+i*48, 48, pp, WHITE, BLACK );
	}
	while( realcode != 0xee )
		RcGetActCode();
}

static	void	setup_colors(void)
{
	FBSetColor( YELLOW, 255, 255, 0 );
	FBSetColor( GREEN, 0, 255, 0 );
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

int tetris_exec( int fdfb, int fdrc, int fdlcd, char *cfgfile )
{
	struct timeval	tv;
	int				x;
	int				i;
	int				fd;

	if ( FBInitialize( 720, 576, 8, fdfb ) < 0 )
		return -1;

	setup_colors();

	if ( RcInitialize( fdrc ) < 0 )
		return -1;

	fd = open( "/var/games/tetris.hscore", O_RDONLY );
	if ( fd == -1 )
	{
		for( i=0; i < 8; i++ )
		{
			strcpy(hsc[i].name,"nobody");
			hsc[i].points=30;
		}
	}
	else
	{
		read( fd, hsc, sizeof(hsc) );
		close(fd);
	}

	Fx2ShowPig( 450, 105, 128, 96 );

	while( doexit != 3 )
	{
		BoardInitialize();
		DrawBoard( );	/* 0 = all */
		NextItem();

		doexit=0;
		while( !doexit )
		{
			tv.tv_sec = 0;
			tv.tv_usec = 20000;
			x = select( 0, 0, 0, 0, &tv );		/* 10ms pause */
			MoveSide();
			if ( !FallDown() )
			{
				RemoveCompl();
				if ( !NextItem() )
					doexit=1;
			}
#ifdef USEX
			FBFlushGrafic();
#endif

			RcGetActCode( );
		}

		if ( doexit != 3 )
		{
			actcode=0xee;
			DrawGameOver();
			SaveGame();
			ShowHScore();
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

	Fx2StopPig();

	RcClose();
	FBClose();

/* save hscore */
	fd = open( "/var/games/tetris.hscore", O_CREAT|O_WRONLY, 438 );
	if ( fd != -1 )
	{
		write( fd, hsc, sizeof(hsc) );
		close(fd);
	}

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
		else if ( !strcmp(par->id,P_ID_PROXY) && par->val && *par->val )
			proxy_addr=par->val;
		else if ( !strcmp(par->id,P_ID_HSCORE) && par->val && *par->val )
			hscore=par->val;
		else if ( !strcmp(par->id,P_ID_PROXY_USER) && par->val && *par->val )
			proxy_user=par->val;
	}
	return tetris_exec( fd_fb, fd_rc, -1, 0 );
}

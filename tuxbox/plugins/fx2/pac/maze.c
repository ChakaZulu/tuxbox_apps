/*
** initial coding by fx2
*/


#include <stdio.h>

#include <draw.h>
#include <sys/time.h>
#include <rcinput.h>
#include <colors.h>
#include <pics.h>
#include <text.h>
#include <pig.h>
#include <level.h>

#define	STATUS_X		80
#define STATUS_Y		50
#define LOGO_X			600
#define LOGO_Y			30

/* time per food */
#define TPF				99

extern	double	sqrt( double in );

extern	int		doexit;

int				gametime=0;
extern	unsigned short	actcode;
int				pices = 0;
int				level = 0;
int				score = 0;
static	int		timeleft=0;

static	int		piccs[] = { 133, 125, 125, 133 };

typedef struct _Ghost
{
	int				x;
	int				y;
	int				x_minor;
	int				y_minor;
	unsigned char	c1;
	unsigned char	look;

} Ghost;

typedef struct _Pac
{
	int				x;
	int				y;
	int				x_minor;
	int				y_minor;
	int				look;
	int				step;
	int				c_step;

} Pac;

static	Ghost	ghost[4];
static	Ghost	ighost0[4] = {
{ 16, 3, 0, 0, BLUE, 0 },
{ 3, 3, 0, 0, GRAY, 0 },
{ 11, 11, 0, 0, RED, 1 },
{ 3, 12, 0, 0, DARK, 1 } };

static	Ghost	ighost1[4] = {
{ 1, 3, 0, 0, RED, 0 },
{ 11, 3, 0, 0, RED, 0 },
{ 16, 14, 0, 0, GREEN, 1 },
{ 5, 7, 0, 0, GREEN, 1 } };

static	Ghost	ighost2[4] = {
{ 5, 3, 0, 0, RED, 1 },
{ 16, 3, 0, 0, BLUE, 0 },
{ 11, 14, 0, 0, BLUE, 1 },
{ 8, 7, 0, 0, RED, 1 } };

static	Ghost	ighost3[4] = {
{ 1, 16, 0, 0, RED, 1 },
{ 2, 16, 0, 0, BLUE, 1 },
{ 8, 16, 0, 0, BLUE, 1 },
{ 9, 16, 0, 0, RED, 1 } };

static	unsigned char	pig_x[] = { 13, 13, 13, 13 };
static	unsigned char	pig_y[] = { 11, 3, 11, 11 };

static	Ghost	*ighosts[] = { ighost0, ighost1, ighost2, ighost3 };

static	Pac		pac;
static	Pac		ipac[] = {
{ 10, 9, 0, 0, 0, 0, 0 },
{ 7, 9, 0, 0, 1, 0, 0 },
{ 1, 3, 0, 0, 3, 0, 0 },
{ 1, 3, 0, 0, 0, 0, 0 },
 };

static	unsigned char	maze[ 22 * 20 ];

static	unsigned char	*mazes[] = { maze0, maze1, maze2, maze3 };

unsigned	char	*pacs[] = {
		pac_0_1, pac_0_2, pac_0_3, pac_0_4, pac_0_5,
		pac_1_1, pac_1_2, pac_1_3, pac_1_4, pac_1_5
};

void	DrawMaze( void )
{
	int				x;
	int				y;
	unsigned char	*p = maze;

	for( y = 0; y < MAZEH; y++ )
	{
		for( x = 0; x < MAZEW; x++ )
		{
			switch ( *p )
			{
			case '#' :
				FBFillRect( x*32, y*32, 32, 32, STEELBLUE );
				break;
			case '.' :
				FBCopyImage( x*32, y*32, 32, 32, futter );
				break;
			case 'z' :
				FBFillRect( x*32, y*32, 32, 32, 0 );
				break;
			default :
				FBFillRect( x*32, y*32, 32, 32, BLACK );
				break;
			}
			p++;
		}
	}
	FBCopyImage( LOGO_X, LOGO_Y, FX_WIDTH, 64, data_fx2 );
}

void	MazeInitialize( void )
{
	memcpy(maze,mazes[ level ],MAZEW*MAZEH);

	pac = ipac[ level ];
	gametime=0;
	actcode=0xee;
	maze[ pac.y * MAZEW + pac.x ] = ' ';
	pices = piccs[ level ]-1;
	timeleft=TPF*pices;
	memcpy(ghost,ighosts[ level ],sizeof(Ghost)*4);
}

void	DrawPac( void )
{
	int		istep = pac.step % 10;
	int		lstep;
	int		rstep;
	int		opac_step = pac.step;

	pac.step = pac.look * 10 + istep;
	if ( pac.step > 19 )
		pac.step = opac_step;
	lstep = pac.step - istep;
	rstep = lstep + 10;
	FBCopyImage( pac.x*32+pac.x_minor,
				pac.y*32+pac.y_minor, 32, 32, pacs[ pac.step/2 ] );

	if ( pac.c_step )
	{
		pac.step--;
		if ( pac.step < lstep )
		{
			pac.step = lstep + 1;
			pac.c_step = 0;
		}
	}
	else
	{
		pac.step++;
		if ( pac.step == rstep )
		{
			pac.step = rstep - 2;
			pac.c_step = 1;
		}
	}
}

void	DrawGhosts( void )
{
	int		i;

	for( i=0; i < 4; i++ )
	{
		FBOverlayImage( ghost[i].x*32+ghost[i].x_minor,
					ghost[i].y*32+ghost[i].y_minor,
					32, 32, ghost[i].x_minor, ghost[i].y_minor,
					ghost[i].c1,
					ghost_0_0,
					maze[ghost[i].y * MAZEW + ghost[i].x] == '.' ? futter : 0,
					ghost[i].x_minor &&
						(maze[ghost[i].y * MAZEW + ghost[i].x +1 ] == '.') ?
						futter : 0,
					ghost[i].y_minor &&
						(maze[(ghost[i].y+1)*MAZEW + ghost[i].x ] == '.' ) ?
						futter : 0 );
	}
}

static	void	DelOnePices( void )
{
	pices--;
	if ( pices < 0 )
		pices=0;
	FBPaintPixel( STATUS_X+pices, STATUS_Y, 0 );
	FBPaintPixel( STATUS_X+pices, STATUS_Y+1, 0 );
	FBPaintPixel( STATUS_X+pices, STATUS_Y+2, 0 );
	FBPaintPixel( STATUS_X+pices, STATUS_Y+3, 0 );
	if ( !pices )
	{
		gametime=timeleft;
		score+=gametime;
	}
}

static	int	collghost(int x, int y, int x_minor, int y_minor )
{
	int		i;
	int		mx;
	int		my;

#define ABS(a)		((a)<0?-(a):(a))
	for( i=0; i<4; i++ )
	{
		mx=(ghost[i].x-x)*32+ghost[i].x_minor - x_minor;
		my=(ghost[i].y-y)*32+ghost[i].y_minor - y_minor;
		mx=ABS(mx);
		my=ABS(my);
		if (( mx == 0 ) && ( my < 30 ))
			return 1;
		if (( my == 0 ) && ( mx < 26 ))
			return 1;
	}
	return 0;
}
void	MovePac( void )
{
static	int	cd = 40;
	if ( !pices )
	{
		cd--;
		if ( !cd )
		{
			doexit=2;
			return;
		}
	}
	else
		cd=40;
	timeleft--;
	if ( !(timeleft%TPF) )
	{
		FBPaintPixel( STATUS_X+(timeleft/TPF), STATUS_Y+6, 0 );
		FBPaintPixel( STATUS_X+(timeleft/TPF), STATUS_Y+7, 0 );
		FBPaintPixel( STATUS_X+(timeleft/TPF), STATUS_Y+8, 0 );
		FBPaintPixel( STATUS_X+(timeleft/TPF), STATUS_Y+9, 0 );
	}
	if ( !timeleft )
	{
		doexit=1;
		return;
	}
	if (( pac.x_minor == 0 ) && ( pac.y_minor == 0 ))
	{
		switch( actcode )
		{
		case RC_UP :
			if ( maze[ (pac.y-1) * MAZEW + pac.x ] != '#' )
				pac.look=2;
			break;
		case RC_DOWN :
			if ( maze[ (pac.y+1) * MAZEW + pac.x ] != '#' )
				pac.look=3;
			break;
		case RC_RIGHT :
			if ( maze[ pac.y * MAZEW + pac.x + 1 ] != '#' )
				pac.look=0;
			break;
		case RC_LEFT :
			if ( maze[ pac.y * MAZEW + pac.x - 1 ] != '#' )
				pac.look=1;
			break;
		}
	}
	switch( pac.look )
	{
	case 0 :		/* right */
		if ( pac.x_minor == 30 )
		{
			pac.x_minor = 0;
			pac.x++;
		}
		else
		{
			if ( pac.x_minor == 0 )		/* next field */
			{
				if ( maze[ pac.y * MAZEW + pac.x + 1 ] != '#' )
				{
					pac.x_minor+=2;
				}
			}
			else
			{
				if ( pac.x_minor == 16 )
				{
					if ( maze[ pac.y * MAZEW + pac.x + 1 ] == '.' )
					{
						DelOnePices();
					}
					maze[ pac.y * MAZEW + pac.x + 1 ] = ' ';
				}
				pac.x_minor+=2;
			}
		}
		break;
	case 1 :		/* left */
		if ( pac.x_minor > 0 )
		{
			pac.x_minor-=2;
			if ( pac.x_minor == 16 )
			{
				if ( maze[ pac.y * MAZEW + pac.x ] == '.' )
				{
					DelOnePices();
				}
				maze[ pac.y * MAZEW + pac.x ] = ' ';
			}
		}
		else
		{
			if ( maze[ pac.y * MAZEW + pac.x - 1 ] != '#' )
			{
				pac.x_minor=30;
				pac.x--;
			}
		}
		break;
	case 2 :		/* up */
		if ( pac.y_minor > 0 )
		{
			pac.y_minor-=2;
			if ( pac.y_minor == 16 )
			{
				if ( maze[ pac.y * MAZEW + pac.x ] == '.' )
				{
					DelOnePices();
				}
				maze[ pac.y * MAZEW + pac.x ] = ' ';
			}
		}
		else
		{
			if ( maze[ (pac.y-1) * MAZEW + pac.x ] != '#' )
			{
				pac.y_minor=30;
				pac.y--;
			}
		}
		break;
	case 3 :		/* down */
		if ( pac.y_minor == 30 )
		{
			pac.y_minor = 0;
			pac.y++;
		}
		else
		{
			if ( pac.y_minor == 0 )		/* next field */
			{
				if ( maze[ (pac.y+1) * MAZEW + pac.x ] != '#' )
				{
					pac.y_minor+=2;
				}
			}
			else
			{
				if ( pac.y_minor == 16 )
				{
					if ( maze[ (pac.y+1) * MAZEW + pac.x ] == '.' )
					{
						DelOnePices();
					}
					maze[ (pac.y+1) * MAZEW + pac.x ] = ' ';
				}
				pac.y_minor+=2;
			}
		}
		break;
	}
}

static	void	RunGhostRandom( int nr )
{
	struct timeval	tv;

	gettimeofday(&tv,0);

	ghost[nr].look = tv.tv_usec % 4;
}

static	void	RunGhostLikeFB( int nr )
{
	if ( ghost[nr].x_minor || ghost[nr].y_minor )
		return;
	switch( actcode )
	{
	case RC_UP :
		if ( maze[ (ghost[nr].y-1) * MAZEW + ghost[nr].x ] != '#' )
			ghost[nr].look=2;
		break;
	case RC_DOWN :
		if ( maze[ (ghost[nr].y+1) * MAZEW + ghost[nr].x ] != '#' )
			ghost[nr].look=3;
		break;
	case RC_RIGHT :
		if ( maze[ ghost[nr].y * MAZEW + ghost[nr].x + 1 ] != '#' )
			ghost[nr].look=0;
		break;
	case RC_LEFT :
		if ( maze[ ghost[nr].y * MAZEW + ghost[nr].x - 1 ] != '#' )
			ghost[nr].look=1;
		break;
	}
}

static	void	RunGhostLikePac( int nr )
{
	if ( ghost[nr].x_minor || ghost[nr].y_minor )
		return;
	switch( pac.look )
	{
	case 2 :
		if ( maze[ (ghost[nr].y-1) * MAZEW + ghost[nr].x ] != '#' )
			ghost[nr].look=2;
		break;
	case 3 :
		if ( maze[ (ghost[nr].y+1) * MAZEW + ghost[nr].x ] != '#' )
			ghost[nr].look=3;
		break;
	case 0 :
		if ( maze[ ghost[nr].y * MAZEW + ghost[nr].x + 1 ] != '#' )
			ghost[nr].look=0;
		break;
	case 1 :
		if ( maze[ ghost[nr].y * MAZEW + ghost[nr].x - 1 ] != '#' )
			ghost[nr].look=1;
		break;
	}
}

static	int	isghost(int y, int x )
{
	int		i;

	for( i=0; i<4; i++ )
	{
		if (( ghost[i].y == y ) && ( ghost[i].x == x ))
			return 1;
	}
	return 0;
}

static	int	MoveAGhost( int nr )
{
	switch( ghost[nr].look )
	{
	case 0 :		/* right */
		if ( ghost[nr].x_minor == 30 )
		{
			ghost[nr].x_minor = 0;
			ghost[nr].x++;
		}
		else
		{
			if ( ghost[nr].x_minor == 0 )		/* next field */
			{
				if (( maze[ ghost[nr].y * MAZEW + ghost[nr].x + 1 ] != '#' ) &&
					!isghost(ghost[nr].y,ghost[nr].x + 1) )
				{
					ghost[nr].x_minor+=2;
				}
				else
					return 1;
			}
			else
			{
				ghost[nr].x_minor+=2;
			}
		}
		break;
	case 1 :		/* left */
		if ( ghost[nr].x_minor > 0 )
		{
			ghost[nr].x_minor-=2;
		}
		else
		{
			if (( maze[ ghost[nr].y * MAZEW + ghost[nr].x - 1 ] != '#' ) &&
				!isghost(ghost[nr].y,ghost[nr].x - 1) )
			{
				ghost[nr].x_minor=30;
				ghost[nr].x--;
			}
			else
				return 1;
		}
		break;
	case 2 :		/* up */
		if ( ghost[nr].y_minor > 0 )
		{
			ghost[nr].y_minor-=2;
		}
		else
		{
			if (( maze[ (ghost[nr].y-1) * MAZEW + ghost[nr].x ] != '#' ) &&
				!isghost(ghost[nr].y-1,ghost[nr].x) )
			{
				ghost[nr].y_minor=30;
				ghost[nr].y--;
			}
			else
				return 1;
		}
		break;
	case 3 :		/* down */
		if ( ghost[nr].y_minor == 30 )
		{
			ghost[nr].y_minor = 0;
			ghost[nr].y++;
		}
		else
		{
			if ( ghost[nr].y_minor == 0 )		/* next field */
			{
				if (( maze[ (ghost[nr].y+1) * MAZEW + ghost[nr].x ] != '#' ) &&
					!isghost(ghost[nr].y+1,ghost[nr].x) )
				{
					ghost[nr].y_minor+=2;
				}
				else
					return 1;
			}
			else
			{
				ghost[nr].y_minor+=2;
			}
		}
		break;
	}
	return 0;
}

void	MoveGhosts( void )
{
	MoveAGhost(0);
	RunGhostLikePac( 0 );
	if ( MoveAGhost(1) )
		RunGhostRandom( 1 );
	if ( MoveAGhost(2) )
		RunGhostLikeFB( 2 );
	MoveAGhost(3);
	RunGhostLikePac( 3 );
}

void	CheckGhosts( void )
{
	if ( collghost( pac.x, pac.y, pac.x_minor, pac.y_minor ) )
		doexit=1;
}

void	DrawFill( void )
{
	FBFillRect( STATUS_X, STATUS_Y, pices, 4, GREEN );
	FBFillRect( STATUS_X, STATUS_Y+6, (timeleft/TPF), 4, RED );
}

void	DrawGameOver( void )
{
	FBCopyImage( 250, 200, GO_WIDTH, 64, data_gameover );
}

void	DrawScore( void )
{
	int		ww[10] = {	NO_0_WIDTH, NO_1_WIDTH, NO_2_WIDTH, NO_3_WIDTH,
						NO_4_WIDTH, NO_5_WIDTH, NO_6_WIDTH, NO_7_WIDTH,
						NO_8_WIDTH, NO_9_WIDTH };
	unsigned char	*nn[10] = {
						data_no0, data_no1, data_no2, data_no3,
						data_no4, data_no5, data_no6, data_no7,
						data_no8, data_no9 };

	char	cscore[ 64 ];
	char	*p=cscore;
	int		x = 250 + SC_WIDTH + 18;
	int		h;

	sprintf(cscore,"%d",score*100);
	FBFillRect( 250,264, SC_WIDTH + 19, 64, BLACK );
	FBCopyImage( 250, 264, SC_WIDTH, 64, data_score );

	for( ; *p; p++ )
	{
		h = (*p - 48);
		FBCopyImage( x, 264, ww[h], 64, nn[h] );
		x += ww[h];
	}
}

void	InitLevel( void )
{
	level=0;
	score=0;
}

void	NextLevel( void )
{
	level++;
	if ( level > 3 )
		level=0;
}

void	MazePig( void )
{
	Fx2ShowPig( (int)pig_x[ level ]*32+51, (int)pig_y[level]*32-10, 256, 208 );
}

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

#if 0
#define	STATUS_X		5
#define STATUS_Y		5
#define LOGO_X			650
#define LOGO_Y			0
#else
#define	STATUS_X		80
#define STATUS_Y		50
#define LOGO_X			600
#define LOGO_Y			30
#endif

extern	double	sqrt( double in );

extern	int		doexit;

int				pac_x = 10;
int				pac_x_minor = 0;
int				pac_y = 9;
int				pac_y_minor = 0;
int				pac_look = 0;
int				pac_step = 0;
int				pac_c_step = 0;
int				gametime=0;
extern	unsigned short	actcode;
int				pices = PICES;
static	int		timeleft=8600;

typedef struct _Ghost
{
	int				x;
	int				y;
	int				x_minor;
	int				y_minor;
	unsigned char	c1;
	unsigned char	look;
} Ghost;

static	Ghost	ghost[4];

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
	int				x;
	int				y;
	unsigned char	*p = maze+2*MAZEW;

	for( y = 0; y < MAZEH-2; y++ )
	{
		for( x = 0; x < MAZEW; x++, p++ )
		{
			if ( *p != '#' )
				*p='.';
		}
	}
	pac_x = 10;
	pac_x_minor = 0;
	pac_y = 9;
	pac_y_minor = 0;
	pac_look = 0;
	pac_step = 0;
	pac_c_step = 0;
	gametime=0;
	actcode=0xee;
	maze[ pac_y * MAZEW + pac_x ] = ' ';
	pices = PICES-1;
	timeleft=8600;
	ghost[0].x = 16;
	ghost[0].y = 3;
	ghost[0].x_minor = 0;
	ghost[0].y_minor = 0;
	ghost[0].c1 = BLUE;
	ghost[0].look = 0;
	ghost[1].x = 3;
	ghost[1].y = 3;
	ghost[1].x_minor = 0;
	ghost[1].y_minor = 0;
	ghost[1].c1 = GRAY;
	ghost[1].look = 0;
	ghost[2].x = 16;
	ghost[2].y = 11;
	ghost[2].x_minor = 0;
	ghost[2].y_minor = 0;
	ghost[2].c1 = RED;
	ghost[2].look = 1;
	ghost[3].x = 3;
	ghost[3].y = 12;
	ghost[3].x_minor = 0;
	ghost[3].y_minor = 0;
	ghost[3].c1 = DARK;
	ghost[3].look = 1;
}

void	DrawPac( void )
{
	int		istep = pac_step % 10;
	int		lstep;
	int		rstep;
	int		opac_step = pac_step;

	pac_step = pac_look * 10 + istep;
	if ( pac_step > 19 )
		pac_step = opac_step;
	lstep = pac_step - istep;
	rstep = lstep + 10;
	FBCopyImage( pac_x*32+pac_x_minor,
				pac_y*32+pac_y_minor, 32, 32, pacs[ pac_step/2 ] );

	if ( pac_c_step )
	{
		pac_step--;
		if ( pac_step < lstep )
		{
			pac_step = lstep + 1;
			pac_c_step = 0;
		}
	}
	else
	{
		pac_step++;
		if ( pac_step == rstep )
		{
			pac_step = rstep - 2;
			pac_c_step = 1;
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
	FBPaintPixel( STATUS_X+pices, STATUS_Y, 0 );
	FBPaintPixel( STATUS_X+pices, STATUS_Y+1, 0 );
	if ( !pices )
	{
		gametime=timeleft;
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
	if ( !(timeleft%52) )
	{
		FBPaintPixel( STATUS_X+(timeleft/52), STATUS_Y+4, 0 );
		FBPaintPixel( STATUS_X+(timeleft/52), STATUS_Y+5, 0 );
	}
	if ( !timeleft )
	{
		doexit=1;
		return;
	}
	if (( pac_x_minor == 0 ) && ( pac_y_minor == 0 ))
	{
		switch( actcode )
		{
		case RC_UP :
			if ( maze[ (pac_y-1) * MAZEW + pac_x ] != '#' )
				pac_look=2;
			break;
		case RC_DOWN :
			if ( maze[ (pac_y+1) * MAZEW + pac_x ] != '#' )
				pac_look=3;
			break;
		case RC_RIGHT :
			if ( maze[ pac_y * MAZEW + pac_x + 1 ] != '#' )
				pac_look=0;
			break;
		case RC_LEFT :
			if ( maze[ pac_y * MAZEW + pac_x - 1 ] != '#' )
				pac_look=1;
			break;
		}
	}
	switch( pac_look )
	{
	case 0 :		/* right */
		if ( pac_x_minor == 30 )
		{
			pac_x_minor = 0;
			pac_x++;
		}
		else
		{
			if ( pac_x_minor == 0 )		/* next field */
			{
				if ( maze[ pac_y * MAZEW + pac_x + 1 ] != '#' )
				{
					pac_x_minor+=2;
				}
			}
			else
			{
				if ( pac_x_minor == 16 )
				{
					if ( maze[ pac_y * MAZEW + pac_x + 1 ] == '.' )
					{
						pices--;
						DelOnePices();
					}
					maze[ pac_y * MAZEW + pac_x + 1 ] = ' ';
				}
				pac_x_minor+=2;
			}
		}
		break;
	case 1 :		/* left */
		if ( pac_x_minor > 0 )
		{
			pac_x_minor-=2;
			if ( pac_x_minor == 16 )
			{
				if ( maze[ pac_y * MAZEW + pac_x ] == '.' )
				{
					pices--;
					DelOnePices();
				}
				maze[ pac_y * MAZEW + pac_x ] = ' ';
			}
		}
		else
		{
			if ( maze[ pac_y * MAZEW + pac_x - 1 ] != '#' )
			{
				pac_x_minor=30;
				pac_x--;
			}
		}
		break;
	case 2 :		/* up */
		if ( pac_y_minor > 0 )
		{
			pac_y_minor-=2;
			if ( pac_y_minor == 16 )
			{
				if ( maze[ pac_y * MAZEW + pac_x ] == '.' )
				{
					pices--;
					DelOnePices();
				}
				maze[ pac_y * MAZEW + pac_x ] = ' ';
			}
		}
		else
		{
			if ( maze[ (pac_y-1) * MAZEW + pac_x ] != '#' )
			{
				pac_y_minor=30;
				pac_y--;
			}
		}
		break;
	case 3 :		/* down */
		if ( pac_y_minor == 30 )
		{
			pac_y_minor = 0;
			pac_y++;
		}
		else
		{
			if ( pac_y_minor == 0 )		/* next field */
			{
				if ( maze[ (pac_y+1) * MAZEW + pac_x ] != '#' )
				{
					pac_y_minor+=2;
				}
			}
			else
			{
				if ( pac_y_minor == 16 )
				{
					if ( maze[ (pac_y+1) * MAZEW + pac_x ] == '.' )
					{
						pices--;
						DelOnePices();
					}
					maze[ (pac_y+1) * MAZEW + pac_x ] = ' ';
				}
				pac_y_minor+=2;
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
	switch( pac_look )
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
	if ( collghost( pac_x, pac_y, pac_x_minor, pac_y_minor ) )
		doexit=1;
}

void	DrawFill( void )
{
	FBFillRect( STATUS_X, STATUS_Y, pices, 2, GREEN );
	FBFillRect( STATUS_X, STATUS_Y+4, (timeleft/52), 2, RED );
}

void	DrawGameOver( void )
{
	FBCopyImage( 250, 290, GO_WIDTH, 64, data_gameover );
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

	char	score[ 64 ];
	char	*p=score;
	int		x = 250 + SC_WIDTH + 18;
	int		h;

	sprintf(score,"%d",gametime*100);
	FBFillRect( 250,250, SC_WIDTH + 19, 64, BLACK );
	FBCopyImage( 250, 250, SC_WIDTH, 64, data_score );

	for( ; *p; p++ )
	{
		h = (*p - 48);
		FBCopyImage( x, 250, ww[h], 64, nn[h] );
		x += ww[h];
	}
}

/*
** initial coding by fx2
*/


#include <stdio.h>
#include <stdlib.h>

#include <draw.h>
#include <sys/time.h>
#include <rcinput.h>
#include <colors.h>
#include <sprite.h>

extern	int		doexit;

extern	unsigned short	realcode;
extern	unsigned short	actcode;

extern	unsigned char	*GetAniPic( int idx, int ani, int *width, int *height );
extern	int				LoadPics( void );

static	int	portfolio[8];		// Wieviele Stopper/Buddler/Lemmings
static	int	level=1;
		int	main_x=1320;
static	Sprite	*deko[10];
static	Sprite	*lemm[100];
static	unsigned char	*bgImage=0;
static	int		haus_x;
static	int		haus_y1;
static	int		haus_y2;
static	int		to_rescue;
static	int		newspeed;
static	int		in_level;		// gerade im level vorhanden
static	int		lem_in;			// im haus
static	int		lem_cnt;		// anz lemming die kommen
static	int		lem_run;		// anz lemming die da sind
static	int		clip1=0;
static	int		clip2=0;
static	int		pic_moved=0;
static	int		action=0;
static	int		pause=0;
static	int		afunc=-1;
static	int		sel_sprite=-1;

static	unsigned char	*svdimage[12];

#define	stride	3200
#define	lfb		bgImage

static	int		isBrick( int x, int y )
{
	return ((y<320)&&(*(lfb+stride*y+x)!=14)&&(*(lfb+stride*y+x)!=STEELBLUE));
}

static	void	CopyBg2Screen( int srcx, int srcy, int dstx, int dsty,
					int dx, int dy )
{
	int				y;
	unsigned char	*p = bgImage+(srcy*stride)+srcx;

	/* line per line ! - because stride */
	for( y=0; y<dy; y++, p += stride )
		FBCopyImage( dstx, dsty+y, dx, 1, p );
}

void	UndrawSprite( Sprite *s )
{
	CopyBg2Screen( s->oldx,s->oldy,
			s->oldx-main_x+32, s->oldy+32,
			s->width+s->width, s->height+s->height);
}


void	bgGetImage( int x1, int y1, int width, int height, unsigned char *to )
{
	int				y;
	unsigned char	*p=lfb + stride*y1 + x1;

	for(y=0;y<height;y++)
	{
		if (( y+y1<320 ) && ( y+y1>=0 ))
			memcpy(to,p,width);
		else
			memset(to,STEELBLUE,width);
		to+=width;
		p+=stride;
	}
}

void	bg2CopyImage( int x1, int y1, int dx, int dy, unsigned char *src, int dbl )
{
	int				x;
	int				y;
	unsigned char	*d;
	unsigned char	*d1;
	unsigned char	*d2;
	unsigned char	*s;

	if ( !dx || !dy )
		return;
	if ( !dbl )
	{
		for( y=0; (y < dy) && (y+y1<320); y++ )
		{
			for( x=0; (x < dx) && (x+x1<3200) && (y+y1>=0); x++ )
			{
				if ( ( x+x1>=0 ) && *(src+dx*y+x) )
					*(lfb+(y1+y)*stride+x1+x) = *(src+dx*y+x);
			}
		}
		return;
	}
	s=src;
	for( y=0; (y < dy) && (y+y1+y<320); y++ )
	{
		if ( y+y1+y<0 )
			continue;
		d=lfb+(y1+y+y)*stride+x1;
		d2=d+stride;
		d1=d-x1+3200;
		for( x=0; (x < dx) && (d<d1); x++, s++ )
		{
			if ( (x+x1>=0) && *s )
			{
				*d=*s;
				d++;
				*d=*s;
				d++;
			}
			else
			{
				d+=2;
			}
		}
		// line 2
		if ( x )
			memcpy(d2,d-x-x,x+x);
	}
}

static	void	bghLine( int x, int y, int l, unsigned char *c, int clen )
{
	int		sy;
	for( sy=y; sy<=y+clen; sy++ )
		memset(lfb+sy*stride+x,c[sy-y],l);
}

static	void	bgvLine( int x, int y, int l, unsigned char *c, int clen )
{
	int		sx, sy;
	for( sy=y; sy<=y+l; sy++ )
		for( sx=x; sx<x+clen; sx++ )
			*(lfb+sy*stride+sx) = c[sx-x];
}

static	void	inBg( int picid, int ani, int x, int y, int usedbl )
{
	unsigned char	*data;
	int				width;
	int				height;

	data=GetAniPic(picid,ani,&width,&height);
	if ( usedbl & 2 )
	{
		x<<=1;
		y<<=1;
	}
	bg2CopyImage( x, y, width, height, data, usedbl & 1 );
}

static	void	inSc( int picid, int ani, int x, int y, int usedbl )
{
	unsigned char	*data;
	int				width;
	int				height;

	data=GetAniPic(picid,ani,&width,&height);
	if ( usedbl & 2 )
	{
		x<<=1;
		y<<=1;
	}
	if ( usedbl & 1 )
	{
		if (( clip1 || clip2 ) &&
			((x>=clip2+main_x) || (x+width+width<clip1+main_x)))
				return;
		if (( x+width+width<main_x ) ||
			( x>main_x+656 ))		// 656 = 720 - 32 - 32
				return;
	}
	else
	{
		if (( clip1 || clip2 ) &&
			((x>=clip2+main_x) || (x+width<clip1+main_x)))
				return;
		if (( x+width<main_x ) ||
			( x>main_x+656 ))
				return;
	}
	FB2CopyImage( x-main_x+32, y+32, width, height, data, usedbl & 1 );
}

void	DrawLevelIntoBg( void )
{
	int		y;
	int		x;
	int		i;

	switch( level )
	{
	case 1:
	case 4:
		/* stein */
		for( y=0; y<160; y+= 32 )
			for( x=530; x < 1038; x+=32 )
				inBg(10,0,x,y,3);
		/* masken */
		inBg(15,0,530,0,3);
		inBg(16,0,664,28,3);
		inBg(17,0,685,3,3);
		inBg(18,0,996,0,3);
		/* moos */
		inBg(12,0,714,15,3);
		/* boden */
		inBg(13,0,687,68,3);
		inBg(13,0,713,70,3);
		inBg(13,0,739,68,3);
		inBg(13,0,763,71,3);
		inBg(13,0,789,74,3);
		inBg(13,0,815,74,3);
		inBg(13,0,828,74,3);
		inBg(13,0,665,123,3);
		inBg(13,0,691,124,3);
		inBg(13,0,717,124,3);
		for(i=0;i<5;i++)
			inBg(13,0,743+26*i,126,3);
		inBg(13,0,873,127,3);
		inBg(13,0,899,128,3);
		/* aeste */
		inBg(14,0,555,17,3);
		inBg(14,0,573,38,3);
		inBg(14,0,829,11,3);
		/* torleiste */
		inBg(11,0,726,36,3);
		/* tor zu */
		inBg(1,0,1452,78,1);
//		/* haus */
//		inBg(7,0,1742,212,1);
		break;
	}
}

void	DrawInfo( int what )
{
	char	text[64];

	if ( what & 1 )
	{
		sprintf(text,"OUT %d  ",in_level);
		FBDrawString(257,354,30,text,GREEN,STEELBLUE);
	}
	if ( what & 2 )
	{
		sprintf(text,"IN %d%%   ",lem_in*100/lem_cnt);
		FBDrawString(400,354,30,text,GREEN,STEELBLUE);
	}
}

void	InitLevel( void )
{
	char	text[64];
	int		i;

	in_level=0;
	action=0;
	lem_in=0;
	lem_run=0;
	afunc=-1;
	main_x=1320;
	memset(deko,0,sizeof(deko));
	deko[0]=CreateSprite(0,0,main_x+320,160);			// cursor
	deko[0]->anilocked=1;
	switch( level )
	{
	case 1 :
	case 4 :
		/* deko */
		deko[1]=CreateSprite(1,0,1452,78);		// tor
		deko[1]->anilocked=1;
		deko[1]->backlocked=1;
		/* zielhaus */
		deko[2]=CreateSprite(7,0,1742,212);		// haus
		deko[2]->anilocked=1;
		deko[2]->backlocked=1;
		deko[3]=CreateSprite(2,0,1752,216);		// flamme
		deko[3]->backlocked=1;
		deko[4]=CreateSprite(2,3,1804,216);		// flamme
		deko[4]->backlocked=1;
		MirrorSprite( deko[4] );

		/* setup */
		memset(lemm,0,sizeof(lemm));
		memset(portfolio,0,sizeof(portfolio));
		portfolio[7]=10;
		haus_x=1778;
		haus_y1=234;
		haus_y2=270;
		to_rescue=5;
		newspeed=50;
		lem_cnt=10;
		break;
	}
	DrawLevelIntoBg();
	CopyBg2Screen( main_x, 0, 32, 32, 656, 320 );
	SpritesGetBackground();
	DrawSprites();
	/* Draw Menu-Item-Numbers */
	for( i=0; i<8; i++ )
	{
		sprintf(text,"%d",portfolio[i]);
		FBDrawString( 110+i*32,387,16,text,28,0);
	}
	sprintf(text,"%d",newspeed);
	FBDrawString( 46,387,16,text,28,0);
	sprintf(text,"%d",100-newspeed);
	FBDrawString( 78,387,16,text,28,0);
	/* copy level to screen */
	DrawInfo(3);
}

void	AnimateDeko( void )
{
	int		l;

	for( l=0; deko[l]; l++ )
	{
		if ( !deko[l]->anilocked || pic_moved )
		{
			if (( l==1 ) && ( action == 1 ) &&
				( deko[l]->ani == deko[l]->maxani ))
			{
				deko[l]->anilocked=1;
				inBg(1,deko[l]->maxani,1452,78,1);	// store open gate in bg
				action=2;				// send lemmings into level
			}
			SpriteNextPic( deko[l] );
			DrawSprite( deko[l] );
		}
	}
	pic_moved=0;
}

int	InitLemm( void )
{
	int				i;

#ifdef USEX
			FBFlushGrafic();
#endif
	FBFillRect( 0, 0, 720, 576, STEELBLUE );
	FBSetClip( 0, 0, 0, 0 );

	if ( !bgImage )
	{
		FBDrawString(100,100,64,"Initialize...",WHITE,0);
#ifdef USEX
			FBFlushGrafic();
#endif

		if ( LoadPics() == -1 )
			return -1;
		bgImage=malloc(3200*320);
		memset(bgImage,STEELBLUE,3200*320);
	}

	if ( !bgImage )
		return -1;

	for( i=0; i<12; i++ )
		svdimage[i]=malloc(32*48);

	for( i=0; i<320; i+=32 )
		inSc( 19, 0, i+main_x, 352, 1 );

	inSc( 30, 0, 66+main_x, 370, 1 );
	inSc( 30, 1, 100+main_x, 372, 1 );
	inSc( 8, 0, 134+main_x, 372, 0 );		// explosion
	inSc( 4, 10, 166+main_x, 374, 1 );		// lemming2
	inSc( 30, 2, 200+main_x, 368, 1 );
	inSc( 30, 3, 234+main_x, 368, 1 );
	inSc( 30, 4, 264+main_x, 372, 1 );
	inSc( 5, 2, 292+main_x, 368, 1 );

	inSc( 20, 0, 320+main_x, 352, 1 );

	FBDrawString(44,390,48,"-",GREEN,0);
	FBDrawString(76,390,48,"+",GREEN,0);

	FBSetClip( 32, 32, 688, 352 );

	return 0;
}

void	RunKey( void )
{
	Sprite	*s;
static	int		step=2;
static	int		cnt=0;
	int			nac;
	int			sel_type=0;

	if ( realcode == 0xee )
	{
		step=2;
		cnt=0;
		return;
	}

	cnt++;
	if( cnt > 8 )
		step=32;
	else if( cnt > 6 )
		step=16;
	else if( cnt > 4 )
		step=8;
	else if( cnt > 2 )
		step=4;

	switch( actcode )
	{
	case RC_LEFT :
		s=deko[0];
		if ( !s ||
			( s->x < 100 ))
				break;
		if ( s->x == main_x - 12 )	// move screen
		{
			if ( s->y > 304 )
				break;
			main_x -= step;
			FBMove( 32, 32, 32+step, 32, 656-step, 320 );
			CopyBg2Screen( main_x, 0, 32, 32, step, 320 );
			SpriteGetBackground( s );
			pic_moved=1;		// mark for redraw unanimated deko-sprites
			break;
		}
		UndrawSprite( s );
		s->x -= step;
		if ( s->x < main_x - 12 )
			s->x = main_x - 12;
		SpriteGetBackground( s );
		DrawSprite( s );
		break;
	case RC_RIGHT :
		s=deko[0];
		if ( !s ||
			( s->x > 3160 ))
				break;
		if ( s->x == main_x+640 )
		{
			if ( s->y > 304 )
				break;
			main_x += step;
			FBMove( 32+step, 32, 32, 32, 656-step, 320 );
			CopyBg2Screen( main_x+656-step, 0, 688-step, 32, step, 320 );
			SpriteGetBackground( s );
			pic_moved=1;		// mark for redraw unanimated deko-sprites
			break;
		}
		UndrawSprite( s );
		s->x += step;
		if ( s->x > main_x+640 )
			s->x=main_x+640;
		SpriteGetBackground( s );
		DrawSprite( s );
		break;
	case RC_UP :
		s=deko[0];
		if ( s->y == - 12 )
				break;
		UndrawSprite( s );
		s->y -= step;
		if ( s->y < - 12 )
			s->y = - 12;
		SpriteGetBackground( s );
		DrawSprite( s );
		break;
	case RC_DOWN :
		s=deko[0];
		if ( s->y == 304 )
				break;
		UndrawSprite( s );
		s->y += step;
		if ( s->y > 304 )
			s->y=304;
		SpriteGetBackground( s );
		DrawSprite( s );
		break;
	case RC_RED :		// kill all lemmings
		break;
	case RC_BLUE :		// PAUSE - GO
		if ( !action )
			return;
		if ( pause == 1 )
			FBCopyImage( 10*32+32, 384, 32, 48, svdimage[10] );
		pause = pause ? 0 : 1;
		if ( pause )
		{
			FBGetImage( 10*32+32, 384, 32, 48, svdimage[10] );
			FBDrawRect( 10*32+32, 384, 31, 47, BLUE );
			FBDrawRect( 10*32+33, 385, 29, 45, BLUE );
		}
		break;
	case RC_1 :
	case RC_2 :
	case RC_3 :
	case RC_4 :
	case RC_5 :
	case RC_6 :
	case RC_7 :
	case RC_8 :
		if ( pause || !action )
			break;
		nac=actcode-1;
		if ( afunc == actcode-1 )
			break;
		if ( !portfolio[ actcode-1 ] )
			break;
		FBGetImage( (nac+2)*32+32, 384, 32, 48, svdimage[nac+2] ); // save
		if ( afunc != -1 )
			FBCopyImage( (afunc+2)*32+32, 384, 32, 48, svdimage[afunc+2] );
		FBDrawRect( (nac+2)*32+32, 384, 31, 47, YELLOW );
		FBDrawRect( (nac+2)*32+33, 385, 29, 45, YELLOW );
		afunc=nac;
		break;
	case RC_OK :
		if( !action )
		{
			action=1;
			s=deko[1];	// tor;
			s->anilocked=0;
			break;
		}
		if (( action == 1 ) || pause || ( sel_sprite == -1 ))
			break;
		s=lemm[ sel_sprite ];
		if ( !s )
			break;
		if ( afunc == 2 )	// EXPLODE
		{
			s->type |= TYP_EXPLODE;
			portfolio[ afunc ]--;
		}
		else
		{
			switch( afunc )
			{
			case 3 :
				sel_type = TYP_STOPPER;
				break;
			case 7 :
				sel_type = TYP_DIGGER;
				break;
			}
			if ( s->type == TYP_WALKER )
			{
				s->type = sel_type;
				portfolio[ afunc ]--;
				if ( sel_type == TYP_DIGGER )
				{
					SpriteChangePic( s, 5 );	// lemming3
				}
			}
		}
		break;
	}
}

void	RemoveBg( void )
{
	int		i;

	for( i=0; i<10; i++ )
		free(svdimage[i]);

	if ( bgImage )
		free( bgImage );
}

static	void	killlem( int i )
{
	in_level--;
	if ( sel_sprite == i )
		sel_sprite=-1;
	lemm[i]=0;
	if ( !in_level && ( lem_run == lem_cnt ))
	{
		action=3;	// level done
	}
}

void	RunLemm( void )
{
static	int		counter1=0;
static	int		blinkc=0;
	int			i;
	int			b;
	Sprite		*s;
	int			cursor_get=0;

	blinkc++;

	if ( action < 2 )
		return;
	if ( pause )
	{
		if ( blinkc%5 )
			return;
		if ( pause == 1 )
		{
			FBCopyImage( 10*32+32, 384, 32, 48, svdimage[10] );
			pause=2;
		}
		else
		{
			FBDrawRect( 10*32+32, 384, 31, 47, BLUE );
			FBDrawRect( 10*32+33, 385, 29, 45, BLUE );
			pause=1;
		}
		return;
	}

	sel_sprite=-1;

	if (( lem_run < lem_cnt ) && !( counter1%((newspeed/2)+1) ))
	{
		lemm[ lem_run ] = CreateSprite(3,0,1490,78);		// lemming1
		lemm[ lem_run ]->dir=0;		// rechts
		lemm[ lem_run ]->type = TYP_WALKER;
		in_level++;
		lem_run++;
		DrawInfo(1);
	}
	if ( in_level )
		UndrawSprite( deko[0] );
	for( i=0; i<lem_run; i++ )
	{
		if ( !lemm[i] )
			continue;
		UndrawSprite( lemm[i] );
		SpriteNextPic( lemm[i] );
	}
	DrawSprite( deko[2] );		// ziel
	for( i=0; i<lem_run; i++ )
	{
		if ( !lemm[i] )
			continue;

		s=lemm[i];

		if ( !( counter1 % 10 ) && ( s->type & TYP_EXPLODE ) )
			s->countdown--;

		if ( (s->countdown>0)&&!(s->type&TYP_FALLEN) )
		{
			if ( s->type&TYP_ATHOME )
			{
				s->y-=2;
				if((s->ani==4)||(s->ani==8))
				{
					s->counter2++;
					if ( s->counter2==2 )
					{
						lem_in++;
						killlem(i);
						DrawInfo(3);
					}
				}
			}
			else
			{	/* lemming im ziel ? */
				if((s->x==haus_x)&&(s->y>haus_y1)&&
					(s->y<haus_y2))
				{
					s->type=TYP_ATHOME;
					s->counter2=0;
					SpriteChangePic( s, 6 );	// lemming4
				}
				else
				{	/* kein bodenkontakt ? */
					if(!isBrick(s->x+4,s->y+s->height*2)&&
					   !isBrick(s->x-6+s->width*2,
								s->y+s->height*2))
					{
						if( !( s->type&TYP_WALKER ) )
						{
							s->type=TYP_WALKER|(s->type&TYP_EXPLODE);
							SpriteChangePic( s, 3 );	// lemming1
							if ( s->dir )
								MirrorSprite( s );
						}
						// freier fall
						s->y += 4;
						s->counter2++;
						// aus bild gefallen
						if(s->y>=320)
						{
							killlem(i);
						}
					}
					else
					{
						if(s->type&TYP_WALKER)
						{	/* aufgeschlagen */
							if(s->counter2>=40)
							{
								s->type=TYP_FALLEN;
							}
							else
							{	/* wieder auf boden */
								s->counter2=0;
								/* laeufer - getestet wird oben */
								if((isBrick(s->x,s->y)&&
									(s->dir==1))||
								   (isBrick(s->x+s->width*2,s->y)&&
									(s->dir==0)))
								{
									MirrorSprite( s );
									s->dir^=1;
								}
								else
								{
									s->x += (s->dir?-2:2);
									if ( s->dir )
									{
										for(b=16;b>0;b-=2)
										{
											if(isBrick(s->x,
												s->y+s->height*2-b))
											{
												s->y-=2;
												b=0;
											}
										}
									}
									else
									{
										for(b=16;b>0;b-=2)
										{
											if(isBrick(s->x+s->width*2,
												s->y+s->height*2-b))
											{
												s->y-=2;
												b=0;
											}
										}
									}
								}
							} /* else, kein matsch */
						} /** walker **/
						if(s->type&TYP_DIGGER)
						{
							if(!(counter1%8))
							{
								if(s->y<320)
								{
									unsigned char	c[26];
									memset(c,STEELBLUE,26);
#define max(a,b)	((a)>(b)?(a):(b))
									bghLine( s->x+4, s->y, 16,c,
										24-max(0,s->y-274));
									s->y+=2;
									cursor_get=1;
								}
							}
						}	/* digger */
					}	/* freier fall */
				}	/* nicht am ziel */
			}	/* countdown */
		}	/* typ-fallen */

		if ( !lemm[i] )
			continue;

		SpriteGetBackground( s );
		DrawSprite( s );
		if (( afunc != -1 ) && ( portfolio[ afunc ] ))
		{
			if ( SpriteCollide( s, deko[0]->x+14, deko[0]->y+14 ) )
			{
				sel_sprite=i;
			}
		}
	}
	if ( cursor_get )
		SpriteGetBackground( deko[0] );
	if ( sel_sprite != -1 )
	{
		SpriteSelPic( deko[0], 1 );
	}
	else
	{
		SpriteSelPic( deko[0], 0 );
	}
	DrawSprite( deko[0] );
	counter1++;
}

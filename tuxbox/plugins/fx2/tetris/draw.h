#ifndef DRAW_H
#define DRAW_H

extern	int		FBInitialize( int xRes, int yRes, int bpp, int extfd );
extern	void	FBClose( void );
extern	void	FBPaintPixel( int x, int y, unsigned char col );
extern	void	FBDrawLine( int xa, int ya, int xb, int yb, unsigned char col );
extern	void	FBDrawHLine( int x, int y, int dx, unsigned char col );
extern	void	FBDrawVLine( int x, int y, int dy, unsigned char col );
extern	void	FBFillRect( int x, int y, int dx, int dy, unsigned char col );
extern	void	FBDrawRect( int x, int y, int dx, int dy, unsigned char col );
extern	void	FBCopyImage( int x, int y, int dx, int dy, unsigned char *src );
extern	void	FBOverlayImage(int x, int y, int dx, int dy, int relx, int rely,
					unsigned char c1,
					unsigned char *src,
					unsigned char *under,
					unsigned char *right,
					unsigned char *bottom );
extern	void	FBCopyImageCol( int x, int y, int dx, int dy, unsigned char col,
					unsigned char *src );
extern	void	FBBlink( int x, int y, int dx, int dy, int count );
extern	void	FBMove( int x, int y, int x2, int y2, int dx, int dy );
extern	void	write_xpm( void );

/* about colors */

#ifdef MY16BIT_COLORS
#define BLUE_MASK		0x001f		/* 5bits */
#define GREEN_MASK		0x07e0		/* 6bits */
#define RED_MASK		0xf800		/* 5bits */

#define P5(a)			((a<<4/255)&0x01f)
#define P6(a)			((a<<5/255)&0x03f)
#define	RGB(r,g,b)		((P5(r)<<11)|(P6(g)<<5)|P5(b))

#define	BLACK			0x0000
#define	YELLOW			0x07ff
#define	WHITE			0xffff
#define	GREEN			0x07e0
#define	RED				0xf800
#define	STEELBLUE		RGB(70,130,180)
#endif

#define BNR0			0
#define BLACK			1
#define	WHITE			2
#define	YELLOW			3
#define	GREEN			4
#define	RED				5
#define	STEELBLUE		6
#define	BLUE			7
#define	GRAY			8
#define	DARK			9
#define	RED1			10
#define	RED2			11

#endif

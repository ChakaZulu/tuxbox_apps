
typedef struct _Sprite
{
	unsigned char	*ori_data;
	unsigned char	*flip_data;
	unsigned char	*data;
	int				width;
	int				height;
	int				oldx;
	int				oldy;
	int				x;
	int				y;
	int				sz;
	int				countdown;
	int				counter2;
	struct _Sprite	*next;
	struct _Sprite	*pre;
	char			type;
	char			picid;
	char			anilocked;
	char			backlocked;
	char			dir;
	char			ani;
	char			maxani;
} Sprite;

extern	int		SpriteCollide( Sprite *s, int x, int y );
extern	Sprite	*CreateSprite( int picid, int ani, int x, int y );
extern	void	MirrorSprite( Sprite *s );
extern	void	SpriteNextPic( Sprite *s );
extern	void	SpriteSelPic( Sprite *s, int ani );
extern	void	DrawSprite( Sprite *s );
extern	void	FreeSprites( void );
extern	void	SpritesGetBackground( void );
extern	void	SpriteGetBackground( Sprite *s );
extern	void	DrawSprites( void );
extern	void	SpriteChangePic( Sprite *s, int picid );

#define TYP_WALKER		2
#define	TYP_EXPLODE		4
#define	TYP_STOPPER		8
#define	TYP_DIGGER		16
#define TYP_ATHOME		32
#define TYP_FALLEN		64

/*
** initial coding by fx2
*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <rcinput.h>
#include <draw.h>

static	int				fd = -1;
		unsigned short	actcode=0xee;
		int				doexit=0;
		int				debug=0;

#define Debug	if(debug)printf

#ifndef i386

#include <dbox/fp.h>

static	int		fd_is_ext = 0;

int	RcInitialize( int extfd )
{
	char	buf[32];

	if ( extfd == -1 )
	{
		fd_is_ext = 0;
		fd = open( "/dev/dbox/rc0", O_RDONLY );
		if ( fd == -1 )
		{
			perror("failed - open /dev/dbox/rc0");
			return(-1);
		}
		fcntl(fd, F_SETFL, O_NONBLOCK );
		ioctl(fd, RC_IOCTL_BCODES, 1);
	}
	else
	{
		fd_is_ext = 1;
		fd = extfd;
		fcntl(fd, F_SETFL, O_NONBLOCK );
	}

/* clear rc-buffer */
	read( fd, buf, 32 );

	return 0;
}

static	unsigned short translate( unsigned short code )
{
	if ((code&0xFF00)==0x5C00)
	{
		switch (code&0xFF)
		{
		case 0x0C: return RC_STANDBY;
		case 0x20: return RC_HOME;
		case 0x27: return RC_SETUP;
		case 0x00: return RC_0;
		case 0x01: return RC_1;
		case 0x02: return RC_2;
		case 0x03: return RC_3;
		case 0x04: return RC_4;
		case 0x05: return RC_5;
		case 0x06: return RC_6;
		case 0x07: return RC_7;
		case 0x08: return RC_8;
		case 0x09: return RC_9;
		case 0x3B: return RC_BLUE;
		case 0x52: return RC_YELLOW;
		case 0x55: return RC_GREEN;
		case 0x2D: return RC_RED;
		case 0x54: return RC_PAGE_UP;
		case 0x53: return RC_PAGE_DOWN;
		case 0x0E: return RC_UP;
 		case 0x0F: return RC_DOWN;
		case 0x2F: return RC_LEFT;
 		case 0x2E: return RC_RIGHT;
		case 0x30: return RC_OK;
 		case 0x16: return RC_PLUS;
 		case 0x17: return RC_MINUS;
 		case 0x28: return RC_SPKR;
 		case 0x82: return RC_HELP;
		default:
			//perror("unknown old rc code");
			return 0xee;
		}
	} else if (!(code&0x00))
		return code&0x3F;
	return 0xee;
}

void		RcGetActCode( void )
{
	char			buf[32];
	int				x;
	unsigned short	code = 0;
static  unsigned short cw=0;

	x = read( fd, buf, 32 );
	if ( x < 2 )
		return;

	Debug("%d bytes from FB received ...\n",x);

	x-=2;

	memcpy(&code,buf+x,2);

	code = translate(code);

	if ( code == 0xee )
		return;

	Debug("code=%04x\n",code);

	switch(code)
	{
	case RC_HELP:
		if ( !cw )
			FBPrintScreen();
		cw=1;
		break;
	case RC_UP:
	case RC_DOWN:
	case RC_RIGHT:
	case RC_LEFT:
	case RC_OK:
		cw=0;
		actcode=code;
		break;
	case RC_HOME:
		doexit=3;
		break;
	}

	return;
}

void	RcClose( void )
{
	if ( !fd_is_ext )
		close(fd);
}

#else		/* i386 */

#include <termios.h>

static	struct termios	tios;

int	RcInitialize( int extfd )
{
	struct termios	ntios;
	fd = 0;

	tcgetattr(fd,&tios);
	memset(&ntios,0,sizeof(ntios));
	tcsetattr(fd,TCSANOW,&ntios);

	return 0;
}

static	unsigned short translate( unsigned char c )
{
	switch(c)
	{
	case 0x41 :
		return RC_UP;
	case 0x42 :
		return RC_DOWN;
	case 0x43 :
		return RC_RIGHT;
	case 0x44 :
		return RC_LEFT;
	}
	return 0;
}

void		RcGetActCode( void )
{
	unsigned char	buf[256];
	int				x;
	int				left;
	unsigned short	code = 0;
	unsigned char	*p = buf;

	x = read(fd,buf,256);
	if ( x>0 )
	{
		for(p=buf, left=x; left; left--,p++)
		{
			switch(*p)
			{
			case 0x1b :
				if ( left >= 3 )
				{
					p+=2;
					code = translate(*p);
					if ( code )
						actcode = code;
					left-=2;
				}
				else
					left=1;
				break;
			case 0x03 :
				doexit=3;
				break;
			case 0x0d :
				actcode=RC_OK;
				break;
			case 0x1c :
				FBPrintScreen();
				break;
			}
		}
	}
}

void	RcClose( void )
{
	tcsetattr(fd,TCSANOW,&tios);

	close(fd);
}

#endif

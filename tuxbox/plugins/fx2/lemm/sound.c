#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <sounds.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <unistd.h>

static	unsigned char	*sounds[6] =
{ snd_die, snd_door, snd_oing, snd_ohno, snd_explode, snd_letsgo };
static	int	ssz[6] =
{ 5708, 5360, 4084, 5621, 1364, 9929 };

static	int				sound_fd=-1;
static	pthread_t		pth;
static	int				play[20];
static	int				playidx=0;
static	pthread_cond_t	cond;
static	pthread_mutex_t	mutex;

static	void	_run_sound( void *ptr )
{
	int				sz;
	int				i;
	unsigned char	*data;
	struct timeval	tv;

	while( 1 )
	{
		tv.tv_usec=50000;
		tv.tv_sec=0;
		select( 0,0,0,0,&tv);
		if ( !playidx )
			continue;
		i=play[playidx-1];
		playidx--;
		if ( i < 0 )
			break;

		data=sounds[i];
		sz=ssz[i];

		while( sz )
		{
		   i=sz>1022?1022:sz;
		   write(sound_fd, data, i);
		   sz-=i;
		   data+=i;
		}

		i=1;
		if ( !playidx )
			ioctl(sound_fd,SNDCTL_DSP_SYNC,&i);
	}
	if ( sound_fd >= 0 )
		close( sound_fd );
	return;
}

void	SoundStart( void )
{
	int					rc;
	int format = AFMT_S8;	// signed, 8 Bit
	int channels = 1;	// 1=mono, 2=stereo
	int speed = 11025;

	if ( sound_fd == -1 )
		sound_fd=open("/dev/dsp",O_WRONLY);
	if ( sound_fd == -1 )
		sound_fd = -2;

	if ( sound_fd == -2 )
		return;

	ioctl(sound_fd, SNDCTL_DSP_SETFMT, &format);
	ioctl(sound_fd, SNDCTL_DSP_CHANNELS, &channels);
	ioctl(sound_fd, SNDCTL_DSP_SPEED, &speed);

	memset(&pth,0,sizeof(pth));
	pthread_cond_init (&cond, NULL);
	pthread_mutex_init (&mutex, NULL);

	playidx=0;
	rc=pthread_create( &pth, NULL, _run_sound, 0 );
}

void	SoundPlay( int pnr )
{
	if ( sound_fd==-2)
		return;
	if ( playidx == 20 )
		return;
	play[ playidx ]=pnr;
	playidx++;
}

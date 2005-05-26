/*
 * Tool for testing all possible PCM-stream parameters
 *
 * $Id: audioplay.c,v 1.2 2005/05/26 19:20:39 carjay Exp $
 *
 * Copyright (C) 2004 Carsten Juttner <carjay@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/soundcard.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define OSS_DSP_DEVNAME "/dev/sound/dsp"

int setupdsp(unsigned char size,unsigned char sign, unsigned char bigendian,unsigned int speed, unsigned int channels){
	int dsp, format;
	if (sign) {
		if (size==8)
			format = AFMT_S8;
		else if (bigendian)
			format = AFMT_S16_BE;
		else
			format = AFMT_S16_LE;
	} else {
		if (size==8)
			format = AFMT_U8;
		else if (bigendian)
			format = AFMT_U16_BE;
		else
			format = AFMT_U16_LE;
	}

	if((dsp = open(OSS_DSP_DEVNAME, O_WRONLY)) == -1)
	{
		perror("could not open dsp-device");
		return -1;
	}

	if(ioctl(dsp, SNDCTL_DSP_SETFMT, &format) == -1)
	{
		perror("could not set format");
		close(dsp);
		return -1;
	}

	if(ioctl(dsp, SNDCTL_DSP_CHANNELS, &channels) == -1)
	{
		perror("could not set channels");
		close(dsp);
		return -1;
	}

	if(ioctl(dsp, SNDCTL_DSP_SPEED, &speed) == -1)
	{
		perror("could not set speed");
		close(dsp);
		return -1;
	}
	return dsp;
}
	
int play(int dsp, char *audiodata, unsigned int bufsize){
	ssize_t read=0;

	while(read < bufsize)
	{
		int add;
		add=write(dsp, audiodata+read, bufsize-read);
		if (add>0)
			read+=add;
		else if (errno!=EAGAIN){
			fprintf(stderr,"could not write PCM data: %s\n",strerror(errno));
			return -1;
		}
	}
	return 0;
}
	
void usage (char *s)
{
	printf ("Audioplay V1.0 - OSS pcm playback tool\n"
			"Usage: %s <pcmfile> <format> <speed> <channels>\n"
			 "\tformat: u8,ube16,ule16,s8,sbe16,sle16\n"
			"\tspeed: 8000,11025,12000,16000,22050\n\t\t24000,32000,44100,48000\n"
			"\tchannels: 1 or 2\n\n",s);
	exit(0);
}

int main (int argc, char **argv)
{
	int dsp;
	unsigned char bigendian=0,sign=1,size=16,channels;
	unsigned int speed;
	char *fname=argv[1];
	unsigned char buffer[1024*128];
	
	FILE *f;
	size_t read;
	
	if (argc !=5) 
		usage(argv[0]);

	if (!strcmp(argv[2],"s8")){
		size = 8;
		sign = 1;
	} else if (!strcmp(argv[2],"u8")){
		size = 8;
		sign = 0;
	} else if (!strcmp(argv[2],"sbe16")){
		sign = 1;
		bigendian = 1;
	} else if (!strcmp(argv[2],"sle16")){
		sign = 1;
		bigendian = 0;
	} else if (!strcmp(argv[2],"ube16")){
		sign = 0;
		bigendian = 1;
	} else if (!strcmp(argv[2],"ule16")){
		sign = 0;
		bigendian = 0;
	} else {
		fprintf (stderr,"unknown format: %s\n",argv[2]);
		usage(argv[0]);
	}
	speed = atoi(argv[3]);
	if ((speed!=8000)&&(speed!=11025)&&(speed!=12000)&&(speed!=16000)&&(speed!=22050)&&
					(speed!=24000)&&(speed!=32000)&&(speed!=44100)&&(speed!=48000)){
		fprintf (stderr,"unknown speed: %d\n",speed);
		usage(argv[0]);
	}

	channels=atoi(argv[4]);
	if (channels>2){
		fprintf (stderr,"invalid number of channels: %d\n",atoi(argv[4]));
		usage(argv[0]);
	}
	
	f=fopen(fname, "rb");
	if (!f){
		perror ("error opening audiofile");
		exit(1);
	}

	if ((dsp=setupdsp(size,sign,bigendian,speed,channels))<0){
		fprintf (stderr,"error setting up dsp\n");
		fclose (f);
		exit (1);
	}
	while ((read=fread(buffer,1,sizeof(buffer),f))){
		if (read!=sizeof(buffer))
			memset (buffer,0x00,sizeof(buffer)-read);
		if (play(dsp,buffer,sizeof(buffer))<0) break;
	}
	fclose (f);
	close(dsp);
	return 0;	
}

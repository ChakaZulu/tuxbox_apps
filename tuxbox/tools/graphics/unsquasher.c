/******************************************************************************
 *	Unsquasher - unsquasher.c
 *                                                                            
 *	Turns the proprietary image format of the GTX/eNX to YUY2
 *
 *	(c) 2003 Carsten Juttner (carjay@gmx.net)
 *  									      
 * 	This program is free software; you can redistribute it and/or modify
 * 	it under the terms of the GNU General Public License as published by
 * 	The Free Software Foundation; either version 2 of the License, or
 * 	(at your option) any later version.
 *
 * 	This program is distributed in the hope that it will be useful,
 * 	but WITHOUT ANY WARRANTY; without even the implied warranty of
 * 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * 	GNU General Public License for more details.
 *
 * 	You should have received a copy of the GNU General Public License
 * 	along with this program; if not, write to the Free Software
 * 	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ******************************************************************************
 * $Id: unsquasher.c,v 1.2 2003/11/14 18:07:28 carjay Exp $
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <error.h>

char squashdat[16] = {
	0,2,8,				// added
	0x1c,0x30,0x44,0x58,0x6c,0x80,	// constant value
	0x94,0xa8,0xbc,0xd0,0xe4,
	-8,-2				// added
};

void unsquash (char *inbuf, char *outbuf, int length){
	unsigned char y=0x10;
	char chromastore[2];
	char *pcomp = chromastore;
	unsigned char pt = 0;
	chromastore [0]=chromastore[1]=0x80;	// initial settings
	while (length--){
		char sqf,sqs;
		unsigned char first,second;
		unsigned char tmp = *inbuf++;
		first = (tmp>>4)&0x0f;
		second = tmp&0x0f;
		sqf = squashdat[first];
		sqs = squashdat[second];
		if (first<=0x02||first>=0x0e)  pcomp[pt] += sqf;
		else pcomp[pt] = sqf + (pcomp[pt]&0x01);
		if (second<=0x02||second>=0x0e) y += sqs;
		else y = sqs + (y&0x01);
		if (y<16) y=16;	
		if (y>235) y=235;
		if ((unsigned char)(pcomp[pt])<16) pcomp[pt]=16;
		if ((unsigned char)(pcomp[pt])>240) pcomp[pt]=240;
		*outbuf++ = y;
		*outbuf++ = pcomp[pt];
		pt^=1;
	}
}




void usage(char *name){
	printf ("Usage: %s <input filename> <output filename>\n"
		"       input file as read from /dev/v4l/video0\n"
		"       output file contains pixel data in YUY2-format\n\n"
		"       expects that the the first two 16-bit (BE) numbers\n"
		"        of the input file are width and height and will apply\n"
		"        this header to the output file as well.\n"
		"       line length equals to bytes per scan line\n"
		"Example: %s in.syuv out.yuy2\n",name,name);
}

int main (int argc, char **argv){
	char *infile, *outfile;
	int ifd, ofd;
	unsigned char temp;
	int linelength, lines;
	struct stat finfo;
	unsigned char *inbuf, *outbuf;
	unsigned char header[6];	
	off_t imagesize;
	
	if (argc!=3){
		usage(argv[0]);
		return 1;
	}

	infile = argv[1];
	outfile = argv[2];

	ifd = open (infile,O_RDONLY);
	if (ifd<0){
		perror ("could not open infile");
		return 1;
	}

	ofd = open (outfile, O_RDWR|O_CREAT|O_TRUNC,00644);
	if (ofd<0){
		perror ("could not open outfile");
		close (ifd);
		return 1;
	}

	read(ifd,&temp,1);
	linelength = temp<<8;
	read(ifd,&temp,1);
	linelength |= temp&0xff;
	lseek (ifd, 4, SEEK_CUR);
	
	if ((linelength<0)||(linelength>720)){
		printf ("invalid linelength %d",linelength);
		return 1;
	}

	inbuf = malloc (linelength);
	if (!inbuf) {
		perror ("out of memory");
		close (ofd);
		close (ifd);
		return 1;
	}
	outbuf = malloc (linelength*2);
	if (!outbuf) {
		perror ("out of memory");
		free (inbuf);
		close (ofd);
		close (ifd);
		return 1;
	}

	fstat (ifd, &finfo);
	imagesize = finfo.st_size-sizeof(header);
	lines = imagesize / linelength;
	if (!lines){
		perror ("linesize bigger than file");
		free (inbuf);
		free (outbuf);
		close (ofd);
		close (ifd);
		return 1;
	}
	if (imagesize%linelength){
		printf ("Warning: file does not fit to linelength, %ld excess bytes\n", imagesize%linelength);
	}
	
	printf ("unsquashing %d lines now\n",lines);
	
	header[0]=(linelength&0xff00)>>8;	// write bigendian header
	header[1]=(linelength&0xff);
	header[2]=(lines&0xff00)>>8;
	header[3]=(lines&0xff);
	header[4]=0x00;		// reserved for now
	header[5]=0x00;
	write (ofd, header, sizeof(header));
	while (lines--){
		read (ifd, inbuf, linelength);
		unsquash (inbuf,outbuf,linelength);
		write (ofd, outbuf, linelength*2);
	}
	
	free (inbuf);
	free (outbuf);
	close (ifd);
	close (ofd);
	return 0;
}

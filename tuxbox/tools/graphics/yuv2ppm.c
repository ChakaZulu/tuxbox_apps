/******************************************************************************
 *	yuv2ppm - yuv2ppm.c
 *                                                                            
 *	Quick and dirty transcoder for the yuy2-data to rgb as a ppm-file
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
 * $Id: yuv2ppm.c,v 1.3 2003/11/22 23:54:37 carjay Exp $
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <error.h>

// this sucks with the dbox2
// LUTs and/or fixed point should speed it up
static inline void matrix(unsigned char y, unsigned char cb, unsigned char cr,
				unsigned char *r, unsigned char *g, unsigned char *b){
/*
	The (normalized to 1)-RGB vector is multiplied to the following matrix
 	| 0.299    0.587    0.114|   |R|
  	| 0.701   -0.587   -0.114| * |G|
  	|-0.299   -0.587    0.886|   |B|

	Rec.601 limits set a head- and footroom of 
		16..235 for Y
		16..240 for Cr/Cb (16=-1.0, 128=0, 240=1.0)
	The squasher should only produce values in that range (sometimes it doesn't though?!)
	
	The inverted matrix is:
 	| 1.000    1.000    0.000|   | Y|
  	| 1.000   -0.509   -0.194| * |Cr|
  	| 1.000    0.000    1.000|   |Cb|

	We clearly see that Cr is R-Y and Cb is B-Y. BTW, Cb and Cr are not really U and V
	because for PAL U and V are multiplied by an additional reduction-factor to avoid
	overmodulating the CVBS-signal.
	
	RGB-values are from 0..255, so we need to scale this back accordingly.
	Since the hardware does not do gamma correction we unscale linearly.
	
	subtract 128.5 from Cb/Cr and divide by 112 to get the factor
	subtract 16.5 from Y and divide by 219 to get the factor
*/
		double _Y,_Cr,_Cb;
		double _R,_G,_B;


		_Y = (y-16.5)/219.0;
		_Cr = (cr-128.5)/112.0;
		_Cb = (cb-128.5)/112.0;
		_R = (_Y	+ _Cr 				)*255.0;
		_G = (_Y	+ (-0.509*_Cr)	+ (-0.194*_Cb)	)*255.0;
		_B = (_Y			+ _Cb		)*255.0;
		if (_R<0) _R=0;		// clipping is necessary to compensate for 
		if (_G<0) _G=0;		//	rounding/quantization errors
		if (_B<0) _B=0;
		*r = (_R>255.0)? 255 : _R;
		*g = (_G>255.0)? 255 : _G;
		*b = (_B>255.0)? 255 : _B;
}

// length in bytes
void yuv2rgb (unsigned char *in, unsigned char *out, int length){
	unsigned char r,g,b,y1,y2,u,v;
	length>>=2;		// 2 byte = 2 * luma and 1 * chroma information
	while (length--){
		y1 = *in++;
		u  = *in++;
		y2 = *in++;
		v  = *in++;
		matrix (y1,u,v,&r,&g,&b);
		*out++=r;
		*out++=g;
		*out++=b;
		matrix (y2,u,v,&r,&g,&b);
		*out++=r;
		*out++=g;
		*out++=b;
       	}
}


void usage (char *name){
	printf ("Usage: %s <input filename> <output filename>\n\n",name);
}

int main (int argc, char **argv){
	int ifd, ofd;
	int linelength, lines;
	struct stat finfo;
	unsigned char *inbuf, *outbuf;
	unsigned char header[6];
	unsigned char ppmheader[64];

	if (argc!=3){
		usage(argv[0]);
		return 1;
	}

	ifd = open (argv[1], O_RDONLY);
	if (ifd<0){
		perror ("Opening input file");
		return 1;
	}
	ofd = open (argv[2], O_RDWR|O_CREAT|O_TRUNC, 00644);
	if (ofd<0){
		perror ("Opening output file");
		close (ifd);
		return 1;
	}
	
	read (ifd,header,sizeof (header));
	
	linelength = ((header[0]<<8)|header[1])<<1;
	lines = ((header[2]<<8)|header[3]);
	
	fstat (ifd,&finfo);
	if ((lines*linelength) != (finfo.st_size-sizeof(header))){
		printf ("header/filesize-mismatch: %dx%d should be %d bytes, not %ld + %d header bytes\n",
					linelength>>1, lines, linelength>>1*lines, (finfo.st_size-sizeof(header)), sizeof(header));
		close (ifd);
		close (ofd);
		return 1;
	}
		
	inbuf = malloc (linelength);
	outbuf = malloc ((linelength*3)/2);
		
	printf ("converting %d lines\n",lines);
	
	sprintf (ppmheader, "P6\n%d\n%d\n255\n",linelength>>1,lines);	
	// TODO: the ppm format seems not to convey the linearity (our RGB-values are non-gamma-corrected)
	write (ofd, ppmheader, strlen(ppmheader));
	while (lines--){
		read (ifd, inbuf, linelength);
		yuv2rgb (inbuf,outbuf,linelength);
		write (ofd, outbuf, (linelength*3)/2);
	}
	
	free (inbuf);
	free (outbuf);
	close (ofd);
	close (ifd);
	return 0;
}

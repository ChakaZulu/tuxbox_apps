/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2005 Zwen

	Kommentar:
	This is an implementation of the floyd steinberg error diffusion algorithm
	adapted for 24bit to 15bit color reduction.

	For a description of the base alorithm see e.g.: 
	http://www.informatik.fh-muenchen.de/~schieder/graphik-01-02/slide0264.html
	
	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <malloc.h>
#include "pictureviewer.h"

void c32_15(unsigned char r, unsigned char g , unsigned char b , unsigned char* d)
{
		*d     = ((r >> 1) & 0x7C) | (g >> 6);
		*(d+1) = ((g << 2) & 0xE0) | (b >> 3);
}

#define FS_CALC_ERROR(color, index) \
				p1 = p2 = (ptr[index] + (this_line_error_##color[ix]>>4)); \
				if(p1>255)p1=255; if(p1<0)p1=0; \
				color = (p1 & 0xF8) | 0x4; \
				error = p2 - color; \
				if(error > 255)error=255;if(error<-255)error=-255; \
				this_line_error_##color[ix-1] += (error * 7); \
				next_line_error_##color[ix+1] += (error * 3); \
				next_line_error_##color[ix]   += (error * 5); \
				next_line_error_##color[ix-1] += error;
				

unsigned char * make15color_errdiff(unsigned char * orgin,int x, int y)
{
	int odd_line=1;
	int ix,iy, error, p1, p2;
	unsigned char r,g,b;
	unsigned char *ptr, *dst;
	unsigned char* np = (unsigned char*)malloc(x*y*2);
	int *this_line_error_r;
	int *this_line_error_g;
	int *this_line_error_b;
	int *next_line_error_r;
	int *next_line_error_g;
	int *next_line_error_b;
	int *save_error_r;
	int *save_error_g;
	int *save_error_b;
	int *error1_r = (int*)malloc((x+2)*sizeof(int));
	int *error1_g = (int*)malloc((x+2)*sizeof(int));
	int *error1_b = (int*)malloc((x+2)*sizeof(int));
	int *error2_r = (int*)malloc((x+2)*sizeof(int));
	int *error2_g = (int*)malloc((x+2)*sizeof(int));
	int *error2_b = (int*)malloc((x+2)*sizeof(int));
	
	dbout("Start error diffusion\n");
	
	this_line_error_r = error1_r;
	this_line_error_g = error1_g;
	this_line_error_b = error1_b;
	next_line_error_r = error2_r;
	next_line_error_g = error2_g;
	next_line_error_b = error2_b;
	memset (this_line_error_r, 0 , (x+2) * sizeof(int));
	memset (this_line_error_g, 0 , (x+2) * sizeof(int));
	memset (this_line_error_b, 0 , (x+2) * sizeof(int));
	memset (next_line_error_r, 0 , (x+2) * sizeof(int));
	memset (next_line_error_g, 0 , (x+2) * sizeof(int));
	memset (next_line_error_b, 0 , (x+2) * sizeof(int));
	ptr = orgin;
	dst = np;

	for(iy=0 ; iy < y ; iy++)
	{
		save_error_r = this_line_error_r;
		this_line_error_r = next_line_error_r;
		next_line_error_r = save_error_r;
		save_error_g = this_line_error_g;
		this_line_error_g = next_line_error_g;
		next_line_error_g = save_error_g;
		save_error_b = this_line_error_b;
		this_line_error_b = next_line_error_b;
		next_line_error_b = save_error_b;
		memset (next_line_error_r, 0 , (x+2) * sizeof(int));
		memset (next_line_error_g, 0 , (x+2) * sizeof(int));
		memset (next_line_error_b, 0 , (x+2) * sizeof(int));
		
		if(odd_line)
		{
			for(ix=1 ; ix <= x ; ix++)
			{
				FS_CALC_ERROR(r,0);
				FS_CALC_ERROR(g,1);
				FS_CALC_ERROR(b,2);
				c32_15(r,g,b,dst);
				ptr+=3;
				dst+=2;
			}
			odd_line=0;
		}
		else
		{
			ptr+=(x-1)*3;
			dst+=(x-1)*2;
			for(ix=x ; ix >= 1 ; ix--)
			{
				FS_CALC_ERROR(r,0);
				FS_CALC_ERROR(g,1);
				FS_CALC_ERROR(b,2);
				c32_15(r,g,b,dst);
				ptr-=3;
				dst-=2;
			}
			ptr+=x*3;
			dst+=x*2;
			odd_line=1;
		}
	}
	free(error1_r);
	free(error1_g);
	free(error1_b);
	free(error2_r);
	free(error2_g);
	free(error2_b);
	dbout("End error diffusion\n");
	return np;
}


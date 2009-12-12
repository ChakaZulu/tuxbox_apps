/*
 * $Id: lcd.h,v 1.1 2009/12/12 18:42:44 rhabarber1848 Exp $
 *
 * shellexec - d-box2 linux project
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
*/

#ifndef __lcddisplay__
#define __lcddisplay__

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include <dbox/lcd-ks0713.h>

enum
	{
	PIXEL_ON = LCD_PIXEL_ON,
	PIXEL_OFF = LCD_PIXEL_OFF,
	PIXEL_INV =	LCD_PIXEL_INV
	};
		
	int LCD_Init(void);
	void LCD_Clear(void);
	int LCD_Close(void);

	int LCD_invalid_col(int x);
	int LCD_invalid_row(int y);
	void LCD_convert_data();
	int LCD_sgn(int arg);
	void LCD_setIconBasePath(char *base);
	void LCD_update();

	void LCD_draw_point (int x,int y, int state);
	void LCD_draw_line (int x1, int y1, int x2, int y2, int state);
	void LCD_draw_fill_rect (int left,int top,int right,int bottom,int state);
	void LCD_draw_rectangle (int left,int top, int right, int bottom, int linestate,int fillstate);
	void LCD_draw_polygon(int num_vertices, int *vertices, int state);
	void LCD_draw_char(int x, int y, char c);
	void LCD_draw_string(int x, int y, char *string);

	void LCD_paintIcon(unsigned char *filename, int x, int y, int col);

#endif

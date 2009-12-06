/*
 * $Id: lcd.c,v 1.1 2009/12/06 13:40:32 rhabarber1848 Exp $
 *
 * lcshot - d-box2 linux project
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

#include "lcd.h"

unsigned char raw[132][64];
static unsigned char lcd[LCD_ROWS][LCD_COLS];
static int fd=-1;

void LCD_Read(void)
{
int x,y,z;
char tmp2;

	memset(raw,0,132*64);
	read(fd, &lcd, 120*64/8);
	for(x=0;x < LCD_COLS;x++) 
	{   
		for(y=0;y < LCD_ROWS; y++) 
		{
			tmp2 = lcd[y][x];
			for(z=0;z <= 7;z++) 
			{
				if(tmp2 & (1<<z))
					raw[x][y * 8 + z] = 1;
			}
		}
	}
}


int LCD_Init(void)
{
	//open device
	if((fd = open("/dev/dbox/lcd0",O_RDWR)) < 0)
	{
		perror("LCD (/dev/dbox/lcd0)");
		return -1;
	}
	
	LCD_Read();
	return 0;
}

int LCD_Close(void)
{
	if(fd>=0)
	{
		close(fd);
		fd=-1;
	}
	return 0;
}


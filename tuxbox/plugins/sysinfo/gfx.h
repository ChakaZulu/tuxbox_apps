/*
 * $Id: gfx.h,v 1.1 2009/12/20 16:22:58 rhabarber1848 Exp $
 *
 * sysinfo - d-box2 linux project
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

#ifndef __GFX_H__

#define __GFX_H__

void RenderBox(int sx, int sy, int ex, int ey, int mode, int color);
void RenderLine( int xa, int ya, int xb, int yb,int width, int farbe );
void RenderHLine( int sx, int sy, int ex, char dot, char width, char spacing, int farbe );
void RenderVLine( int sx, int sy, int ey, char dot, char width, char spacing, int farbe );
void PaintIcon(char *filename, int x, int y, unsigned char offset);

#endif

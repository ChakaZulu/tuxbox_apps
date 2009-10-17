#ifndef __fb_window_h__
#define __fb_window_h__
/*
 * $Header: /cvs/tuxbox/apps/tuxbox/neutrino/src/driver/fb_window.h,v 1.7 2009/10/17 07:38:04 seife Exp $
 *
 * abstract fb_window class - d-box2 linux project
 *
 * (C) 2003 by thegoodguy <thegoodguy@berlios.de>
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
#include <stdint.h>

class CFBWindow
{
 public:
	typedef uint32_t color_t;
	typedef void *   font_t;
	typedef void *   private_data_t;

 private:
	private_data_t private_data;
	int x, y;   /* upper left corner */
	int dx, dy; /* dimension         */

 public:

	CFBWindow(const int _x, const int _y, const int _dx, const int _dy);
	virtual~CFBWindow();

	void paintBoxRel(const int _x, const int _y, const int _dx, const int _dy, const color_t _col, const int radius = 0, const int corners = 0xf);
	bool paintIcon(const char * const _filename, const int _x, const int _y, const color_t _offset = 1);
	void RenderString(const font_t _font, const int _x, const int _y, const int _width, const char * const _text, const color_t _color, const int _boxheight = 0, const bool _utf8_encoded = false);
};

#endif /* __fb_window_h__ */

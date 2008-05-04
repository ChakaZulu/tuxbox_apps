/*
 * $Id: buttons.cpp,v 1.5 2008/05/04 22:36:22 dbt Exp $
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <gui/widget/buttons.h>
#include <gui/color.h>


#include <system/settings.h>

/* paintButtons usage: use this fucntion for painting icons with captions in horizontal or vertical direction. 
 * frameBuffer		set framebuffer
 * font					set font eg: g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]
 * x 						set horizontal startposition
 * y 						set vertical startposition
 * buttonwidth		set width of button include width of icon, space and caption
 * count				set count of buttons
 * content			set iconfile and locale constant, for an empty text let locale constant empty , so you can paint icons without captions, 
 * maxwidth			optional, default value is 720 (full screenwidth) sets maximal width there can paint buttons, should be the same like width eg. from a menue
 * vertical_paint	optional, default value is false (horizontal) sets direction of painting
 */
void paintButtons(CFrameBuffer * const frameBuffer, Font * const font,
			const CLocaleManager * const localemanager, 
			const int x, 
			const int y, 
			const unsigned int buttonwidth, 
			const unsigned int count, 
			const struct button_label * const content,
			const unsigned int maxwidth,
			bool vertical_paint)
{
	int fheight = font->getHeight();
	unsigned int bwidth, fwidth;
	int ybase, yicon_start, ytext_start;
	int iconh, iconw, max_iconw = 0;
	int xstart  = x, ystart = y;
	int space = 4;
	unsigned int bestButtonwidth = maxwidth/count;
	unsigned int maxButtonwidth = bestButtonwidth > buttonwidth ? bestButtonwidth : buttonwidth;

	
	for (unsigned int i = 0; i < count; i++)
	{
		const char * buttontext =  content[i].locale ? localemanager->getText(content[i].locale) : "";
		const char * icon = content[i].button ? content[i].button : "";
		
		// real rendered textwidth		
		unsigned int real_textwidth = font->getRenderWidth(buttontext);
		
		// get height/width of icon
		iconh =  frameBuffer->getIconHeight(icon);
		iconw = frameBuffer->getIconWidth(icon);
		
		// calculate maximal witdh of icons
		max_iconw = max_iconw>iconw ? max_iconw : iconw; 
	
		// get width of  buttontext
		unsigned int fwidth = ((max_iconw+2*space+real_textwidth) > (maxButtonwidth)) ? maxButtonwidth : real_textwidth;
		
		// calculate finally buttonwidth
		bwidth = max_iconw + 2*space + fwidth;	
			
		// calculate baseline startposition of icon and text in y
		ybase = ystart + fheight - fheight / 2;
		yicon_start = ybase - iconh / 2;
		ytext_start = ybase + fheight / 2;

		// paint icon and text
		frameBuffer->paintIcon(icon, xstart , yicon_start);
		int xbuttontext = xstart + max_iconw + space;
		font->RenderString(xbuttontext, ytext_start, bwidth, buttontext, COL_INFOBAR_SHADOW_PLUS_1, 0, true); // UTF-8
		
		/* 	set next startposition x, if text is length=0 then offset is =renderwidth of icon, 
 		* 		for generating buttons without captions, 
 		*/		
		
		int lentext = strlen(buttontext);	
		if (vertical_paint) 
		// set xstart for painting buttons with vertical arrangement 
		{
				if (lentext !=0)
				{
					xstart = x;	
					ystart += fheight;				
				}
				else
				{
					xstart = xbuttontext;		
				}
		}
		else
		{
			// set xstart for painting buttons with horizontal arrangement as default
			xstart = lentext !=0 ? (xstart + bwidth + 2*space) : xbuttontext;
		}	
	}
}

/*
	$Id: audioplayer_setup.h,v 1.1 2009/10/13 19:17:10 dbt Exp $

	audioplayer setup implementation - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2009 T. Graf 'dbt'
	Homepage: http://www.dbox2-tuning.net/

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

	$Log: audioplayer_setup.h,v $
	Revision 1.1  2009/10/13 19:17:10  dbt
	init audioplayer_setup for it's own file
	
*/

#ifndef __audioplayer_setup__
#define __audioplayer_setup__

#include <gui/widget/menue.h>

#include <driver/framebuffer.h>


#include <string>

class CAudioPlayerSetup : public CMenuTarget
{
	private:
		CFrameBuffer *frameBuffer;
		
		int x, y, width, height, menue_width, hheight, mheight;

		void hide();
		void showAudioPlayerSetup();


	public:	
		CAudioPlayerSetup();
		~CAudioPlayerSetup();
		int exec(CMenuTarget* parent, const std::string & actionKey);
};


#endif

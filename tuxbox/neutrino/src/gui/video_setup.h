/*
	$Id: video_setup.h,v 1.2 2009/10/27 21:49:04 dbt Exp $

	video setup implementation - Neutrino-GUI

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

	$Log: video_setup.h,v $
	Revision 1.2  2009/10/27 21:49:04  dbt
	changed modul text to video
	
	Revision 1.1  2009/10/27 20:28:42  dbt
	init video setup for it's own modul
	
*/

#ifndef __video_setup__
#define __video_setup__

#include <gui/widget/menue.h>
#include <gui/widget/rgbcsynccontroler.h>

#include <driver/framebuffer.h>

#include <string>

class CVideoSetup : public CMenuTarget, CChangeObserver
{
	private:
		CFrameBuffer *frameBuffer;
		
		CMenuForwarder *   SyncControlerForwarder;
 		CMenuOptionChooser * VcrVideoOutSignalOptionChooser;

		int video_out_signal;
		int vcr_video_out_signal;
				
		int x, y, width, height, hheight, mheight;

 		virtual bool changeNotify(const neutrino_locale_t OptionName, void *);

		void hide();
		void showVideoSetup();


	public:	
		CVideoSetup();
		~CVideoSetup();
		int exec(CMenuTarget* parent, const std::string & actionKey);
};


#endif

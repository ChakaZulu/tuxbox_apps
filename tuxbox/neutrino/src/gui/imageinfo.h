/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: imageinfo.h,v 1.3 2007/04/08 17:17:38 dbt Exp $

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


#ifndef __imageinfo__
#define __imageinfo__

#include <string>

#include <configfile.h>

#include <gui/widget/menue.h>

#include <driver/framebuffer.h>
#include <driver/pig.h>

class CImageInfo : public CMenuTarget
{
	private:
		CConfigFile     * configfile;
		CFrameBuffer	*frameBuffer;

		int x, y;
		int xpos, ypos; //line starts
		int width, height;
		int hheight,iheight,sheight; // head/info/small font height

		int startX, startY; //boxposition
		int endX, endY;
	
		int max_height;	// Frambuffer 0.. max
		int max_width;
		
		int pigw; //picbox dimensions
		int pigh;

		int font_head;
		int font_info;
		int font_small;
	
		int x_offset_large; //offsets(space) between caption and infostrings
		int x_offset_small;

		std::string homepage;
		std::string creator;
		std::string imagename;
		std::string version;
		std::string subversion;
		std::string cvstime;
		std::string info;
		std::string distribution;
		std::string releaseCycle;
		std::string imagedate;
		std::string imagetype;
		std::string partitions;
		CPIG *pig;
		
		void paint();
		void paint_pig(int x, int y, int w, int h);
		void paintLine(int xpos, int font, const char* text);
		void paintContent(int fontSize, int xpos, int ypos, const char* text);
		void paintSupport(int y_startposition);
		void paintLicense(int y_startposition);
		void paintRevisionInfos(int y_startposition);
		void paintPartitions(int y_startposition);
		void clearContentBox();
		std::string getRawInfos();
		std::string readFile(std::string filename);

	public:

		CImageInfo();
		~CImageInfo();

		void hide();
		int exec(CMenuTarget* parent, const std::string& actionKey);
		const char *ImageInfo( int InfoType );
		std::string getChipInfo();
		std::string getImageInfoVersion();
		std::string getSysInfo(std::string infotag, bool reverse);
		void LoadImageInfo(void);
};

#endif

/*
	$Id: imageinfo.cpp,v 1.12 2007/04/22 20:37:04 dbt Exp $
	
	Neutrino-GUI  -   DBoxII-Project


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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/imageinfo.h>
#include <gui/widget/icons.h>

#include <cstring>
#include <iostream>
#include <fstream>

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/screen_max.h>

#include <daemonc/remotecontrol.h>

#include <system/flashtool.h>

using namespace std;

extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */

CImageInfo::CImageInfo()
{
	frameBuffer 	= CFrameBuffer::getInstance();

	font_head 	= SNeutrinoSettings::FONT_TYPE_MENU_TITLE;
	font_small 	= SNeutrinoSettings::FONT_TYPE_IMAGEINFO_SMALL;
	font_info 	= SNeutrinoSettings::FONT_TYPE_IMAGEINFO_INFO;

	hheight		= g_Font[font_head]->getHeight();
	iheight		= g_Font[font_info]->getHeight();
	sheight		= g_Font[font_small]->getHeight();
	
	//~ startX 	= g_settings.screen_StartX; //mainwindow position
	//~ startY 	= g_settings.screen_StartY;
	//~ endX 	= g_settings.screen_EndX;
	//~ endY 	= g_settings.screen_EndY;
	
	startX 	= 45; //mainwindow position
	startY 	= 35;
	endX 	= 720-startX;
	endY 	= 572-startY;
	
	width  	= w_max (endX, 5);
	height 	= h_max (endY, 5);

	max_width 	= endX-startX;
	max_height 	= endY-startY;
	
	pigw = 215;
	pigh = 170;
	
	x = endX - pigw -16;
	y = startY + hheight +16;
	
	x_offset_large	= 135;
	x_offset_small	= 7;

	// read partition info
	partitions = readFile("/proc/mtd");
}

CImageInfo::~CImageInfo()
{
	delete pig;
}

int CImageInfo::exec(CMenuTarget* parent, const std::string &)
{
	if (parent)
	{
		parent->hide();
	}

	paint();

	pig = new CPIG (0);
	paint_pig( x, y, pigw, pigh);

	neutrino_msg_t msg;

	while (1)
	{
		neutrino_msg_data_t data;
		
		// Timeout after 20 seconds -- NOT user configurable, no timeout in radiomode ;-)
		unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd_MS(20000);
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		// home key or timeout
		if (msg == CRCInput::RC_home || msg == CRCInput::RC_timeout)
		{
			break;
		}

		if ( msg >  CRCInput::RC_MaxRC && msg != CRCInput::RC_timeout)
		{
			CNeutrinoApp::getInstance()->handleMsg( msg, data );
		}
		
		// press redkey
		if (msg == CRCInput::RC_red )
		{
			CImageInfo::paintLicense(ypos);
		}

		// press greenkey
		if (msg == CRCInput::RC_green)
		{
			CImageInfo::paintSupport(ypos);
		}
		
		// press yellowkey
		if (msg == CRCInput::RC_yellow)
		{
			CImageInfo::paintRevisionInfos(ypos);
		}
		
		// press bluekey
		if (msg == CRCInput::RC_blue)
		{
			CImageInfo::paintPartitions(ypos);
		}
	}

	delete pig;
	hide();

	return menu_return::RETURN_REPAINT;
}

void CImageInfo::hide()
{
	pig->hide();
	frameBuffer->paintBackgroundBoxRel(0,0, 720, 572); //clean complete screen
}

void CImageInfo::clearContentBox()
{
	frameBuffer->paintBoxRel(xpos, iheight*10+6, width-10, endY-(iheight*10+6)-32, COL_MENUCONTENT_PLUS_0 );
}

void CImageInfo::paint_pig(int x, int y, int w, int h)
{
	int xPig = x; //picture position
	int yPig = y; //+ hheight +16;
	frameBuffer->paintBoxRel(xPig, yPig, w, h, COL_MENUCONTENT_PLUS_0);
	pig->show (xPig, yPig, w, h);
}

void CImageInfo::paintLine(int xpos, int font, const char* text)
{
	g_Font[font]->RenderString(xpos, ypos, width-10, text, COL_MENUCONTENT, 0, true);
}

void CImageInfo::paintContent(int fontSize, int xpos, int ypos, const char *text)
{
	g_Font[fontSize]->RenderString(xpos, ypos, width-10, text, COL_MENUCONTENT, 0, true);
}

void CImageInfo::paintSupport(int y_startposition)
{
	clearContentBox();
	
	//paint info only if present in /.version
	if (info.length())
		 {
			paintContent(font_info, xpos, y_startposition,g_Locale->getText(LOCALE_IMAGEINFO_INFO));
			paintContent(font_info, xpos+x_offset_large+30, y_startposition, info.c_str());
			y_startposition += hheight; 
		 }
	
	paintContent(font_info, xpos, y_startposition,g_Locale->getText(LOCALE_IMAGEINFO_SUPPORTHERE));
	
	y_startposition += hheight;	
	paintContent(font_info, xpos, y_startposition,g_Locale->getText(LOCALE_IMAGEINFO_HOMEPAGE));
	paintContent(font_info, xpos+x_offset_large+30, y_startposition, homepage.c_str());
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition,g_Locale->getText(LOCALE_IMAGEINFO_DOKUMENTATION));
	paintContent(font_info, xpos+x_offset_large+30, y_startposition, "http://wiki.godofgta.de");
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition,g_Locale->getText(LOCALE_IMAGEINFO_FORUM));
	paintContent(font_info, xpos+x_offset_large+30, y_startposition, "http://forum.tuxbox.org");
	
	y_startposition += sheight;
	frameBuffer->paintLine(xpos, y_startposition, max_width, y_startposition, COL_MENUCONTENT_PLUS_3);
	
	y_startposition += hheight;
	paintContent(font_info, xpos, y_startposition,g_Locale->getText(LOCALE_IMAGEINFO_NOTEFLASHTYPE));
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition, g_Locale->getText(LOCALE_IMAGEINFO_CHIPSET) );
	paintContent(font_info, xpos+x_offset_large+60,y_startposition, getChipInfo().c_str());
}

void CImageInfo::paintPartitions(int y_startposition)
{
	unsigned int spos, rpos;
	int readlen;

	clearContentBox();
	spos = 30;
	while (spos < partitions.length())
	{
		rpos = partitions.find("\n",spos);
		readlen = rpos-spos;
		paintContent(font_info, xpos, y_startposition, partitions.substr(spos, readlen).c_str());
		y_startposition += iheight;
		spos += readlen + 1;
	}
}

void CImageInfo::paintLicense(int y_startposition)
{
	// lines for license
	string LLine[] = {"This program is free software; you can redistribute it and/or modify",
							"it under the terms of the GNU General Public License as published by",
							"the Free Software Foundation; either version 2 of the License, or",
							"(at your option) any later version.",
							"",
							"This program is distributed in the hope that it will be useful,",
							"but WITHOUT ANY WARRANTY; without even the implied warranty of",
							"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.",
							"See the GNU General Public License for more details.",
							"",
							"You should have received a copy of the GNU General Public License",
							"along with this program; if not, write to the Free Software",
							"Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA."};
	
	clearContentBox();
	
	int fh = sheight+12;
	for (int i = 0; i<13; i++) // paint all lines
		{
			g_Font[font_small]->RenderString(xpos,  y_startposition, width-10, LLine[i].c_str(), COL_MENUCONTENT, 0, true);
			y_startposition += (fh >>1);
		}

}

void CImageInfo::paintRevisionInfos(int y_startposition)
{
	clearContentBox();
	
	paintContent(font_info, xpos, y_startposition, "Kernel:" );
	paintContent(font_info, xpos+x_offset_large, y_startposition, getSysInfo("-dbox2", true).c_str());
//	paintContent(font_info, xpos+x_offset_large, y_startposition, readFile("/proc/version").c_str());
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition, "BusyBox:" );
	paintContent(font_info, xpos+x_offset_large, y_startposition, getSysInfo("BusyBox v", false).c_str());
	
	//~ CNeutrinoInfo neutrinoinfo;
	//~ y_startposition += iheight;
	//~ paintContent(font_info, xpos, y_startposition, "Neutrino:" );
	//~ paintContent(font_info, xpos+x_offset_large, y_startposition, neutrinoinfo.getNeutrinoVersion().c_str());
	
	//~ CMoviePlayerInfo mplayer;
	//~ y_startposition += iheight;
	//~ paintContent(font_info, xpos, y_startposition, "Movieplayer:" );
	//~ paintContent(font_info, xpos+x_offset_large, y_startposition, mplayer.getMoviePlayerVersion().c_str());
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition, "Imageinfo:" );
	paintContent(font_info, xpos+x_offset_large, y_startposition, getImageInfoVersion().c_str());
		
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition, "JFFS2:" );
	paintContent(font_info, xpos+x_offset_large, y_startposition, getSysInfo("JFFS2 version ", false).c_str());
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition, "Squashfs:" );
	paintContent(font_info, xpos+x_offset_large, y_startposition, getSysInfo("squashfs: version ", false).c_str());
	
}

string CImageInfo::readFile(string filename)
{
	string systxt;

	ifstream f(filename.c_str());
	char ch;
	while(f.get(ch)) {
		systxt += ch;
	}
	f.close();
	return systxt;
}

string CImageInfo::getRawInfos()
{
	const char *sysinfofile ="/tmp/systmp";
	std::string cmd = "dmesg > ";
	std::string cmdbb = "busybox >>";
	
	//generate infofile if it's necessarily
	if ((access(sysinfofile, 0 ) != -1)==false)
	{
		system((cmd + sysinfofile).c_str());
		system((cmdbb + sysinfofile).c_str());
	}
	
	return readFile(sysinfofile);
}


string CImageInfo::getSysInfo(string infotag, bool reverse)
/* use CImageInfo::getSysinfo():" 
 * arg infotag is a part of line in the output file "/tmp/systmp" generated by system command <dmesg> in "getRawInfos()" 
 * for example-> first line in "/tmp/systmp" is: "<4>Linux version 2.4.34-dbox2 (user@linux) (gcc version 3.4.4)..."
 * 
 * if you using with arg reverse = false:
 * getSysInfo("Linux version ", false) returns as string:  "2.4.34-dbox2 (user@linux) (gcc version 3.4.4)..."
 *
 * if you using arg reverse = true: 
 * getSysInfo("-dbox", true) returns as string:  "Linux version 2.4.34"
 * so you can get many more informations from "/tmp/systmp"
 */
{
	string str = getRawInfos();
	string sysstr;
	int lpos, spos, rpos, readlen;
	int taglen = infotag.length();
	
	string::size_type loc = str.find( infotag, 0 );
	if( loc != string::npos )
			if (reverse == false)
				{
					lpos = str.find(infotag,0);
					spos = lpos + taglen;
					rpos = str.find("\n",spos);
					readlen = rpos-spos;
					sysstr = str.substr(spos, readlen);	
				}
			else
				{
					rpos = str.find(infotag,0);
					lpos = str.rfind(">", rpos)+1;
					readlen = rpos - lpos;
					sysstr = str.substr(lpos, readlen);
				}
		else
		{
			sysstr=g_Locale->getText(LOCALE_FLASHUPDATE_GETINFOFILEERROR);
		}
	return sysstr;
}

string CImageInfo::getChipInfo()
{		
	string str = getSysInfo("D-Box 2 flash memory: Found ", false);
	
	string::size_type locx = str.find( "x16", 0 );
		if( locx != string::npos )
			{
			string::size_type loc = str.find( "1 x16", 0 );
			if( loc != string::npos )
				{
					chiptype = "1xI";
				}
			else
				{
					chiptype = "2xI";
				}
			}
		else
		{
			chiptype=g_Locale->getText(LOCALE_FLASHUPDATE_GETINFOFILEERROR);
		}
	return chiptype;
}

void CImageInfo::LoadImageInfo(void)
{
	CConfigFile config('\t');
	config.loadConfig("/.version");
	
	homepage 	= config.getString("homepage", "http://www.tuxbox.org");
	creator 	= config.getString("creator", "");
	imagename 	= config.getString("imagename", "self compiled");
	version 	= config.getString("version", "");
	subversion 	= config.getString("subversion", "");
	cvstime 	= config.getString("cvs", "");
	info 		= config.getString("info", "");
	distribution 	= imagename + " " + subversion;
	
	CFlashVersionInfo versionInfo(version); //get from flashtool.cpp
	
	releaseCycle 	= versionInfo.getReleaseCycle();
	imagedate 	= (std::string)versionInfo.getDate() + " " + versionInfo.getTime();
	imagetype 	= versionInfo.getType();
}

string CImageInfo::getImageInfoVersion()
{
/* 	Note: revision (revstr) will change automaticly on cvs-commits, 
 * 	if you made some changes without cvs-commit, you must change it before by yourself
 */
	string revstr = "$Revision: 1.12 $";
	while (revstr.find("$") !=string::npos)
	{
		revstr.replace(revstr.find("$"),1 ,""); //normalize output, remove "$"	
	}
	return revstr;
}

void CImageInfo::paint()
{
	const char * head_string;
	
	xpos = startX + 10;
	ypos = startY + 5;

	LoadImageInfo();
	head_string = g_Locale->getText(LOCALE_IMAGEINFO_HEAD);
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, head_string);
	
	//paint main window
	frameBuffer->paintBoxRel(startX,startY, max_width, max_height, COL_MENUCONTENT_PLUS_0);
	//paint title bar
	frameBuffer->paintBoxRel(startX, startY, max_width, hheight+7, COL_MENUHEAD_PLUS_0);
	//paint title
	g_Font[font_head]->RenderString(xpos, startY + hheight + 4, width, head_string, COL_MENUHEAD, 0, true);

	ypos += hheight;
	ypos += (iheight >>1);
	
	//paint head infos
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_IMAGE)); //imagename
	paintLine(xpos + x_offset_large, font_info, distribution.c_str());
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_VERSION));
	paintLine(xpos + x_offset_large, font_info, releaseCycle.c_str()); //releaseCycle
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_IMAGETYPE));
	paintLine(xpos + x_offset_large, font_info, imagetype.c_str()); //imagetype
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_DATE));
	paintLine(xpos + x_offset_large, font_info, imagedate.c_str()); //date of generation 
	
	//paint creator, cvslevel, info only if present in /.version
	ypos += iheight;
	if (creator.length())
		{
			paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_CREATOR));
			paintLine(xpos + x_offset_large, font_info, creator.c_str());
		}
		
	ypos += iheight;
	if (cvstime.length())
		{
			paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_CVSLEVEL));
			paintLine(xpos + x_offset_large, font_info, cvstime.c_str());
		}	

	//paint separator
	ypos += iheight;
	frameBuffer->paintLine(xpos, ypos-10, x-10  , ypos-10, COL_MENUCONTENT_PLUS_3);
	
	//license lines
	ypos += iheight;
	paintLicense(ypos);

	//paint buttonbar
	frameBuffer->paintBoxRel(startX, endY-32, max_width, 32, COL_MENUCONTENT_PLUS_0);
	
	//paint icons and captions
	int yIconPos = endY-26, xIconOffset = max_width/5;
	
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_RED, xpos ,yIconPos+3); //License
	g_Font[font_small]->RenderString(xpos+20, yIconPos+22, xIconOffset, g_Locale->getText(LOCALE_IMAGEINFO_LICENSE), COL_MENUCONTENT, 0, true);
	
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_GREEN, (xpos + xIconOffset) ,yIconPos+3); //Support
	g_Font[font_small]->RenderString(xpos+xIconOffset+20, yIconPos+22, xIconOffset, g_Locale->getText(LOCALE_IMAGEINFO_SUPPORT), COL_MENUCONTENT, 0, true);
	
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW, (xpos + xIconOffset*2) ,yIconPos+3); //Revision
	g_Font[font_small]->RenderString(xpos+(xIconOffset*2)+20, yIconPos+22, xIconOffset, g_Locale->getText(LOCALE_IMAGEINFO_DETAILS), COL_MENUCONTENT, 0, true);
	
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_BLUE, (xpos + xIconOffset*3) ,yIconPos+3); //Partitions
	g_Font[font_small]->RenderString(xpos+(xIconOffset*3)+20, yIconPos+22, xIconOffset, g_Locale->getText(LOCALE_IMAGEINFO_PARTITIONS), COL_MENUCONTENT, 0, true);

	frameBuffer->paintIcon("home.raw",(xpos + xIconOffset*4) ,yIconPos-3); //Home 
	g_Font[font_small]->RenderString(xpos+(xIconOffset*4)+36, yIconPos+22, xIconOffset, g_Locale->getText(LOCALE_MENU_BACK), COL_MENUCONTENT, 0, true);
}

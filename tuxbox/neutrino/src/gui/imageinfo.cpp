/*
	$Id: imageinfo.cpp,v 1.16 2008/05/01 00:08:23 dbt Exp $
	
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

#define MOVIEBROWSER
#ifdef MOVIEBROWSER
#include <gui/moviebrowser.h>
#endif /* MOVIEBROWSER */
#include <gui/pictureviewer.h>
#include <gui/streaminfo2.h>

#include <gui/widget/icons.h>
#include <gui/widget/buttons.h>

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
	font_small_text 	= SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL;

	hheight		= g_Font[font_head]->getHeight();
	iheight		= g_Font[font_info]->getHeight();
	sheight		= g_Font[font_small]->getHeight();
	ssheight		= g_Font[font_small_text]->getHeight();
	
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
	frameBuffer->paintBoxRel(xpos, iheight*10+6, max_width-10, endY-(iheight*10+6)-32, COL_MENUCONTENT_PLUS_0 );
}

void CImageInfo::paint_pig(int x, int y, int w, int h)
{
	int xPig = x; //picture position
	int yPig = y; //+ hheight +16;
	frameBuffer->paintBoxRel(xPig, yPig, w, h, COL_MENUCONTENT_PLUS_0);
	pig->show (xPig, yPig, w, h);
}

void CImageInfo::paintLine(int xpos, int font, const char* text, uint8_t    color = COL_MENUCONTENT )
{
	g_Font[font]->RenderString(xpos, ypos, width-10, text, color, 0, true);
}

void CImageInfo::paintContent(int fontSize, int xposC, int yposC, const char *text, uint8_t    color = COL_MENUCONTENT)
{
	g_Font[fontSize]->RenderString(xposC, yposC, width-10, text, color, 0, true);
}

void CImageInfo::paintSupport(int y_startposition)
{
	clearContentBox();
	
	//paint comment lines only if present in /.version
	if (comment1.length())
		 {
			paintContent(font_info, xpos, y_startposition, comment1.c_str());
			y_startposition += sheight; 
		 }
		 
	if (comment2.length())
		{
			paintContent(font_info, xpos, y_startposition,  comment2.c_str());
			y_startposition += iheight+5; 
		}
		
	paintContent(font_info, xpos, y_startposition,g_Locale->getText(LOCALE_IMAGEINFO_SUPPORTHERE), COL_MENUCONTENTINACTIVE);
	
	y_startposition += iheight;	
	paintContent(font_info, xpos, y_startposition,g_Locale->getText(LOCALE_IMAGEINFO_HOMEPAGE), COL_MENUCONTENTINACTIVE);
	paintContent(font_info, xpos+x_offset_large, y_startposition, homepage.c_str());
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition,g_Locale->getText(LOCALE_IMAGEINFO_DOKUMENTATION), COL_MENUCONTENTINACTIVE);
	paintContent(font_info, xpos+x_offset_large, y_startposition, "http://wiki.dbox2-tuning.net");
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition,g_Locale->getText(LOCALE_IMAGEINFO_FORUM), COL_MENUCONTENTINACTIVE);
	paintContent(font_info, xpos+x_offset_large, y_startposition, "http://forum.tuxbox.org");
	
#ifndef HAVE_DREAMBOX_HARDWARE	
	y_startposition += ssheight/2;
	frameBuffer->paintLine(xpos, y_startposition, max_width, y_startposition, COL_GRAY);	
		
	y_startposition += ssheight+ ssheight/2;
	paintContent(font_info, xpos, y_startposition,g_Locale->getText(LOCALE_IMAGEINFO_NOTEFLASHTYPE));
	
	y_startposition += sheight;
	paintContent(font_info, xpos, y_startposition, g_Locale->getText(LOCALE_IMAGEINFO_CHIPSET) );
	paintContent(font_info, xpos+x_offset_large+60,y_startposition, getChipInfo().c_str());		
#endif

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
	
	int fh = ssheight+12;
	for (int i = 0; i<13; i++) // paint all lines
		{
			g_Font[font_small_text]->RenderString(xpos,  y_startposition, width-10, LLine[i].c_str(), COL_MENUCONTENT, 0, true);
			y_startposition += (fh >>1);
		}

}

void CImageInfo::paintRevisionInfos(int y_startposition)
{
	struct utsname u;

	clearContentBox();
	
	paintContent(font_info, xpos, y_startposition, "Kernel:", COL_MENUCONTENTINACTIVE );
	if (!uname(&u))	// no idea what could go wrong here, but u would be uninitialized.
		paintContent(font_info, xpos+x_offset_large, y_startposition, u.release);
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition, "BusyBox:", COL_MENUCONTENTINACTIVE );
	paintContent(font_info, xpos+x_offset_large, y_startposition, getSysInfo("BusyBox v", false).c_str());
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition, "JFFS2:", COL_MENUCONTENTINACTIVE );
	paintContent(font_info, xpos+x_offset_large, y_startposition, getSysInfo("JFFS2 version ", false).c_str());
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition, "Squashfs:", COL_MENUCONTENTINACTIVE );
	paintContent(font_info, xpos+x_offset_large, y_startposition, getSysInfo("squashfs: version ", false).c_str());
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition, "Imageinfo:", COL_MENUCONTENTINACTIVE );
	paintContent(font_info, xpos+x_offset_large, y_startposition, getModulVersion("","$Revision: 1.16 $").c_str());
	
#ifdef MOVIEBROWSER
	y_startposition += iheight;
	static CMovieBrowser mb;
	paintContent(font_info, xpos, y_startposition, "Moviebrowser:", COL_MENUCONTENTINACTIVE );
	paintContent(font_info, xpos+x_offset_large, y_startposition, mb.getMovieBrowserVersion().c_str());
#endif /* MOVIEBROWSER */

	y_startposition += iheight;
	static CPictureViewerGui pv;
	paintContent(font_info, xpos, y_startposition, "Pictureviewer:", COL_MENUCONTENTINACTIVE );
	paintContent(font_info, xpos+x_offset_large, y_startposition, pv.getPictureViewerVersion().c_str());
	
	y_startposition += iheight;
	static CStreamInfo2 si;
	paintContent(font_info, xpos, y_startposition, "Streaminfo:", COL_MENUCONTENTINACTIVE );
	paintContent(font_info, xpos+x_offset_large, y_startposition, si.getStreamInfoVersion().c_str());
	
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
	unsigned int spos, rpos;
	string line;

	chiptype=g_Locale->getText(LOCALE_FLASHUPDATE_GETINFOFILEERROR);
	spos = 0;
	while (spos < partitions.length())
	{
		rpos = partitions.find("\n",spos);
		if (rpos == string::npos) // must not happen!
			break;
		line = partitions.substr(spos, rpos - spos);
		string::size_type n = line.find("BR bootloader" ,0);
		if (n != string::npos) {
			// printf("Bootloader found: %s\n", line.c_str());
			// printf("character at n-6: %s\n", line.substr(n-6, 1).c_str());
			/* the 1xI boxen have mtd0: 00020000 00020000 "BR bootloader"
			   the 2xI boxen have mtd0: 00020000 00004000 "BR bootloader"
			                                  (n-6)--^     ^--(n)
			   This is at least as reliable as the old method of grepping
			   through dmesg output... */
			if (line.substr(n - 6, 1) == "4")
				chiptype = "2xI";
			else
				chiptype = "1xI";
			break;
		}
		spos = rpos + 1;
	}

	return chiptype;
}

void CImageInfo::LoadImageInfo(void)
{
	CConfigFile config('\t');
	config.loadConfig("/.version");
	
	homepage		= config.getString("homepage", "http://www.tuxbox.org");
	creator		= config.getString("creator", "");
	imagename		= config.getString("imagename", "self compiled");
	version		= config.getString("version", "");
	subversion		= config.getString("subversion", "");
	cvstime		= config.getString("cvs", "");
	comment1		= config.getString("comment1", "");
	comment2		=config.getString("comment2","");
	distribution 	= imagename + " " + subversion;
	
	CFlashVersionInfo versionInfo(version); //get from flashtool.cpp
	
	releaseCycle 	= versionInfo.getReleaseCycle();
	imagedate 	= (std::string)versionInfo.getDate() + " " + versionInfo.getTime();
	imagetype 	= versionInfo.getType();
}

const char* CImageInfo::getImageInfo (int InfoType)
{
	LoadImageInfo();	
	switch (InfoType)
 	{
    case IMAGENAME :  //imagename
		return imagename.c_str();
		break;
	case DISTRIBUTION :  //imagename + subversion
		return distribution.c_str();
		break;
	case IMAGEVERSION :  //raw version info 
		return version.c_str();
		break;
	case SUBVERSION :  //subversion
		return subversion.c_str();
		break;
	case CVSTIME :  //date of cvs if present
		return cvstime.c_str();
		break;
	case CREATOR :   //name of imagebuilder
		return creator.c_str();
		break;
	case HOMEPAGE :   //support hompage
		return homepage.c_str();
		break;
	case COMMENT1 :   //firt comment if present
		return comment1.c_str();
		break;
	case COMMENT2 :   //second comment if present
		return comment2.c_str();
		break;
 	default : return "n/a";
  }
}

void CImageInfo::paintHead(int x, int y, const char *localetext)
{
	int headheight = hheight;
	frameBuffer->paintBoxRel(x, y, max_width, headheight + 4, COL_MENUHEAD, g_settings.rounded_corners ? CORNER_RADIUS_MID : 0 , CORNER_TOP);
	g_Font[font_head]->RenderString(x+5, y + headheight + 4, width, localetext, COL_MENUHEAD, 0, true);}
	
const struct button_label CImageInfoButtons[5] =
{
	{ NEUTRINO_ICON_BUTTON_RED   , LOCALE_IMAGEINFO_LICENSE },
	{ NEUTRINO_ICON_BUTTON_GREEN  , LOCALE_IMAGEINFO_SUPPORT },
	{ NEUTRINO_ICON_BUTTON_YELLOW,  LOCALE_IMAGEINFO_DETAILS },
	{ NEUTRINO_ICON_BUTTON_BLUE, LOCALE_IMAGEINFO_PARTITIONS },
	{ NEUTRINO_ICON_BUTTON_HOME, LOCALE_IMAGEINFO_EXIT }
};

void CImageInfo::paintFoot(int x, int y)
{
	int ButtonHeight = ssheight;
	frameBuffer->paintBoxRel(x, y, max_width, ButtonHeight, COL_INFOBAR_SHADOW_PLUS_1, g_settings.rounded_corners ? CORNER_RADIUS_MID : 0 , CORNER_BOTTOM);
	::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 5, y, max_width/6, 5, CImageInfoButtons, width);
}	

void CImageInfo::paint()
{
	xpos = startX + 10;
	ypos = startY + 10;

	LoadImageInfo();
	const char *head_string = g_Locale->getText(LOCALE_IMAGEINFO_HEAD);
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, head_string);
	
	//paint head
	paintHead (startX, startY, head_string);
	
	//paint main window
	frameBuffer->paintBoxRel(startX, startY+hheight+4, max_width, max_height-hheight-ssheight-4, COL_MENUCONTENT_PLUS_0);

	ypos += hheight;
	ypos += (iheight >>1);
	
	//paint head infos
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_IMAGE), COL_MENUCONTENTINACTIVE); //imagename
	paintLine(xpos + x_offset_large, font_info, distribution.c_str());
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_VERSION), COL_MENUCONTENTINACTIVE);
	paintLine(xpos + x_offset_large, font_info, releaseCycle.c_str()); //releaseCycle
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_IMAGETYPE), COL_MENUCONTENTINACTIVE);
	paintLine(xpos + x_offset_large, font_info, imagetype.c_str()); //imagetype
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_DATE), COL_MENUCONTENTINACTIVE);
	paintLine(xpos + x_offset_large, font_info, imagedate.c_str()); //date of generation 
	
	//paint creator, cvslevel, info, comment only if present in /.version
	ypos += iheight;
	if (creator.length())
		{
			paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_CREATOR), COL_MENUCONTENTINACTIVE);
			paintLine(xpos + x_offset_large, font_info, creator.c_str());
		}
		
	ypos += iheight;
	if (cvstime.length())
		{
			paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_CVSLEVEL), COL_MENUCONTENTINACTIVE);
			paintLine(xpos + x_offset_large, font_info, cvstime.c_str());
		}

	ypos += iheight;

	//license lines
	ypos += sheight;
	paintLicense(ypos);

	//paint foot		
	paintFoot(startX, startY+max_height-ssheight);
		
}

/* 	useful stuff for version informations * getModulVersion()
 * 	returns a numeric version string for better version handling from any module without 	
 * 	special characters like "$" or the complete string "Revision" ->> eg: "$Revision: 1.16 $" becomes "1.146", 
 * 	argument prefix can be empty or a replacement for "Revision"-string eg. "Version: " or "v." as required,
 * 	argument ID_string must be a CVS-keyword like "$Revision: 1.16 $", used and changed by 
 * 	cvs-committs or a version data string eg: "1.xxx" by yourself
 * 	some examples:
 * 	getModulVersion("Version: ","$Revision: 1.16 $")	 returns "Version: 1.153"	
 * 	getModulVersion("v.","$Revision: 1.16 $")			 returns "v.1.153"
 *  	getModulVersion("","$Revision: 1.16 $")		 		 returns "1.153"
 */
 std::string CImageInfo::getModulVersion(const std::string &prefix_string, std::string ID_string)
{
	string revstr = ID_string;
	while (revstr.find("$") !=string::npos)
	{
		revstr.replace(revstr.find("$"),1 ,""); //normalize output, remove "$"	
	}
	
	while (revstr.find("Revision: ") !=string::npos)
	{
		revstr.replace(revstr.find("Revision: "),10 ,""); //normalize output, remove "Revision:"	
	}
	
	return revstr;
}

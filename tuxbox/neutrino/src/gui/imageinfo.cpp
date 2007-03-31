/*
	$Id: imageinfo.cpp,v 1.9 2007/03/31 09:05:32 dbt Exp $
	
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
//#include <gui/movieplayer.h>

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

char releaseCycle[25]= "";
char imagedate[18]	= "";
char imagetype[18] = "";
char distribution[25] = "";

extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */

CImageInfo::CImageInfo()
{
	frameBuffer = CFrameBuffer::getInstance();

	font_head 	= SNeutrinoSettings::FONT_TYPE_MENU_TITLE;
	font_small 	= SNeutrinoSettings::FONT_TYPE_IMAGEINFO_SMALL;
	font_info 		= SNeutrinoSettings::FONT_TYPE_IMAGEINFO_INFO;

	hheight	= g_Font[font_head]->getHeight();
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
	
	 x_offset_large		= 135;
	 x_offset_small	= 7;
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
	int xPig = x- 10; //picture position
	int yPig = y; //+ hheight +16;
	frameBuffer->paintBoxRel(xPig, yPig, w, h, COL_MENUCONTENT_PLUS_0);
	pig->show (xPig, yPig, w, h);
}

void CImageInfo::paintLine(int xpos, int font, const char* text)
{
	char buf[100];
	sprintf((char*) buf, "%s", text);
	g_Font[font]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true);
}

void CImageInfo::paintContent(int fontSize, int xpos, int ypos, const char *text)
{
	g_Font[fontSize]->RenderString(xpos, ypos, width-10, text, COL_MENUCONTENT, 0, true);
}

void CImageInfo::paintSupport(int y_startposition)
{
	clearContentBox();
	
	//paint info only if present in /.version
	const char *vinfo = ImageInfo(VERSION_INFO);
	if (strlen(vinfo)!=0)
		 {
			paintContent(font_info, xpos, y_startposition,g_Locale->getText(LOCALE_IMAGEINFO_INFO));
			paintContent(font_info, xpos+x_offset_large+30, y_startposition, vinfo);
			y_startposition += hheight; 
		 }
	
	paintContent(font_info, xpos, y_startposition,g_Locale->getText(LOCALE_IMAGEINFO_SUPPORTHERE));
	
	y_startposition += hheight;	
	paintContent(font_info, xpos, y_startposition,g_Locale->getText(LOCALE_IMAGEINFO_HOMEPAGE));
	paintContent(font_info, xpos+x_offset_large+30, y_startposition, ImageInfo(VERSION_HOMEPAGE));
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition,g_Locale->getText(LOCALE_IMAGEINFO_DOKUMENTATION));
	paintContent(font_info, xpos+x_offset_large+30, y_startposition,"http://wiki.godofgta.de");
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition,g_Locale->getText(LOCALE_IMAGEINFO_FORUM));
	paintContent(font_info, xpos+x_offset_large+30, y_startposition,"http://forum.tuxbox.org");
	
	y_startposition += sheight;
	frameBuffer->paintLine(xpos, y_startposition, max_width, y_startposition, COL_MENUCONTENT_PLUS_3);
	
	y_startposition += hheight;
	paintContent(font_info, xpos, y_startposition,g_Locale->getText(LOCALE_IMAGEINFO_NOTEFLASHTYPE));
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition, g_Locale->getText(LOCALE_IMAGEINFO_CHIPSET) );
	paintContent(font_info, xpos+x_offset_large+30, y_startposition, getChipInfo().c_str());
}

void CImageInfo::paintPartitions(int y_startposition)
{
	clearContentBox();
	int xpartpos = xpos+x_offset_large +164;
	
	paintContent(font_info, xpos, y_startposition, "mtd0: \"BR-Bootloader\"" );
	paintContent(font_info, xpartpos, y_startposition, getSysInfo(" : \"BR bootloader\"", true).c_str());
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition, "mtd1: \"FLFS (u-boot)\"" );
	paintContent(font_info, xpartpos, y_startposition, getSysInfo(" : \"FLFS (U-Boot)\"", true).c_str());
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition, "mtd2: \"root (rootfs)\"" );
	paintContent(font_info, xpartpos, y_startposition, getSysInfo(" : \"root (rootfs)\"", true).c_str());
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition, "mtd3: \"var (jffs2)\"" );
	paintContent(font_info, xpartpos, y_startposition, getSysInfo(" : \"var (jffs2)\"", true).c_str());
	
	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition, "mtd4: \"Flash without bootloader\"" );
	paintContent(font_info, xpartpos, y_startposition, getSysInfo(" : \"Flash without bootloader\"", true).c_str());

	y_startposition += iheight;
	paintContent(font_info, xpos, y_startposition, "mtd5: \"Complete Flash\"" );
	paintContent(font_info, xpartpos, y_startposition, getSysInfo(" : \"Complete Flash\"", true).c_str());
	
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


string CImageInfo::getRawInfos()
{
	const char *sysinfofile ="/tmp/systmp";
	string  systxt, cmd, cmdbb;
	cmd = "dmesg > ";
	cmdbb = "busybox >>";
	
	//generate infofile if it's necessarily
	if ((access(sysinfofile, 0 ) != -1)==false)
		{	
		string command1 = cmd + sysinfofile; //for dmesg infos
		string command2 = cmdbb + sysinfofile; //for busyox infos		
		system(command1.c_str());
		system(command2.c_str());	
		} 	
	
	//read infofile		
    ifstream f(sysinfofile); 
     char ch;
       while(f.get(ch))
		   systxt += ch;
    f.close(); 
		 
	return systxt;	
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
	return CImageInfo::sysstr;
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
	return CImageInfo::chiptype;
}

const char *CImageInfo::ImageInfo(int InfoType)
{
	CConfigFile config('\t');
	config.loadConfig("/.version");
	
	const char * homepage = config.getString	("homepage",  "http://www.tuxbox.org").c_str();
	const char * creator = config.getString		("creator",   "").c_str();
	const char * imagename = config.getString	("imagename", "self compiled").c_str();
	const char * version = config.getString		("version",   "").c_str();
	const char * subversion 	= config.getString	("subversion",   "").c_str();
	const char * cvstime = config.getString		("cvs",   "").c_str();
	const char * info = config.getString				("info",   "").c_str();
	//const char * distribution;
	sprintf((char*) distribution, "%s %s", imagename, subversion);	
	
    static CFlashVersionInfo versionInfo(version); //get from flashtool.cpp
	
	sprintf((char*) releaseCycle, "%s"	, versionInfo.getReleaseCycle());
	sprintf((char*) imagedate, "%s  %s", versionInfo.getDate(), versionInfo.getTime());
	sprintf((char*) imagetype, "%s", versionInfo.getType());
	
	
	switch (InfoType)
	{
		case VERSION_IMAGENAME: 	i_info = imagename; //return imagename;	
			break;
		case VERSION_CREATOR:	 		i_info = creator; //return creator;		
			break;
		case VERSION_HOMEPAGE: 		i_info = homepage; //return homepage;			
			break;
		case VERSION_REVISION: 		i_info = version; //return version;			
			break;
		case VERSION_SUBVERSION: 	i_info = subversion; //return subversion;			
			break;	
		case VERSION_CVSLEVEL: 		i_info = cvstime; //return cvstime;		
			break;
		case VERSION_DISTRIBUTION: i_info = distribution; //return distribution;		
			break;
		case VERSION_RELCYCLE: 		i_info = releaseCycle; //return releaseCycle;			
			break;
		case VERSION_DATE: 				i_info = imagedate; //return imagedate;
			break;
		case VERSION_TYPE: 				i_info = imagetype; //return imagetype;		
			break;
		case VERSION_INFO: 				i_info = info; //return info;			##Note: currently not used##
			break;
		default: 
			break;
	}
	return CImageInfo::i_info;
}

string CImageInfo::getImageInfoVersion()
{
/* 	Note: revision (revstr) will change automaticly on cvs-commits, 
 * 	if you made some changes without cvs-commit, you must change it before by yourself
 */
	revstr = "$Revision: 1.9 $";
		while (revstr.find("$") !=string::npos) 
				revstr.replace(revstr.find("$"),1 ,""); //normalize output, remove "$"	
	return CImageInfo::revstr;
}

void CImageInfo::paint()
{
	const char * head_string;	
	
 	xpos = startX + 10;
	ypos = startY + 5;

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
	paintLine(xpos + x_offset_large, font_info, ImageInfo(VERSION_DISTRIBUTION));
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_VERSION));
	paintLine(xpos + x_offset_large, font_info, ImageInfo(VERSION_RELCYCLE)); //releaseCycle
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_IMAGETYPE));
	paintLine(xpos + x_offset_large, font_info, ImageInfo(VERSION_TYPE)); //imagetype
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_DATE));
	paintLine(xpos + x_offset_large, font_info, ImageInfo(VERSION_DATE)); //date of generation 
	
	//paint creator and cvslevel only if present in /.version
	ypos += iheight;
	const char *imgcreator = ImageInfo(VERSION_CREATOR);
	if (strlen(imgcreator)!=0)
		{
			paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_CREATOR));
			paintLine(xpos + x_offset_large, font_info, imgcreator);
		}
		
	ypos += iheight;
	const char *imgcvs = ImageInfo(VERSION_CVSLEVEL);	
	if (strlen(imgcvs)!=0)
		{		
			paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_CVSLEVEL));
			paintLine(xpos + x_offset_large, font_info, imgcvs);
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
	
	frameBuffer->paintIcon("rot.raw",xpos ,yIconPos+3); //License
	g_Font[font_small]->RenderString(xpos+20, yIconPos+22, xIconOffset, g_Locale->getText(LOCALE_IMAGEINFO_LICENSE), COL_MENUCONTENT, 0, true);
	
	frameBuffer->paintIcon("gruen.raw",(xpos + xIconOffset) ,yIconPos+3); //Support
	g_Font[font_small]->RenderString(xpos+xIconOffset+20, yIconPos+22, xIconOffset, g_Locale->getText(LOCALE_IMAGEINFO_SUPPORT), COL_MENUCONTENT, 0, true);
	
	frameBuffer->paintIcon("gelb.raw",(xpos + xIconOffset*2) ,yIconPos+3); //Revision
	g_Font[font_small]->RenderString(xpos+(xIconOffset*2)+20, yIconPos+22, xIconOffset, g_Locale->getText(LOCALE_IMAGEINFO_DETAILS), COL_MENUCONTENT, 0, true);
	
	frameBuffer->paintIcon("blau.raw",(xpos + xIconOffset*3) ,yIconPos+3); //Partitions
	g_Font[font_small]->RenderString(xpos+(xIconOffset*3)+20, yIconPos+22, xIconOffset, g_Locale->getText(LOCALE_IMAGEINFO_PARTITIONS), COL_MENUCONTENT, 0, true);

	frameBuffer->paintIcon("home.raw",(xpos + xIconOffset*4) ,yIconPos-3); //Home 
	g_Font[font_small]->RenderString(xpos+(xIconOffset*4)+36, yIconPos+22, xIconOffset, g_Locale->getText(LOCALE_MENU_BACK), COL_MENUCONTENT, 0, true);
		
}

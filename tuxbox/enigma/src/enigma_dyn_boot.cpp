/*
 * $Id: enigma_dyn_boot.cpp,v 1.17 2005/10/26 19:27:03 digi_casi Exp $
 *
 * (C) 2005 by digi_casi <digi_casi@tuxbox.org>
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
 
#ifdef ENABLE_EXPERT_WEBIF
#include <map>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/vfs.h>
#include <net/if.h>
#include <sys/mount.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <lib/system/info.h>
#include <lib/system/http_dyn.h>
#include <enigma_dyn_utils.h>
#include <enigma_dyn_boot.h>
#include <bootmenue/bmconfig.h>
#define INSTIMAGESUPPORT
#include <bootmenue/bmimage.h>
#include <bootmenue/bmboot.h>

extern eString firmwareLevel(eString versionString);

using namespace std;

eString getSkins(eString skinPath, eString skinName)
{	
	bmboot bmgr;
	eString skins;
	
	bmgr.getSkins(skinPath);
	for (unsigned int i = 0; i < bmgr.skinList.size(); i++)
	{
		eString name = bmgr.skinList[i];
		eString location = skinPath + "/" + name;
		
		skins = skins + "<option value=\"" + location + "\"" + eString((skinName == name) ? " selected" : "") + ">" + name + "</option>";
	}

	if (!skins)
		skins = "<option value=\"" + skinPath + "\">none</option>";
	
	return skins;
}

eString getMenus()
{
	bool bm = false;
	bool fw = false;
	bool fwInstalled = false;
	eString line;
	eString result;
	bmboot bmgr;
	
	bmgr.mountJFFS2();
		
	ifstream initFile ("/tmp/jffs2/etc/init");
	if (initFile)
	{
		while (getline(initFile, line, '\n'))
		{
			if (line.find("fwpro") != eString::npos)
			{
				int pos = line.find_first_not_of(" ");
				fw = line[pos] != '#' && line[pos] != ':';
				fwInstalled = true;
			}
			if (line.find("bm.sh") != eString::npos)
			{
				int pos = line.find_first_not_of(" ");
				bm = line[pos] != '#' && line[pos] != ':';
			}
		}
	}
	initFile.close();
	
	if (bm && fw)
	{
		bmgr.activateMenu("BM");
		fw = false;
	}
	
	eString menus = "<option value=\"none\">none</option>";
	menus += "<option value=\"BM\"" + eString((bm) ? " selected" : "") + ">BootManager</option>";
	if (fwInstalled)
		menus += "<option value=\"FW\"" + eString((fw) ? " selected" : "") + ">FlashWizard</option>";
	result = readFile(TEMPLATE_DIR + "bootMenus.tmp");
	result.strReplace("#OPTIONS#", menus);
	if (bm)
		result.strReplace("#BMSETTINGSBUTTON#", button(100, "Settings", TOPNAVICOLOR, "javascript:editBootManagerSettings('')", "#000000"));
	else
		result.strReplace("#BMSETTINGSBUTTON#", "&nbsp;");
	
	bmgr.unmountJFFS2();
		
	return result;
}

eString getInstalledImages()
{
	eString images;
	bmconfig cfg;
	bmimages imgs;
	
	cfg.load();
	imgs.load(cfg.mpoint, true);
	
	for (unsigned int i = 0; i < imgs.imageList.size(); i++)
	{
		eString name = imgs.imageList[i].name;
		eString location = imgs.imageList[i].location;
		eString image = readFile(TEMPLATE_DIR + "image.tmp");
		image.strReplace("#NAME#", name);
		eString version = firmwareLevel(getAttribute(location + "/.version", "version"));
		image.strReplace("#VERSION#", version);
		if (location)
		{
			image.strReplace("#LOCATION#", location);
			image.strReplace("#SETTINGSBUTTON#", button(80, "Settings", GREEN, "javascript:editImageSettings('" + location + "')", "#FFFFFF"));
			image.strReplace("#DELETEBUTTON#", button(80, "Delete", RED, "javascript:deleteImage('" + location + "')", "#FFFFFF"));
		}
		else
		{
			image.strReplace("#LOCATION#", "&nbsp;");
			image.strReplace("#SETTINGSBUTTON#", "&nbsp;");
			image.strReplace("#DELETEBUTTON#", "&nbsp;");
		}
		images += image;
	}
	
	if (!images)
		images = "<tr><td colspan=\"5\">No images found on selected boot device.</td></tr>";

	return images;
}

eString getConfigBoot(void)
{
	eString result = readFile(TEMPLATE_DIR + "bootMgr.tmp");
	result.strReplace("#MENU#", getMenus());
	result.strReplace("#IMAGES#", getInstalledImages());
	result.strReplace("#ADDIMAGEBUTTON#", button(100, "Add", GREEN, "javascript:showAddImageWindow('')", "#FFFFFF"));

	return result;
}

eString installImage(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	bmimages imgs;
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString sourceImage = opt["image"];
	eString mountDir = opt["target"];
	eString imageName = opt["name"];
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	imgs.add(sourceImage, imageName, mountDir);
	
	return closeWindow(content, "", 10);
}

eString deleteImage(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString image = opt["image"];
	
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	system(eString("rm -rf " + image).c_str());
	
	return closeWindow(content, "", 10);
}

eString editImageSettings(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString image = opt["image"];
	
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	return "function not available yet.";
}


eString setImageSettings(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	return closeWindow(content, "", 10);
}

eString editBootManagerSettings(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	
	content->local_header["Content-Type"]="text/html; charset=utf-8";

	bmconfig cfg;
	bmboot bmgr;
	
	bmgr.mountJFFS2();
	cfg.load();
	bmgr.unmountJFFS2();
	
	eString result = readFile(TEMPLATE_DIR + "bootMgrSettings.tmp");
	result.strReplace("#SKINOPTIONS#", getSkins(SKINDIR, cfg.skinName));
	result.strReplace("#MPOINT#", cfg.mpoint);
	result.strReplace("#SELECTEDENTRY#", cfg.selectedEntry);
	result.strReplace("#INETD#", cfg.inetd);
	result.strReplace("#TIMEOUTVALUE#", cfg.timeoutValue);
	result.strReplace("#VIDEOFORMAT#", cfg.videoFormat);
	result.strReplace("#SKINPATH#", cfg.skinPath);
	result.strReplace("#SKINNAME#", cfg.skinName);
	
	result.strReplace("#BUTTONSUBMIT#", button(100, "Change", TOPNAVICOLOR, "javascript:submitSettings()", "#000000"));
	
	return result;
}

eString setBootManagerSettings(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	bmconfig cfg;
	bmboot bmgr;
	
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	cfg.mpoint = opt["mpoint"];
	cfg.selectedEntry = opt["selectedEntry"];
	cfg.inetd = opt["inetd"];
	cfg.timeoutValue = opt["timeoutValue"];
	cfg.videoFormat = opt["videoFormat"];
	cfg.skinPath = opt["skinPath"];
	cfg.skinName = opt["skinName"];
	unsigned int pos = cfg.skinName.find_last_of("/");
	if (pos != eString::npos && pos > 0)
		cfg.skinName = cfg.skinName.right(cfg.skinName.length() - pos - 1);
	
	bmgr.mountJFFS2();
	cfg.save();
	bmgr.unmountJFFS2();
	
	return closeWindow(content, "", 10);
}

eString selectBootMenu(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	bmboot bmgr;
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString menu = opt["menu"];
	
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	bmgr.activateMenu(menu);
	
	return closeWindow(content, "", 10);
}

eString showAddImageWindow(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
//	return closeWindow(content, "", 10);
	return "function not available yet.";
}

void ezapBootManagerInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/cgi-bin/installimage", installImage, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/deleteimage", deleteImage, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/editimagesettings", editImageSettings, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/setimagesettings", setImageSettings, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/selectbootmenu", selectBootMenu, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/showaddimagewindow", showAddImageWindow, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/editbootmanagersettings", editBootManagerSettings, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/setbootmanagersettings", setBootManagerSettings, lockWeb);
}
#endif

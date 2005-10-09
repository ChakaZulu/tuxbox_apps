/*
 * $Id: enigma_dyn_boot.cpp,v 1.11 2005/10/09 08:30:28 digi_casi Exp $
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
 
#ifdef ENABLE_DYN_BOOT
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

extern eString firmwareLevel(eString versionString);

using namespace std;

#define SKINDIR "/var/tuxbox/config/enigma/boot"

void mountJFFS2()
{
	system("umount /tmp/jffs2");
	system("mkdir /tmp/jffs2");
	system("mount -t jffs2 /dev/mtdblock/1 /tmp/jffs2");
}

void unmountJFFS2()
{
	system("umount /tmp/jffs2");
	system("rm -rf /tmp/jffs2");
}

eString getSkins(eString skinPath, eString skinName)
{	
	eString skins;
	mountJFFS2();
	
	if (skinPath.find("/var") == 0)
	{
		skinPath = "/tmp/jffs2" + skinPath.right(skinPath.length() - 4);
		DIR *d = opendir(skinPath.c_str());
		if (d)
		{
			while (struct dirent *e = readdir(d))
			{
				if (strcmp(e->d_name, ".") && strcmp(e->d_name, ".."))
				{
					eString location = skinPath + "/" + eString(e->d_name);
					eString name = eString(e->d_name);
					
					if (location.right(5) == ".skin")
					{
						if (location.find("/tmp/jffs2") == 0)
							location = "/var" + location.right(location.length() - 10);
						skins = skins + "<option value=\"" + location + "\"" + eString((skinName == name) ? " selected" : "") + ">" + name + "</option>";
					}
				}
			}
			closedir(d);
		}
	}
	unmountJFFS2();
	
	if (!skins)
		skins = "<option value=\"" + skinPath + "\">none</option>";
	
	return skins;
}

void activateMenu(eString menu)
{
	bool bm = (menu == "BM");
	bool fw = (menu == "FW");
	bool found = false;
	bool active = false;
	bool initChanged = false;
	eString line;
	eString file;
	
	mountJFFS2();
	
	ifstream initFile ("/tmp/jffs2/etc/init");
	if (initFile)
	{
		while (getline(initFile, line, '\n'))
		{
			if (line.find("fwpro") != eString::npos)
			{
				int pos = line.find_first_not_of(" ");
				active = (line[pos] != '#' && line[pos] != ':');
				if (fw)
				{
					if (!active)
					{
						line[pos] = ' ';
						initChanged = true;
					}
					found = true;
				}
				else
				{
					if (active)
					{
						if (pos > 1)
							line[pos - 2] = ':';
						else
							line = ": " + line;
						initChanged = true;	
					}
				}
			}
			if (line.find("bm.sh") != eString::npos)
			{
				int pos = line.find_first_not_of(" ");
				active = (line[pos] != '#' && line[pos] != ':');
				if (bm)
				{
					if (!active)
					{
						line[pos] = ' ';
						initChanged = true;
					}
					found = true;
				}
				else
				{
					if (active)
					{
						if (pos > 1)
							line[pos - 2] = ':';
						else
							line = ": " + line;	
						initChanged = true;
					}
				}
			}
			if (file)
				file += "\n" + line;
			else
				file = line;
		}
	}
	initFile.close();
	
	if (!found)
	{
		if (bm)
			file += "\n/bin/bootmenue && /tmp/bm.sh";
	}
	
	if (initChanged)
	{
		FILE *out = fopen("/tmp/jffs2/etc/init", "w");
		fprintf(out, file.c_str());
		fclose(out);
		system("chmod +x /tmp/jffs2/etc/init");
	}
	
	unmountJFFS2();
}

eString getMenus()
{
	bool bm = false;
	bool fw = false;
	bool fwInstalled = false;
	eString line;
	eString result;
	
	mountJFFS2();
		
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
		activateMenu("BM");
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
	
	unmountJFFS2();
		
	return result;
}

eString getInstalledImages()
{
	eString images;
	struct stat s;
	
	eString dir[2] = {"/mnt/usb/image/", "/mnt/usb/fwpro/"};
	
	for (int i = 0; i < 2; i++)
	{
		DIR *d = opendir(dir[i].c_str());
		if (d)
		{
			while (struct dirent *e = readdir(d))
			{
				if (strcmp(e->d_name, ".") && strcmp(e->d_name, ".."))
				{
					eString location = dir[i] + eString(e->d_name);
					stat(location.c_str(), &s);
					if (S_ISDIR(s.st_mode))
					{
						eString name = e->d_name;
						eString tmp = name + "/imagename";
						ifstream nameFile(tmp.c_str());
						if (nameFile)
						{
							eString line;
							getline(nameFile, line, '\n');
							nameFile.close();
							if (line.length() > 0)
								name = line;
						}
	
						eString image = readFile(TEMPLATE_DIR + "image.tmp");
						image.strReplace("#NAME#", name);
						image.strReplace("#LOCATION#", location);
						eString version = firmwareLevel(getAttribute(location + "/.version", "version"));
						image.strReplace("#VERSION#", version);
						image.strReplace("#SETTINGSBUTTON#", button(80, "Settings", GREEN, "javascript:editImageSettings('" + location + "')", "#FFFFFF"));
						image.strReplace("#DELETEBUTTON#", button(80, "Delete", RED, "javascript:deleteImage('" + location + "')", "#FFFFFF"));
						images += image;
					}
				}
			}
			closedir(d);
		}
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

int unpackImage(eString sourceImage, eString imageName, eString mountDir)
{
	int freeSpace = 0;
	eString imageDir = mountDir + "/fwpro/" + imageName;
	
	static int lastpid = -1;

	eDebug("[BOOTMANAGER] unpackImage: installation device = %s, image file = %s, image dir = %s", mountDir.c_str(), sourceImage.c_str(), imageDir.c_str());
	
	if (lastpid != -1)
	{
		kill(lastpid, SIGKILL);
		waitpid(lastpid, 0, 0);
		lastpid = -1;
	}
	
	// check if enough space is available
	struct statfs s;
	if (statfs(mountDir.c_str(), &s) >= 0) 
		freeSpace = (s.f_bavail * (s.f_bsize / 1024));
	eDebug("[BOOTMANAGER] unpackImage: free space on device = %d", freeSpace);
	if (freeSpace < 20000)
		return -1;
		
	if (access(mountDir.c_str(), W_OK) != 0)
		return -2;

	// check if directory is available, delete it and recreate it
	if (access(imageDir.c_str(), W_OK) == 0)
		system(eString("rm -rf " + imageDir).c_str());
	system(eString("mkdir " + imageDir + " --mode=777 --parents").c_str());
	if (access(imageDir.c_str(), W_OK) != 0)
		return -3;
	
	// split image file
	eString squashfsPart = mountDir + "/squashfs.img";
	remove(eString(squashfsPart + "/squashfs.img").c_str());
	if (system(eString("dd if=" + sourceImage + " of=" + squashfsPart + " bs=1024 skip=1152 count=4992").c_str()) >> 8)
		return -4;
	eString cramfsPart = mountDir + "/cramfs.img";
	remove(eString(cramfsPart + "/cramfs.img").c_str());
	if (system(eString("dd if=" + sourceImage + " of=" + cramfsPart + " bs=1024 skip=0 count=1152").c_str()) >> 8)
		return -5;
	remove(sourceImage.c_str());
	
	switch (lastpid = fork())
	{
		case -1:
			eDebug("[BOOTMANAGER] fork failed.");
			return -6;
		case 0:
		{
			eDebug("[BOOTMANAGER] unpackImage: forked.");
			for (unsigned int i = 0; i < 90; ++i )
				close(i);

			// mount squashfs part of the image
			system("rm -rf /tmp/image");
			system("mkdir /tmp/image");
			if (system(eString("mount -t squashfs " + squashfsPart + " /tmp/image -o loop").c_str()) >> 8)
			{
				remove(squashfsPart.c_str());
				eDebug("[BOOTMANAGER] mounting squashfs image part failed.");
				_exit(-6);
			}
			// copy files
			if (system(eString("cp -rd /tmp/image/* " + imageDir).c_str()) >> 8)
			{
				system("umount /tmp/image");
				remove(squashfsPart.c_str());
				eDebug("[BOOTMANAGER] copying squashfs image part failed.");
				_exit(-7);
			}
			
			eDebug("[BOOTMANAGER] unpackImage: squashfs installed.");
			
			// unmount squashfs part of the image
			system("umount /tmp/image");
			remove(squashfsPart.c_str());
			
			// mount cramfs part of the image
			if (system(eString().sprintf("mount -t cramfs %s /tmp/image -o loop", cramfsPart.c_str()).c_str()) >> 8)
			{
				remove(cramfsPart.c_str());
				eDebug("[BOOTMANAGER] mounting cramfs image part failed.");
				_exit(-8);
			}
			// copy files
			if (system(eString("cp -rd /tmp/image/* " + imageDir).c_str()) >> 8)
			{
				system("umount /tmp/image");
				remove(cramfsPart.c_str());
				eDebug("[BOOTMANAGER] copying cramfs image part failed.");
				_exit(-9);
			}
			// unmount cramfs part of the image
			system("umount /tmp/image");
			remove(cramfsPart.c_str());
			
			eDebug("[BOOTMANAGER] unpackImage: cramfs installed.");
	
			// delete temp mount dir
			system("rm -rf /tmp/image");
			
			// construct /var
			system(eString("rm -rf " + imageDir + "/var " + imageDir + "/var_init/tmp/init").c_str());
			system(eString("mv " + imageDir + "/var_init " + imageDir + "/var").c_str());
			system(eString("touch " + imageDir + "/var/.init").c_str());
			
			eDebug("[BOOTMANAGER] unpackImage: /var prepared.");
			
			// remove jffs2 mounting from rcS
			system(eString("mv " + imageDir + "/etc/init.d/rcS " + imageDir + "/etc/init.d/rcS.org").c_str());
			system(eString("sed 's?/bin/mount -t jffs2 /dev/mtdblock/1 /var?#/bin/mount -t jffs2 /dev/mtdblock/1 /var?g' " + imageDir + "/etc/init.d/rcS.org > " + imageDir + "/etc/init.d/rcS").c_str());
			system(eString("chmod +x " + imageDir + "/etc/init.d/rcS").c_str());
			
			eDebug("[BOOTMANAGER] installing image completed successfully.");
	
			_exit(0);
			break;
		}
	}
	
	return 0;
}

eString installImage(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString sourceImage = opt["image"];
	eString mountDir = opt["target"];
	eString imageName = opt["name"];
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	unpackImage(sourceImage, imageName, mountDir);
	
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

	bmconfig *config = new bmconfig();
	
	mountJFFS2();
	config->load();
	unmountJFFS2();
	
	eString result = readFile(TEMPLATE_DIR + "bootMgrSettings.tmp");
	result.strReplace("#SKINOPTIONS#", getSkins(SKINDIR, config->skinName));
	result.strReplace("#MPOINT#", config->mpoint);
	result.strReplace("#SELECTEDENTRY#", config->selectedEntry);
	result.strReplace("#INETD#", config->inetd);
	result.strReplace("#TIMEOUTVALUE#", config->timeoutValue);
	result.strReplace("#VIDEOFORMAT#", config->videoFormat);
	result.strReplace("#SKINPATH#", config->skinPath);
	result.strReplace("#SKINNAME#", config->skinName);
	
	result.strReplace("#BUTTONSUBMIT#", button(100, "Change", TOPNAVICOLOR, "javascript:submitSettings()", "#000000"));
	
	return result;
}

eString setBootManagerSettings(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	bmconfig *config;
	
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	config = new bmconfig();
	config->mpoint = opt["mpoint"];
	config->selectedEntry = opt["selectedEntry"];
	config->inetd = opt["inetd"];
	config->timeoutValue = opt["timeoutValue"];
	config->videoFormat = opt["videoFormat"];
	config->skinPath = opt["skinPath"];
	config->skinName = opt["skinName"];
	unsigned int pos = config->skinName.find_last_of("/");
	if (pos != eString::npos && pos > 0)
		config->skinName = config->skinName.right(config->skinName.length() - pos - 1);
	
	mountJFFS2();
	config->save();
	unmountJFFS2();
	
	return closeWindow(content, "", 10);
}

eString selectBootMenu(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString menu = opt["menu"];
	
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	activateMenu(menu);
	
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

#endif // ENABLE_DYN_BOOT

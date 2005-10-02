/*
 * $Id: enigma_dyn_boot.cpp,v 1.1 2005/10/02 13:20:38 digi_casi Exp $
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

extern eString firmwareLevel(eString versionString);

using namespace std;

void activateMenu(eString menu)
{
	bool bm = (menu == "BM");
	bool fw = (menu == "FW");
	bool found = false;
	bool active = false;
	eString line;
	eString file;
	
	ifstream initFile ("/var/etc/init");
	if (initFile)
	{
		while (getline(initFile, line, '\n'))
		{
			if (line.find("fwpro"))
			{
				int pos = line.find_first_not_of(" ");
				active = (line[pos] != '#' && line[pos] != ':');
				if (fw)
				{
					if (!active)
						line[pos] = ' ';
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
					}
				}
			}
			if (line.find("bm.sh"))
			{
				int pos = line.find_first_not_of(" ");
				active = (line[pos] != '#' && line[pos] != ':');
				if (bm)
				{
					if (!active)
						line[pos] = ' ';
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
					}
				}
			}
			file += line;
		}
	}
	initFile.close();
	
	if (!found)
	{
		if (bm)
			file = "/bin/bootmenue && /tmp/bm.sh\n" + file;
	}
	
	FILE *out = fopen("/var/etc/init", "w");
	fprintf(out, file.c_str());
	fclose(out);
}

eString getMenus(bool *bootmanager)
{
	bool bm = false;
	bool fw = false;
	eString line;
	eString result;
	
//	if (!access("/root", R_OK))
	{
		ifstream initFile ("/var/etc/init");
		if (initFile)
		{
			while (getline(initFile, line, '\n'))
			{
				if (line.find("fwpro"))
				{
					int pos = line.find_first_not_of(" ");
					fw = line[pos] != '#' && line[pos] != ':';
				}
				if (line.find("bm.sh"))
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
	
		result = readFile(TEMPLATE_DIR + "bootMenus.tmp");
		eString menus = "<option value=\"BM\"" + eString((bm) ? " selected" : "") + ">BootManager</option>";
		menus += "<option value=\"FW\"" + eString((fw) ? " selected" : "") + ">FlashWizzard</option>";
		result.strReplace("#OPTIONS#", menus);
	}
//	else
//		result.strReplace("#OPTIONS#", "Boot menu can only be selected when flash image is active.");
	
	*bootmanager = bm;
		
	return result;
}

eString getInstalledImages()
{
	eString images;
	struct stat s;
	
	std::string dir[2];
	dir[0] = "/mnt/usb/image/";
	dir[1] = "/mnt/usb/fwpro/";
	
	for (int i = 0; i < 2; i++)
	{
		DIR *d = opendir(dir[i].c_str());
		if (d)
		{
			while (struct dirent *e = readdir(d))
			{
				if (strcmp(e->d_name, ".") && strcmp(e->d_name, ".."))
				{
					std::string name = dir[i] + e->d_name;
					stat(name.c_str(), &s);
					if (S_ISDIR(s.st_mode))
					{
						eString tmp = e->d_name;
						eString location = name;
						eString name = e->d_name;
						tmp = name + "/imagename";
						if (FILE *in = fopen(tmp.c_str(), "rt"))
						{
							char line[256];
							line[0] = '\0';
							fgets(line, 256, in);
							fclose(in);
							if (strlen(line) > 0)
							{
								line[strlen(line) - 1] = '\0';
								name = std::string(line);
							}
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

	return images;
}

eString getConfigBoot(void)
{
	bool bm = false;
	eString result = readFile(TEMPLATE_DIR + "bootMgr.tmp");
	result.strReplace("#MENU#", getMenus(&bm));
	result.strReplace("#IMAGES#", getInstalledImages());
	
	if (bm)
		result.strReplace("#BMSETTINGSBUTTON#", button(100, "Settings", GREEN, "javascript:editBootManagerSettings('')", "#FFFFFF"));
	else
		result.strReplace("#BMSETTINGSBUTTON#", "");

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

void ezapBootManagerInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/cgi-bin/installimage", installImage, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/deleteimage", deleteImage, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/editimagesettings", editImageSettings, lockWeb);
}

#endif // ENABLE_DYN_BOOT

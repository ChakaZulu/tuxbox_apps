#ifdef ENABLE_DYN_CONF
#ifndef DISABLE_FILE

#include <map>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <iomanip>
#include <iostream>
#include <fstream>

#include <enigma.h>
#include <enigma_main.h>
#include <enigma_standby.h>
#include <timer.h>
#include <lib/driver/eavswitch.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/epgcache.h>
#include <lib/dvb/servicestructure.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/service.h>
#include <lib/dvb/record.h>
#include <lib/dvb/serviceplaylist.h>

#include <lib/system/info.h>
#include <lib/system/http_dyn.h>
#include <lib/system/econfig.h>
#include <enigma_dyn.h>
#include <enigma_dyn_utils.h>
#include <enigma_dyn_conf.h>
#include <configfile.h>

using namespace std;

#if 0
eString getImageMediaPath(void)
{
	eString mediaPath;
	if (access("/tmp/org", R_OK) == 0)
		mediaPath = getAttribute("/tmp/org", "mpoint");
	else
		mediaPath = getAttribute("/Image.info", "mpoint");
	return mediaPath;
}

bool dreamFlashIsInstalled(void)
{
	eString mediaPath = getImageMediaPath();
	return ((access(eString(mediaPath + "/tools/lcdmenu.conf").c_str(), R_OK) == 0)
		&& (access(eString(mediaPath + "/tools/lcdmenu").c_str(), X_OK) == 0)
		&& (access(eString(mediaPath + "/tools/menu").c_str(), X_OK) == 0)
		);	
}

eString getInstalledImages(void)
{
	eString result;
	eString image;
	unsigned int pos = 0;
	int i = 0;
	eString mediaPath = getImageMediaPath();
	eString dreamFlashImages = getAttribute(mediaPath + "/tools/lcdmenu.conf", "menu_items");
	eString activeImage = getAttribute(mediaPath + "/tools/lcdmenu.conf", "default_entry");
	if (dreamFlashImages.length() > 0)
		dreamFlashImages = dreamFlashImages.substr(0, dreamFlashImages.length() - 1); //remove last comma
	while (dreamFlashImages.length() > 0)
	{
		if ((pos = dreamFlashImages.find(",")) != eString::npos)
		{
			image = dreamFlashImages.substr(0, pos);
			dreamFlashImages = dreamFlashImages.substr(pos + 1);
		}
		else
		{
			image = dreamFlashImages;
			dreamFlashImages = "";
		}
		result += "<tr>";
		result += "<td>";
		if (i == atoi(activeImage.c_str()))
			result += "<img src=\"on.gif\" alt=\"online\" border=0>";
		else
			result += "<img src=\"off.gif\" alt=\"offline\" border=0>";
		result += "</td>";
		result += "<td>";
		if (i != atoi(activeImage.c_str()))
			result += button(100, "Select", GREEN, "javascript:selectImage('" + eString().sprintf("%d", i) + "')", "#FFFFFF");
		else
			result += "&nbsp;";
		result += "</td>";
		result += "<td>";
		result += image;
		result += "</td>";
		result += "</tr>";
		i++;
	}
	if (result == "")
		result = "<tr><td>none</td></tr>";
	
	return result;
}
#endif

void deactivateSwapFile(eString swapFile)
{
	eString cmd;
	cmd = "swapoff " + swapFile;
	system(cmd.c_str());
}

void activateSwapFile(eString swapFile)
{
	eString procswaps = readFile("/proc/swaps");
	eString th1, th2, th3, th4, th5;
	eString td1, td2, td3, td4, td5;
	std::stringstream tmp;
	tmp.str(procswaps);
	tmp >> th1 >> th2 >> th3 >> th4 >> th5 >> td1 >> td2 >> td3 >> td4 >> td5;
	if ((td1 != "") && (td1 != swapFile))
		deactivateSwapFile(td1);
	if ((td1 == "") || (td1 != swapFile))
	{
		eString cmd = "mkswap " + swapFile;
		system(cmd.c_str());
		cmd = "swapon " + swapFile;
		system(cmd.c_str());
	}
}

void setSwapFile(int nextswapfile, eString nextswapfilename)
{
	eConfig::getInstance()->setKey("/extras/swapfile", nextswapfile);
	if (nextswapfile == 1)
	{
		eConfig::getInstance()->setKey("/extras/swapfilename", nextswapfilename.c_str());
		activateSwapFile(nextswapfilename);
	}
	else
		deactivateSwapFile(nextswapfilename);
}

eString setConfigSwapFile(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString swap = opt["swap"];
	eString swapFile = opt["swapfile"];

	setSwapFile((swap == "on") ? 1 : 0, swapFile);

	return closeWindow(content, "", 500);
}

eString setConfigMultiBoot(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString imageNumber = opt["image"];

	CConfigFile *config = new CConfigFile(',');
	if (config->loadConfig("/var/mnt/usb/tools/lcdmenu.conf"))
	{
		config->setString("default_entry", imageNumber);
		config->setModifiedFlag(true);
		config->saveConfig("/var/mnt/usb/tools/lcdmenu.conf");

		delete(config);
	}

	return closeWindow(content, "", 500);
}

void initHDDparms(void)
{
#ifndef DISABLE_FILE
	eString cmd;
	int ti = 0, ac = 0;

	eConfig::getInstance()->getKey("/extras/hdparm-s", ti);
	eConfig::getInstance()->getKey("/extras/hdparm-m", ac);
	if (ac)
	{
		cmd.sprintf("hdparm -S %d /dev/ide/host0/bus0/target0/lun0/disc", ti);
		system(cmd.c_str());
	}
	if (ti)
	{
		cmd.sprintf("hdparm -M %d /dev/ide/host0/bus0/target0/lun0/disc", ac);
		system(cmd.c_str());
	}
#endif
}

eString setConfigSettings(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString maxmtu = opt["maxmtu"];
	eString samba = opt["samba"];
	eString hddti = opt["hddstandby"];
	eString hddac = opt["hddacoustics"];
	eString timeroffset = opt["timeroffset"];
	eString showsatpos = opt["showsatpos"];
	eString webiflock = opt["webiflock"];
	
	int oldti = 0;
	eConfig::getInstance()->getKey("/extras/hdparm-s", oldti);
	int oldac = 0;
	eConfig::getInstance()->getKey("/extras/hdparm-m", oldac);
	
//	eConfig::getInstance()->setKey("/extras/fastshutdown", (fastshutdown == "on" ? 1 : 0));
	eConfig::getInstance()->setKey("/elitedvb/network/samba", (samba == "on" ? 1 : 0));
	eConfig::getInstance()->setKey("/ezap/webif/webIfLock", (webiflock == "on" ? 1 : 0));
	eConfig::getInstance()->setKey("/extras/showSatPos", (showsatpos == "on" ? 1 : 0));
	eConfig::getInstance()->setKey("/enigma/timeroffset", atoi(timeroffset.c_str()));
	eConfig::getInstance()->setKey("/elitedvb/network/maxmtu", atoi(maxmtu.c_str()));
	system(eString("/sbin/ifconfig eth0 mtu " + maxmtu).c_str());
	if ((atoi(hddti.c_str()) * 12) != oldti)
		eConfig::getInstance()->setKey("/extras/hdparm-s", atoi(hddti.c_str()) * 12);
	if (atoi(hddac.c_str()) != oldac)
		eConfig::getInstance()->setKey("/extras/hdparm-m", atoi(hddac.c_str()));
	initHDDparms();

	return closeWindow(content, "", 500);
}

void ezapConfInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/cgi-bin/setConfigSwapFile", setConfigSwapFile, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/setConfigMultiBoot", setConfigMultiBoot, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/setConfigSettings", setConfigSettings, lockWeb);
}
#endif
#endif

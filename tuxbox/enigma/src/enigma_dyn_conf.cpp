/*
 * $Id: enigma_dyn_conf.cpp,v 1.25 2009/07/22 18:23:58 dbluelle Exp $
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

eString getConfigSwapFile(void)
{
	eString result;
	eString th1, th2, th3, th4, th5;
	eString td1, td2, td3, td4, td5;

	int swapfile = 0;
	eString procswaps = readFile("/proc/swaps");
	if (procswaps)
	{
		result = readFile(TEMPLATE_DIR + "configSwapFile.tmp");
		std::stringstream tmp;
		tmp.str(procswaps);
		tmp >> th1 >> th2 >> th3 >> th4 >> th5 >> td1 >> td2 >> td3 >> td4 >> td5;
		
		result.strReplace("#TH1#", th1);
		result.strReplace("#TH2#", th2);
		result.strReplace("#TH3#", th3);
		result.strReplace("#TH4#", th4);
		result.strReplace("#TH5#", th5);
		result.strReplace("#TD1#", td1);
		result.strReplace("#TD2#", td2);
		result.strReplace("#TD3#", td3);
		result.strReplace("#TD4#", td4);
		result.strReplace("#TD5#", td5);
	}
	else
		result = "No swap file active.";
	
	result += readFile(TEMPLATE_DIR + "configSwapFileMenu.tmp");
	eConfig::getInstance()->getKey("/extras/swapfile", swapfile);
	char *swapfilename = 0;
	eConfig::getInstance()->getKey("/extras/swapfilename", swapfilename);
	result.strReplace("#SWAP#", (swapfile == 1) ? "checked" : "");
	eString rpl;
	if (swapfilename)
	{
		rpl = swapfilename;
		free(swapfilename);
	}
	result.strReplace("#SWAPFILE#", rpl);
	result.strReplace("#SWAPFILEBUTTON#", button(100, "Configure", NOCOLOR, "javascript:configSwapFile()", "#000000"));
	
	return result;
}

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
	system("echo 0 > /proc/sys/vm/swappiness");
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
	
	if (swap == "on")
	{
		if (access(swapFile.c_str(), W_OK) != 0)
			system(eString("dd if=/dev/zero of=" + swapFile + " bs=1024 count=32768").c_str());
	}

	setSwapFile((swap == "on") ? 1 : 0, swapFile);

	return closeWindow(content, "", 500);
}

void initHDDparms(void)
{
#ifndef DISABLE_FILE
	eString cmd;
	int ti = 0, ac = 0;

	eConfig::getInstance()->getKey("/extras/hdparm-s", ti);
	eConfig::getInstance()->getKey("/extras/hdparm-m", ac);
	if (ti)
	{
		cmd.sprintf("hdparm -S %d /dev/ide/host0/bus0/target0/lun0/disc", ti);
		system(cmd.c_str());
	}
	if (ac)
	{
		cmd.sprintf("hdparm -M %d /dev/ide/host0/bus0/target0/lun0/disc", ac);
		system(cmd.c_str());
	}
#endif
}

eString getConfigSettings(void)
{
	eString result = readFile(TEMPLATE_DIR + "configSettings.tmp");
	int fastshutdown = 0;
	eConfig::getInstance()->getKey("/extras/fastshutdown", fastshutdown);
	int showSatPos = 1;
	eConfig::getInstance()->getKey("/extras/showSatPos", showSatPos);
	result.strReplace("#SHOWSATPOS#", (showSatPos == 1) ? "checked" : "");
	int timeroffset = 0;
	eConfig::getInstance()->getKey("/enigma/timeroffset", timeroffset);
	result.strReplace("#TIMEROFFSET#", eString().sprintf("%d", timeroffset));
	int timeroffset2 = 0;
	eConfig::getInstance()->getKey("/enigma/timeroffset2", timeroffset2);
	result.strReplace("#TIMEROFFSET2#", eString().sprintf("%d", timeroffset2));
	int defaultaction = 0;
	eConfig::getInstance()->getKey("/enigma/timerenddefaultaction", defaultaction);
	switch(defaultaction)
	{
		default:
		case 0:
			result.strReplace("#TIMERENDDEFAULTACTION#", eString().sprintf("<option selected value=\"0\">Nothing</option><option value=\"%d\">Standby</option><option value=\"%d\">Shutdown</option>",ePlaylistEntry::doGoSleep,ePlaylistEntry::doShutdown));
			break;
		case ePlaylistEntry::doGoSleep:
			result.strReplace("#TIMERENDDEFAULTACTION#", eString().sprintf("<option value=\"0\">Nothing</option><option selected value=\"%d\">Standby</option><option value=\"%d\">Shutdown</option>",ePlaylistEntry::doGoSleep,ePlaylistEntry::doShutdown));
			break;
		case ePlaylistEntry::doShutdown:
			result.strReplace("#TIMERENDDEFAULTACTION#", eString().sprintf("<option value=\"0\">Nothing</option><option value=\"%d\">Standby</option><option selected value=\"%d\">Shutdown</option>",ePlaylistEntry::doGoSleep,ePlaylistEntry::doShutdown));
			break;
	}
	int maxmtu = 1500;
	eConfig::getInstance()->getKey("/elitedvb/network/maxmtu", maxmtu);
	result.strReplace("#MAXMTU#", eString().sprintf("%d", maxmtu));
	int samba = 1;
	eConfig::getInstance()->getKey("/elitedvb/network/samba", samba);
	result.strReplace("#SAMBA#", (samba == 1) ? "checked" : "");
	int webLock = 1;
	eConfig::getInstance()->getKey("/ezap/webif/lockWebIf", webLock);
	result.strReplace("#WEBIFLOCK#", (webLock == 1) ? "checked" : "");
	int hddti = 24;
	eConfig::getInstance()->getKey("/extras/hdparm-s", hddti);
	result.strReplace("#HDDSTANDBY#", eString().sprintf("%d", hddti / 12));
	int hddac = 160;
	eConfig::getInstance()->getKey("/extras/hdparm-m", hddac);
	result.strReplace("#HDDACOUSTICS#", eString().sprintf("%d", hddac));
	char *audiochannelspriority=0;
	eConfig::getInstance()->getKey("/extras/audiochannelspriority", audiochannelspriority);
	eString rpl="";
	if ( audiochannelspriority )
	{
		rpl = audiochannelspriority;
		free(audiochannelspriority);
	}
	result.strReplace("#AUDIOCHANNELSPRIORITY#", rpl);
	char *trustedhosts=NULL;
	eConfig::getInstance()->getKey("/ezap/webif/trustedhosts", trustedhosts);
	rpl = "";
	if ( trustedhosts )
	{
		rpl = trustedhosts;
		free(trustedhosts);
	}
	result.strReplace("#TRUSTEDHOSTS#", rpl);
	char *epgcachepath=NULL;
	eConfig::getInstance()->getKey("/extras/epgcachepath", epgcachepath);
	rpl = "/hdd";
	if ( epgcachepath )
	{
		rpl = epgcachepath;
		free(epgcachepath);
	}
	result.strReplace("#EPGCACHEPATH#", rpl);
	
	char *epgfilename=NULL;
	eConfig::getInstance()->getKey("/extras/epgfile", epgfilename);
	rpl = "epg.dat";
	if ( epgfilename )
	{
		rpl = epgfilename;
		free(epgfilename);
	}
	result.strReplace("#EPGFILENAME#", rpl);
	
	return result;
}

eString setConfigSettings(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString maxmtu = opt["maxmtu"];
	eString samba = opt["samba"];
	eString hddti = opt["hddstandby"];
	eString hddac = opt["hddacoustics"];
	eString timeroffset = opt["timeroffset"];
	eString timeroffset2 = opt["timeroffset2"];
	eString timerenddefaultaction = opt["timerenddefaultaction"];
	eString showsatpos = opt["showsatpos"];
	eString webiflock = opt["webiflock"];
	eString audiochannelspriority = opt["audiochannelspriority"];
	eString trustedhosts = opt["trustedhosts"];
	eString epgcachepath = opt["epgcachepath"];
	eString epgfilename = opt["epgfilename"];
	eConfig::getInstance()->setKey("/ezap/webif/trustedhosts", trustedhosts.c_str());
	eConfig::getInstance()->setKey("/extras/epgcachepath", epgcachepath.c_str());
	eConfig::getInstance()->setKey("/extras/epgfile", epgfilename.c_str());

	int oldti = 0;
	eConfig::getInstance()->getKey("/extras/hdparm-s", oldti);
	int oldac = 0;
	eConfig::getInstance()->getKey("/extras/hdparm-m", oldac);

	eConfig::getInstance()->setKey("/elitedvb/network/samba", (samba == "on" ? 1 : 0));
	
	int webLock1 = 0;
	eConfig::getInstance()->getKey("/ezap/webif/lockWebIf", webLock1);
	eConfig::getInstance()->setKey("/ezap/webif/lockWebIf", (webiflock == "on" ? 1 : 0));
	int webLock2 = 0;
	eConfig::getInstance()->getKey("/ezap/webif/lockWebIf", webLock2);
	if (webLock1 != webLock2)
		eZap::getInstance()->reconfigureHTTPServer();
	eConfig::getInstance()->setKey("/extras/showSatPos", (showsatpos == "on" ? 1 : 0));
	eConfig::getInstance()->setKey("/enigma/timeroffset", atoi(timeroffset.c_str()));
	eConfig::getInstance()->setKey("/enigma/timeroffset2", atoi(timeroffset2.c_str()));
	eConfig::getInstance()->setKey("/enigma/timerenddefaultaction", atoi(timerenddefaultaction.c_str()));
	eConfig::getInstance()->setKey("/elitedvb/network/maxmtu", atoi(maxmtu.c_str()));
	system(eString("/sbin/ifconfig eth0 mtu " + maxmtu).c_str());
	if ((atoi(hddti.c_str()) * 12) != oldti)
		eConfig::getInstance()->setKey("/extras/hdparm-s", atoi(hddti.c_str()) * 12);
	if (atoi(hddac.c_str()) != oldac)
		eConfig::getInstance()->setKey("/extras/hdparm-m", atoi(hddac.c_str()));
	initHDDparms();
	eConfig::getInstance()->setKey("/extras/audiochannelspriority", audiochannelspriority.c_str());

	return closeWindow(content, "", 500);
}

void ezapConfInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/cgi-bin/setConfigSwapFile", setConfigSwapFile, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/setConfigSettings", setConfigSettings, lockWeb);
}
#endif

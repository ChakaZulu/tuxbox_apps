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

eString getConfigSwapFile(void)
{
	eString result;
	result = readFile(TEMPLATE_DIR + "configSwapFile.tmp");
	eString th1, th2, th3, th4, th5;
	eString td1, td2, td3, td4, td5;

	int swapfile = 0;
	eString procswaps = readFile("/proc/swaps");
	std::stringstream tmp;
	tmp.str(procswaps);
	tmp >> th1 >> th2 >> th3 >> th4 >> th5 >> td1 >> td2 >> td3 >> td4 >> td5;
	if (!td1)
	{
		th1 = "&nbsp;"; th2 = th3 = th4 = th5 = "&nbsp;";
		td1 = "none"; td2 = td3 = td4 = td5 = "&nbsp;";
	}
	eConfig::getInstance()->getKey("/extras/swapfile", swapfile);
	char *swapfilename=0;
	eConfig::getInstance()->getKey("/extras/swapfilename", swapfilename);
	result.strReplace("#SWAP#", (swapfile == 1) ? "checked" : "");
	eString rpl="";
	if ( swapfilename )
	{
		rpl=swapfilename;
		free(swapfilename);
	}
	result.strReplace("#SWAPFILE#", rpl);
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
	int maxmtu = 1500;
	eConfig::getInstance()->getKey("/elitedvb/network/maxmtu", maxmtu);
	result.strReplace("#MAXMTU#", eString().sprintf("%d", maxmtu));
	int samba = 1;
	eConfig::getInstance()->getKey("/elitedvb/network/samba", samba);
	result.strReplace("#SAMBA#", (samba == 1) ? "checked" : "");
	int webLock = 1;
	eConfig::getInstance()->getKey("/ezap/webif/webIfLock", webLock);
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
	eString showsatpos = opt["showsatpos"];
	eString webiflock = opt["webiflock"];
	eString audiochannelspriority = opt["audiochannelspriority"];
	
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
	eConfig::getInstance()->setKey("/extras/audiochannelspriority", audiochannelspriority.c_str());

	return closeWindow(content, "", 500);
}

void ezapConfInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/cgi-bin/setConfigSwapFile", setConfigSwapFile, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/setConfigSettings", setConfigSettings, lockWeb);
}
#endif
#endif

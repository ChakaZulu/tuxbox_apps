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

using namespace std;

void activateSwapFile(eString swapFile)
{
	eString cmd;
	cmd = "mkswap " + swapFile;
	system(cmd.c_str());
	cmd = "swapon " + swapFile;
	system(cmd.c_str());
}

void deactivateSwapFile(eString swapFile)
{
	eString cmd;
	cmd = "swapoff " + swapFile;
	system(cmd.c_str());
}

void setSwapFile(int nextswapfile, eString nextswapfilename)
{
	int curswapfile = 0;
	eConfig::getInstance()->getKey("/extras/swapfile", curswapfile);
	char *curswapfilename;
	if (eConfig::getInstance()->getKey("/extras/swapfilename", curswapfilename))
		curswapfilename = "";

	if (curswapfile != nextswapfile)
	{
		if (curswapfile != 0)
			deactivateSwapFile(eString(curswapfilename));

		if (nextswapfile != 0)
			activateSwapFile(nextswapfilename);
		else
			deactivateSwapFile(nextswapfilename);

		eConfig::getInstance()->setKey("/extras/swapfile", nextswapfile);
		eConfig::getInstance()->setKey("/extras/swapfilename", nextswapfilename.c_str());
	}
}

eString setConfigUSB(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString swapUSB = opt["swapusb"];
	eString swapUSBFile = opt["swapusbfile"];
	eString bootUSB = opt["bootUSB"];
	eString bootUSBImage = opt["bootusbimage"];

	if (swapUSB == "on")
	{
		setSwapFile(1, swapUSBFile);
	}
	else
	{
		int curswapfile = 0;
		eConfig::getInstance()->getKey("/extras/swapfile", curswapfile);
		if (curswapfile == 1)
			setSwapFile(0, swapUSBFile);
	}

	return closeWindow(content, "", 500);
}

eString setConfigHDD(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString swapHDD = opt["swaphdd"];
	eString swapHDDFile = opt["swaphddfile"];
	eString bootHDD = opt["bootHDD"];
	eString bootHDDImage = opt["boothddimage"];

	if (swapHDD == "on")
	{
		setSwapFile(2, swapHDDFile);
	}
	else
	{
		int curswapfile = 0;
		eConfig::getInstance()->getKey("/extras/swapfile", curswapfile);
		if (curswapfile == 2)
			setSwapFile(0, swapHDDFile);
	}

	return closeWindow(content, "", 500);
}

void ezapConfInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/cgi-bin/setConfigUSB", setConfigUSB, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/setConfigHDD", setConfigHDD, lockWeb);
}
#endif
#endif



#ifdef ENABLE_DYN_ROTOR

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
#include <enigma_dyn_rotor.h>
#include <configfile.h>

using namespace std;

eString getConfigRotor(void)
{
	eString result = readFile(TEMPLATE_DIR + "rotor.tmp");
	eString tmp;
	
	for (std::list<eLNB>::iterator it(eTransponderList::getInstance()->getLNBs().begin()); it != eTransponderList::getInstance()->getLNBs().end(); it++)
	{
		// go thru all satellites...
		for (ePtrList<eSatellite>::iterator s (it->getSatelliteList().begin()); s != it->getSatelliteList().end(); s++)
		{
			tmp += "<tr>";
			tmp += "<td>" + s->getDescription() + "</td>";
			tmp += "<td>" + eString().sprintf("%d", s->getOrbitalPosition()) + "</td>";
			int motorPosition = it->getDiSEqC().RotorTable[s->getOrbitalPosition()];
			tmp += "<td>" + eString().sprintf("%d", motorPosition) + "</td>";
			tmp += "</tr>";
		}
	}
	result.strReplace("#MOTORPOSITIONS#", tmp);

	return result;
}

void ezapRotorInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
//	dyn_resolver->addDyn("GET", "/cgi-bin/setConfigSwapFile", setConfigSwapFile, lockWeb);
//	dyn_resolver->addDyn("GET", "/cgi-bin/setConfigMultiBoot", setConfigMultiBoot, lockWeb);
}
#endif

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
#include <lib/dvb/frontend.h>

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
	eString tmp  = readFile(TEMPLATE_DIR + "rotorSat.tmp");
	eTransponder *tp = NULL;
	eString satPos, satName, motorPos, current;
	
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (sapi && sapi->transponder)
		tp = sapi->transponder;
	
	for (std::list<eLNB>::iterator it(eTransponderList::getInstance()->getLNBs().begin()); it != eTransponderList::getInstance()->getLNBs().end(); it++)
	{
		// go thru all satellites...
		for (ePtrList<eSatellite>::iterator s (it->getSatelliteList().begin()); s != it->getSatelliteList().end(); s++)
		{
			if (tp && s->getOrbitalPosition() == tp->satellite.orbital_position)
				current = "on.gif";
			else
				current = "trans.gif";
			satName = s->getDescription();
			satPos = eString().sprintf("%d", s->getOrbitalPosition());
			int motorPosition = it->getDiSEqC().RotorTable[s->getOrbitalPosition()];
			motorPos = eString().sprintf("%d", motorPosition);

			tmp.strReplace("#CURRENT#", current);
			tmp.strReplace("#SATPOS#", satPos);
			tmp.strReplace("#SATNAME#", satName);
			tmp.strReplace("#MOTORPOS#", motorPos);
		}
	}
	result.strReplace("#MOTORPOSITIONS#", tmp);

	return result;
}

static eString sendDiSEqCCmd(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	eString addr = opt["addr"];
	eString cmd = opt["cmd"];
	eString params = opt["params"];
	eString frame = opt["frame"];
	if (!frame)
		frame = "E0";
	
	int iaddr, icmd, iframe;
	sscanf(addr.c_str(), "%x", &iaddr);
	sscanf(cmd.c_str(), "%x", &icmd);
	sscanf(frame.c_str(), "%x", &iframe);

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	
	eDebug("[enigma_dyn_rotor] sending DiSeqCCmd: %x %x %s %x", iaddr, icmd, params.c_str(), iframe);
	eFrontend::getInstance()->sendDiSEqCCmd(iaddr, icmd, params, iframe);
	
	return closeWindow(content, "", 500);
}

void ezapRotorInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/cgi-bin/sendDiSEqCCmd", sendDiSEqCCmd, lockWeb);
}
#endif

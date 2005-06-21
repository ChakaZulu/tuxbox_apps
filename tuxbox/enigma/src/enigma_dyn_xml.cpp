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
#include <enigma_dyn_xml.h>
#include <streaminfo.h>

using namespace std;

extern eString zap[5][5];
extern eString firmwareLevel(eString verid);
extern bool onSameTP(const eServiceReferenceDVB& ref1, const eServiceReferenceDVB &ref2); // implemented in timer.cpp
extern eString getIP(void);

static eString getImageInfo(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/xml; charset=utf-8";
	content->local_header["Cache-Control"] = "no-cache";

	std::stringstream result;

	eString myVersion = getAttribute("/.version", "version");
	eString myCatalogURL = getAttribute("/.version", "catalog");
	eString myComment = getAttribute("/.version", "comment");
	eString myImageURL = getAttribute("/.version", "url");

	result  << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		<< "<image>"
		<< "<version>" << firmwareLevel(myVersion) << "</version>"
		<< "<url>" << myImageURL << "</url>"
		<< "<comment>" << myComment << "</comment>"
		<< "<catalog>" << myCatalogURL << "</catalog>"
		<< "</image>";

	return result.str();
}

static eString getStatus(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString name, provider, vpid, apid, pcrpid, tpid, vidform("n/a"), tsid, onid, sid, pmt;

	content->local_header["Content-Type"]="text/xml; charset=utf-8";
	content->local_header["Cache-Control"] = "no-cache";

	eString result;
	time_t atime;
	time(&atime);
	atime += eDVB::getInstance()->time_difference;
	result = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<status>"
		"<current_time>" + eString(ctime(&atime)) + "</current_time>"
		"<standby>";
	if (eZapMain::getInstance()->isSleeping())
		result += "ON";
	else
		result += "OFF";
	result += "</standby>";
	result += "<recording>";
#ifndef DISABLE_FILE
	if (eZapMain::getInstance()->isRecording())
		result += "ON";
	else
#endif
		result += "OFF";
	result += "</recording>";
	result += "<mode>" + eString().sprintf("%d", eZapMain::getInstance()->getMode()) + "</mode>";

	result += "</status>";
	
	return result;
}

static eString getAudioChannels(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/xml; charset=utf-8";
	content->local_header["Cache-Control"] = "no-cache";

	std::stringstream result;

	result  << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		<< "<audio_channels>";
	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		std::list<eDVBServiceController::audioStream> &astreams(sapi->audioStreams);
		for (std::list<eDVBServiceController::audioStream>::iterator it(astreams.begin())
			;it != astreams.end(); ++it)
		{
			result  << "<channel>" 
				<< "<pid>"
				<< eString().sprintf("0x%04x", it->pmtentry->elementary_PID)
				<< "</pid>"
				<< "<selected>";
			if (it->pmtentry->elementary_PID == Decoder::current.apid)
				result << "1";
			else
				result << "0";
			result  << "</selected>"
				<< "<name>" << it->text << "</name>"
				<< "</channel";
		}
	}
	else
		result << "<audio>none</audio>";

	result << "</audio_channels>";
	
	return result.str();
}

static eString getEPG(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::stringstream result;
	eString description, ext_description, genre;
	int genreCategory = 0;
	result << std::setfill('0');

	eService* current;
	eServiceReference ref;

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	
	result  << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		<< "<?xml-stylesheet type=\"text/xsl\" href=\"/xml/channelepg.xsl\"?>"
		<< "<epg>";

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		eString type = opt["type"];
		eString serviceRef = opt["ref"];
		ref = (serviceRef) ? string2ref(serviceRef) : sapi->service;
		current = eDVB::getInstance()->settings->getTransponders()->searchService(ref);
		if (current)
		{
			result	<< "<service_reference>" << ref2string(ref) << "</service_reference>"
				<< "<service_name>" << filter_string(current->service_name) << "</service_name>";
			eServiceReferenceDVB &rref = (eServiceReferenceDVB&)ref;
			eEPGCache::getInstance()->Lock();
			const timeMap* evt = eEPGCache::getInstance()->getTimeMap(rref);

			if (evt)
			{
				timeMap::const_iterator It;
				int tsidonid = (rref.getTransportStreamID().get()<<16) | rref.getOriginalNetworkID().get();
			
				int i = 0;
				for (It = evt->begin(); It != evt->end(); ++It)
				{
					ext_description = "";
					EITEvent event(*It->second,tsidonid);
					for (ePtrList<Descriptor>::iterator d(event.descriptor); d != event.descriptor.end(); ++d)
					{
						Descriptor *descriptor = *d;
						if (descriptor->Tag() == DESCR_EXTENDED_EVENT)
							ext_description += ((ExtendedEventDescriptor*)descriptor)->text;
						else
						if (descriptor->Tag() == DESCR_SHORT_EVENT)
							description = ((ShortEventDescriptor*)descriptor)->event_name;
						else
						if (descriptor->Tag() == DESCR_CONTENT)
						{
							genre = "";
							genreCategory = 0;
							ContentDescriptor *cod = (ContentDescriptor *)descriptor;

							for (ePtrList<descr_content_entry_struct>::iterator ce(cod->contentList.begin()); ce != cod->contentList.end(); ++ce)
							{
								if (genreCategory == 0)
									genreCategory = ce->content_nibble_level_1;
								if (eChannelInfo::getGenre(genreCategory * 16 + ce->content_nibble_level_2))
								{
									if (!genre)
										genre += gettext(eChannelInfo::getGenre(genreCategory * 16 + ce->content_nibble_level_2).c_str());
								}
							}
						}
					}
			
					if (!genre)
						genre = "n/a";

					tm* t = localtime(&event.start_time);

					result << "<event id=\"" << i << "\">";
					eString tmp = filter_string(description);
					tmp.strReplace("&", "&amp;");
					result  << "<date>"
						<< std::setw(2) << t->tm_mday << '.'
						<< std::setw(2) << t->tm_mon+1 << '.' 
						<< std::setw(2) << t->tm_year + 1900
						<< "</date>"
						<< "<time>"
						<< std::setw(2) << t->tm_hour << ':'
						<< std::setw(2) << t->tm_min 
						<< "</time>"
						<< "<duration>" << event.duration<< "</duration>"
						<< "<description>" << filter_string(tmp) << "</description>";
					
					if (type == "extended")
					{
						eString ext_tmp = filter_string(ext_description);
						ext_tmp.strReplace("&", "&amp;");	

						result  << "<genre>" << genre << "</genre>"
							<< "<genrecategory>" << "genre" << eString().sprintf("%02d", genreCategory) << "</genrecategory>"
							<< "<start>" << event.start_time << "</start>"
							<< "<details>" << filter_string(ext_tmp) << "</details>";
					}
					
					result << "</event>";
					i++;
				}
			}
			eEPGCache::getInstance()->Unlock();
		}
	}
	result << "</epg>";

	return result.str();
}

static eString mPlayer(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString vpid = eString().sprintf("%04x", Decoder::current.vpid);
	eString apid = eString().sprintf("%04x", Decoder::current.apid);

	content->local_header["Content-Type"]="video/mpegfile";
	content->local_header["Cache-Control"] = "no-cache";
	content->local_header["vpid"] = vpid;
	content->local_header["apid"] = apid;

	return "http://" + getIP() + ":31339/" + vpid  + "," + apid;
}

eString getTag(int mode, int submode)
{
	eString tag;
	switch(mode)
	{
		case 3: 
			tag = "movie"; 
			break;
		case 4: 
			tag = "file"; 
			break;
		default: 
			switch(submode)
			{
				case 2: tag = "satellite"; break;
				case 3: tag = "provider"; break;
				case 4: tag = "bouquet"; break;
				default: tag = "unknown"; break;
			}
	}
	return tag;
}

struct getContent: public Object
{
	int mode;
	int subm;
	eString &result;
	eServiceInterface *iface;
	bool listCont;
	getContent(int mode, int subm, const eServiceReference &service, eString &result, bool listCont)
		:mode(mode), subm(subm), result(result), iface(eServiceInterface::getInstance()), listCont(listCont)
	{
		Signal1<void, const eServiceReference&> cbSignal;
		CONNECT(cbSignal, getContent::addToString);
		iface->enterDirectory(service, cbSignal);
		iface->leaveDirectory(service);
	}
	void addToString(const eServiceReference& ref)
	{
		// sorry.. at moment we dont show any directory.. or locked service in webif
		if (ref.isLocked() && eConfig::getInstance()->pLockActive())
			return;

		eService *service = iface ? iface->addRef(ref) : 0;
		if (!(ref.data[0] == -1 && ref.data[2] != (int)0xFFFFFFFF))
		{
		
			if (ref.flags & eServiceReference::isDirectory)
				result += "\n<" + getTag(mode, subm) + ">";
			else
				result += "\n<service>";
			
			result += "<reference>" + ref.toString() + "</reference>";

			if (ref.descr)
				result += "<name>" + filter_string(ref.descr) + "</name>";
			else
			if (service)
			{
				result += "<name>" + filter_string(service->service_name) + "</name>";
				if (ref.type == eServiceReference::idDVB && !(ref.flags & eServiceReference::isDirectory))
					result += "<provider>" + filter_string(((eServiceDVB*)service)->service_provider) + "</provider>";
			}

			if (ref.type == eServiceReference::idDVB && !(ref.flags & eServiceReference::isDirectory))
			{
				const eServiceReferenceDVB& dvb_ref = (const eServiceReferenceDVB&)ref;
				eTransponder *tp = eTransponderList::getInstance()->searchTS(
					dvb_ref.getDVBNamespace(),
					dvb_ref.getTransportStreamID(),
					dvb_ref.getOriginalNetworkID());
				if (tp && tp->satellite.isValid())
					result += "<orbital_position>" + eString().setNum(tp->satellite.orbital_position) + "</orbital_position>";
			}
		
			if (service)
				iface->removeRef(ref);
			
			if (listCont && ref.flags & eServiceReference::isDirectory)
			{
				getContent(mode, subm, ref, result, false);
				result += "\n</" + getTag(mode, subm) + ">";
			}
			else
				result += "</service>";
		}
	}
};

static eString getServices(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	/* MODE: 0 = TV, 1 = Radio, 2 = Data, 3 = Movies, 4 = Root */
	/* SUBMODE: 0 = n/a, 1 = All, 2 = Satellites, 2 = Providers, 4 = Bouquets */
	
	content->local_header["Content-Type"] = "text/plain; charset=utf-8";
	std::map<eString,eString> opts = getRequestOptions(opt, '&');
	
	eString mode = "0";
	if (opts["mode"])
		mode = opts["mode"];
	int mod = atoi(mode.c_str());

	eString submode = "2";
	if (opts["submode"])
		submode = opts["submode"];
	int subm = atoi(submode.c_str());
		
	eString sref = zap[mod][subm];
	eServiceReference ref(sref);

	eString result = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	result += "<" + getTag(mod, subm) + "s>";
	getContent t(mod, subm, ref, result, true);
	result += "\n</" + getTag(mod, subm) + "s>";
	result.strReplace("&", "&amp;");
//	result.strReplace("<", "&lt;");
//	result.strReplace(">", "&gt;");
//	result.strReplace("\'", "&apos;");
//	result.strReplace("\"", "&quot;");
	return result;
}

static eString getStreamInfoXSL(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString result;
	
	content->local_header["Content-Type"] = "text/xml; charset=utf-8";
	switch(eSystemInfo::getInstance()->getFEType())
	{
		case eSystemInfo::feSatellite:
			result = readFile(TEMPLATE_DIR + "streaminfo_satellite.xsl");
			break;
		case eSystemInfo::feCable:
			result = readFile(TEMPLATE_DIR + "streaminfo_cable.xsl");
			break;
		case eSystemInfo::feTerrestrial:
			result = readFile(TEMPLATE_DIR + "streaminfo_terrestrial.xsl");
			break;
	}
	
	return result;
}

static eString getChannelEPGXSL(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"] = "text/xml; charset=utf-8";
	return readFile(TEMPLATE_DIR + "channelepg.xsl");
}

static eString getStreamInfo(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString result = readFile(TEMPLATE_DIR + "streaminfo.tmp");

	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "not available";

	eServiceDVB *service=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
	if (service)
	{
		result.strReplace("#SERVICENAME#", filter_string(service->service_name));
		result.strReplace("#PROVIDER#", filter_string(service->service_provider));
	}
	else
	{
		result.strReplace("#SERVICENAME#", "n/a");
		result.strReplace("#PROVIDER#", "n/a");	
	}
	result.strReplace("#VPID#", eString().sprintf("%04xh (%dd)", Decoder::current.vpid, Decoder::current.vpid));
	result.strReplace("#APID#", eString().sprintf("%04xh (%dd)", Decoder::current.apid, Decoder::current.apid));
	result.strReplace("#PCRPID#", eString().sprintf("%04xh (%dd)", Decoder::current.pcrpid, Decoder::current.pcrpid));
	result.strReplace("#TPID#", eString().sprintf("%04xh (%dd)", Decoder::current.tpid, Decoder::current.tpid));
	result.strReplace("#TSID#", eString().sprintf("%04xh", sapi->service.getTransportStreamID().get()));
	result.strReplace("#ONID#", eString().sprintf("%04xh", sapi->service.getOriginalNetworkID().get()));
	result.strReplace("#SID#", eString().sprintf("%04xh", sapi->service.getServiceID().get()));
	result.strReplace("#PMT#", eString().sprintf("%04xh", Decoder::current.pmtpid));
	result.strReplace("#NAMESPACE#", eString().sprintf("%04xh", sapi->service.getDVBNamespace().get()));

	FILE *bitstream = 0;

	eString vidform;
	if (Decoder::current.vpid != -1)
		bitstream = fopen("/proc/bus/bitstream", "rt");
	if (bitstream)
	{
		char buffer[100];
		int xres = 0, yres = 0, aspect = 0, framerate = 0;
		while (fgets(buffer, 100, bitstream))
		{
			if (!strncmp(buffer, "H_SIZE:  ", 9))
				xres=atoi(buffer+9);
			if (!strncmp(buffer, "V_SIZE:  ", 9))
				yres=atoi(buffer+9);
			if (!strncmp(buffer, "A_RATIO: ", 9))
				aspect=atoi(buffer+9);
			if (!strncmp(buffer, "F_RATE: ", 8))
				framerate=atoi(buffer+8);
		}
		fclose(bitstream);
		
		vidform.sprintf("%dx%d ", xres, yres);
		switch (aspect)
		{
			case 1:
				vidform += "(square)"; break;
			case 2:
				vidform += "(4:3)"; break;
			case 3:
				vidform += "(16:9)"; break;
			case 4:
				vidform += "(20:9)"; break;
		}
		switch (framerate)
		{
			case 1:
				vidform += ", 23.976 fps"; break;
			case 2:
				vidform += ", 24 fps"; break;
			case 3:
				vidform += ", 25 fps"; break;
			case 4:
				vidform += ", 29.97 fps"; break;
			case 5:
				vidform += ", 30 fps"; break;
			case 6:
				vidform += ", 50 fps"; break;
			case 7:
				vidform += ", 59.94 fps"; break;
			case 8:
				vidform += ", 80 fps"; break;
		}
	}

	eString sRef;
	if (eServiceInterface::getInstance()->service)
		sRef = eServiceInterface::getInstance()->service.toString();
	result.strReplace("#SERVICEREFERENCE#", sRef);
	result.strReplace("#VIDEOFORMAT#", vidform);
	
	extern struct caids_t caids[];
	extern unsigned int caids_cnt;

	clearCA();
	eString cryptSystems;
	// singleLock s(eDVBServiceController::availCALock);
	std::set<int>& availCA = sapi->availableCASystems;
	for (std::set<int>::iterator i(availCA.begin()); i != availCA.end(); ++i)
	{
		eString caname = eStreaminfo::getInstance()->getCAName(*i, 0);
		if (caname)
		{
			if (cryptSystems)
				cryptSystems += ", ";
			cryptSystems += caname;
		}
	}
  	result.strReplace("#SUPPORTEDCRYPTSYSTEMS#", cryptSystems);
	
	int foundone = 0;
	cryptSystems = "";
	std::set<int>& calist = sapi->usedCASystems;
	for (std::set<int>::iterator i(calist.begin()); i != calist.end(); ++i)
	{
		eString caname = eStreaminfo::getInstance()->getCAName(*i, 1);
		eString codesys = eString().sprintf("%04xh:  ", *i) + caname;
		if (cryptSystems)
			cryptSystems += ", ";
		cryptSystems += codesys;
		foundone++;
	}
	if (!foundone)
		cryptSystems = "None";
	result.strReplace("#USEDCRYPTSYSTEMS#", cryptSystems);

	int tpData = 0;
	eTransponder *tp = sapi->transponder;
	if (tp)
	{
		switch(eSystemInfo::getInstance()->getFEType())
		{
			case eSystemInfo::feSatellite:
			{
				for (std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin());
					tpData == 0 && it != eTransponderList::getInstance()->getLNBs().end(); it++)
					for (ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin());
						s != it->getSatelliteList().end(); s++)
						if (s->getOrbitalPosition() == tp->satellite.orbital_position) 
						{
							result.strReplace("#SATELLITE#", s->getDescription().c_str());
							tpData = 1;
							break;
						}
				if (tpData == 1)
				{
					result.strReplace("#FREQUENCY#", eString().sprintf("%d", tp->satellite.frequency / 1000));
					result.strReplace("#SYMBOLRATE#", eString().sprintf("%d", tp->satellite.symbol_rate / 1000));
					result.strReplace("#POLARISATION#", tp->satellite.polarisation ? "Vertical" : "Horizontal");
					result.strReplace("#INVERSION#", tp->satellite.inversion ? "Yes" : "No");

					switch (tp->satellite.fec)
					{
						case 0: result.strReplace("#FEC#", "Auto"); break;
						case 1: result.strReplace("#FEC#", "1/2"); break;
						case 2: result.strReplace("#FEC#", "2/3"); break;
						case 3: result.strReplace("#FEC#", "3/4"); break;
						case 4: result.strReplace("#FEC#", "5/6"); break;
						case 5: result.strReplace("#FEC#", "7/8"); break;
						case 6: result.strReplace("#FEC#", "8/9"); break;
					}

					eFrontend *fe = eFrontend::getInstance();
					int status = fe->Status();
					bool lock = status & FE_HAS_LOCK;
					bool sync = status & FE_HAS_SYNC;
					result.strReplace("#SNR#", eString().sprintf("%d%%", fe->SNR() * 100/65535));
					result.strReplace("#AGC#", eString().sprintf("%d%%", fe->SignalStrength() * 100/65535));
					result.strReplace("#BER#", eString().sprintf("%u", fe->BER()));
					result.strReplace("#LOCK#", (lock ? "Yes" : "No"));
					result.strReplace("#SYNC#", (sync ? "Yes" : "No"));
				}
			}
			case eSystemInfo::feCable:
			{
				result.strReplace("#FREQUENCY#", eString().sprintf("%d", tp->cable.frequency / 1000));
				result.strReplace("#SYMBOLRATE#", eString().sprintf("%d", tp->cable.symbol_rate / 1000));
				result.strReplace("#INVERSION#", tp->cable.inversion ? "Yes" : "No");

				switch (tp->cable.modulation)
				{
					case 0: result.strReplace("#MODULATION#", "Auto"); break;
					case 1: result.strReplace("#MODULATION#", "16-QAM"); break;
					case 2: result.strReplace("#MODULATION#", "32-QAM"); break;
					case 3: result.strReplace("#MODULATION#", "64-QAM"); break;
					case 4: result.strReplace("#MODULATION#", "128-QAM"); break;
					case 5: result.strReplace("#MODULATION#", "256-QAM"); break;
				}

				switch (tp->cable.fec_inner)
				{
					case 0: result.strReplace("FEC#", "Auto"); break;
					case 1: result.strReplace("FEC#", "1/2"); break;
					case 2: result.strReplace("FEC#", "2/3"); break;
					case 3: result.strReplace("FEC#", "3/4"); break;
					case 4: result.strReplace("FEC#", "5/6"); break;
					case 5: result.strReplace("FEC#", "7/8"); break;
					case 6: result.strReplace("FEC#", "8/9"); break;
				}
			}
			case eSystemInfo::feTerrestrial:
			{
				result.strReplace("#CENTERFREQUENCY#", eString().sprintf("%d",  tp->terrestrial.centre_frequency / 1000));
				result.strReplace("#INVERSION#", eString().sprintf("%d",  tp->terrestrial.inversion));
				result.strReplace("#HIERARCHYINFO#", eString().sprintf("%d",   tp->terrestrial.hierarchy_information));

				switch (tp->terrestrial.bandwidth)
				{
					case 0: result.strReplace("#BANDWIDTH#", "8"); break;
					case 1: result.strReplace("#BANDWIDTH#", "7"); break;
					case 2: result.strReplace("#BANDWIDTH#", "6"); break;
				}

				switch (tp->terrestrial.constellation)
				{
					case 0: result.strReplace("#CONSTELLATION#", "Auto"); break;
					case 1: result.strReplace("#CONSTELLATION#", "QPSK"); break;
					case 2: result.strReplace("#CONSTELLATION#", "16-QAM"); break;
					case 3: result.strReplace("#CONSTELLATION#", "64-QAM"); break;
				}

				switch (tp->terrestrial.guard_interval)
				{
					case 0: result.strReplace("#GUARDINTERVAL#", "Auto"); break;
					case 1: result.strReplace("#GUARDINTERVAL#", "1/32"); break;
					case 2: result.strReplace("#GUARDINTERVAL#", "1/16"); break;
					case 3: result.strReplace("#GUARDINTERVAL#", "1/8"); break;
					case 4: result.strReplace("#GUARDINTERVAL#", "1/4"); break;
				}

				switch (tp->terrestrial.transmission_mode)
				{
					case 0: result.strReplace("#TRANSMISSION#", "Auto"); break;
					case 1: result.strReplace("#TRANSMISSION#", "2k"); break;
					case 2: result.strReplace("#TRANSMISSION#", "8k"); break;
				}

				switch (tp->terrestrial.code_rate_lp)
				{
					case 0: result.strReplace("#CODERATELP#", "Auto"); break;
					case 1: result.strReplace("#CODERATELP#", "1/2"); break;
					case 2: result.strReplace("#CODERATELP#", "2/3"); break;
					case 3: result.strReplace("#CODERATELP#", "3/4"); break;
					case 4: result.strReplace("#CODERATELP#", "5/6"); break;
					case 5: result.strReplace("#CODERATELP#", "7/8"); break;
				}
				
				switch (tp->terrestrial.code_rate_hp)
				{
					case 0: result.strReplace("#CODERATEHP#", "Auto"); break;
					case 1: result.strReplace("#CODERATEHP#", "1/2"); break;
					case 2: result.strReplace("#CODERATEHP#", "2/3"); break;
					case 3: result.strReplace("#CODERATEHP#", "3/4"); break;
					case 4: result.strReplace("#CODERATEHP#", "5/6"); break;
					case 5: result.strReplace("#CODERATEHP#", "7/8"); break;
				}
			}
		}
	}
	
	if (tpData == 0)
	{
		result.strReplace("#SATELLITE#", "n/a");
		result.strReplace("#FREQUENCY#", "n/a");
		result.strReplace("#SYMBOLRATE#", "n/a");
		result.strReplace("#POLARISATION#", "n/a");
		result.strReplace("#INVERSION#", "n/a");
		result.strReplace("#FEC#", "n/a");
		result.strReplace("#SNR#", "n/a");
		result.strReplace("#AGC#", "n/a");
		result.strReplace("#BER#", "n/a");
		result.strReplace("#LOCK#", "n/a");
		result.strReplace("#SYNC#", "n/a");
	}
		
	return result;
}

void ezapXMLInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/xml/status", getStatus, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/epg", getEPG, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/imageinfo", getImageInfo, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/audiochannels", getAudioChannels, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/mplayer.mply", mPlayer, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/getservices", getServices, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/streaminfo", getStreamInfo, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/streaminfo.xsl", getStreamInfoXSL, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/channelepg.xsl", getChannelEPGXSL, lockWeb);
}


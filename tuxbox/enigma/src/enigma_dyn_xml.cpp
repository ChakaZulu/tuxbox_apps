#ifdef ENABLE_DYN_XML
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
#include <enigma_dyn_xml.h>

using namespace std;

extern eString zap[5][5];
extern eString firmwareLevel(eString verid);
extern bool onSameTP(const eServiceReferenceDVB& ref1, const eServiceReferenceDVB &ref2); // implemented in timer.cpp
extern eString removeBadChars(eString s);

static eString getimageinfoXML(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/xml; charset=utf-8";
	content->local_header["Cache-Control"] = "no-cache";

	std::stringstream result;

	eString myVersion = getAttribute("/.version", "version");
	eString myCatalogURL = getAttribute("/.version", "catalog");
	eString myComment = getAttribute("/.version", "comment");
	eString myImageURL = getAttribute("/.version", "url");

	result << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><content>";
	result << "<title>Installed Image Information</title>";
	result << "<version>" << firmwareLevel(myVersion) << "</version>";
	result << "<url>" << myImageURL << "</url>";
	result << "<comment>" << myComment << "</comment>";
	result << "<catalog>" << myCatalogURL << "</catalog>";
	result << "</content>";

	return result.str();
}

class eXMLNavigatorListDirectory: public Object
{
	eString &result;
	eString origpath;
	eString path;
	eServiceInterface &iface;
public:
	eXMLNavigatorListDirectory(eString &result, eString origpath, eString path, eServiceInterface &iface): result(result), origpath(origpath), path(path), iface(iface)
	{
		eDebug("path: %s", path.c_str());
	}
	void addEntry(const eServiceReference &e)
	{
#ifndef DISABLE_FILE
		if (eDVB::getInstance()->recorder && !e.path && !e.flags)
		{
			if (!onSameTP(eDVB::getInstance()->recorder->recRef,(eServiceReferenceDVB&)e))
					 return;
		}
#endif
		eString serviceRef = ref2string(e);

		if (!(e.flags & eServiceReference::isDirectory))
			result += "<name path=\"" + serviceRef + "\">";
		else
			result += "<name path=\"" + serviceRef + "\">";

		eService *service = iface.addRef(e);
		if (!service)
			result += "n/a";
		else
		{
			result +=  filter_string(service->service_name);
			iface.removeRef(e);
		}
		result += "</name>";
	}
};

static eString getXMLZapContent(eString path)
{
	eString tpath, result;

	unsigned int pos = 0, lastpos = 0, temp = 0;

	if ((path.find(";", 0)) == eString::npos)
		path = ";" + path;

	while ((pos = path.find(";", lastpos)) != eString::npos)
	{
		lastpos = pos + 1;
		if ((temp = path.find(";", lastpos)) != eString::npos)
			tpath = path.mid(lastpos, temp - lastpos);
		else
			tpath = path.mid(lastpos, strlen(path.c_str()) - lastpos);

		eServiceReference current_service = string2ref(tpath);
		eServiceInterface *iface = eServiceInterface::getInstance();

		// first pass thru is to get all user bouquets
		eXMLNavigatorListDirectory navlist(result, path, tpath, *iface);
		Signal1<void, const eServiceReference&> signal;
		signal.connect(slot(navlist, &eXMLNavigatorListDirectory::addEntry));
		iface->enterDirectory(current_service, signal);
		eDebug("entered");
		iface->leaveDirectory(current_service);
		eDebug("exited");
	}

	return result;
}

static eString zaplist(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;

	std::map<eString,eString> opt = getRequestOptions(opts, ',');
	eString mode = opt["mode"];
	eString spath = opt["path"];

	content->local_header["Content-Type"]="text/xml; charset=utf-8";
	content->local_header["Cache-Control"] = "no-cache";

	result = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><content>";

	if (mode == "tv"){
		result += "<zapmode id=\"TV-BOUQUETS\">";
		if (opts.find("path") == eString::npos)
			spath = zap[ZAPMODETV][ZAPSUBMODEBOUQUETS];
	}else	if (mode == "tvprov")	{
		result += "<zapmode id=\"TV-PROVIDER\">";
		if (opts.find("path") == eString::npos)
			spath = zap[ZAPMODETV][ZAPSUBMODEPROVIDERS];
	}else	if (mode == "tvsat")	{
		result += "<zapmode id=\"TV-SATELLITES\">";
		if (opts.find("path") == eString::npos)
			spath = zap[ZAPMODETV][ZAPSUBMODESATELLITES];
	}else if (mode == "radio"){
		result += "<zapmode id=\"RADIO-BOUQUETS\">";
		if (opts.find("path") == eString::npos)
			spath = zap[ZAPMODERADIO][ZAPSUBMODEBOUQUETS];
	}else if (mode == "radioprov"){
		result += "<zapmode id=\"RADIO-PROVIDERS\">";
		if (opts.find("path") == eString::npos)
			spath = zap[ZAPMODERADIO][ZAPSUBMODEPROVIDERS];
	}else if (mode == "radiosat"){
		result += "<zapmode id=\"RADIO-SATELLITES\">";
		if (opts.find("path") == eString::npos)
			spath = zap[ZAPMODERADIO][ZAPSUBMODESATELLITES];
	}else if (mode == "datasat"){
		result += "<zapmode id=\"DATA-SATELLITES\">";
		if (opts.find("path") == eString::npos)
			spath = zap[ZAPMODEDATA][ZAPSUBMODESATELLITES];
	}else if (mode == "dataprov"){
		result += "<zapmode id=\"DATA-PROVIDERS\">";
		if (opts.find("path") == eString::npos)
			spath = zap[ZAPMODEDATA][ZAPSUBMODEPROVIDERS];
	}else if (mode == "movie"){
		result += "<zapmode id=\"RECORDINGS\">";
		if (opts.find("path") == eString::npos)
			spath = zap[ZAPMODERECORDINGS][ZAPSUBMODECATEGORY];
	}else{
		result += "<zapmode id=\"TV-BOUQUETS\">";
		if (opts.find("path") == eString::npos)
			spath = zap[ZAPMODETV][ZAPSUBMODEBOUQUETS];
	}
	result += getXMLZapContent(spath);
	result += "</zapmode>";
	result += "</content>";

	return result;
}

static eString doStatusXML(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString name, provider, vpid, apid, pcrpid, tpid, vidform("n/a"), tsid, onid, sid, pmt;

	content->local_header["Content-Type"]="text/xml; charset=utf-8";
	content->local_header["Cache-Control"] = "no-cache";

	eString result;
	time_t atime;
	time(&atime);
	atime += eDVB::getInstance()->time_difference;
	result = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>"
		"<content>"
		"<title>Enigma Status</title>"
		"<currenttime>" + eString(ctime(&atime)) + "</currenttime>"
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

	eString sRef;
	if (eServiceInterface::getInstance()->service)
		sRef = eServiceInterface::getInstance()->service.toString();
	result += "<servicereference>" + sRef + "</servicereference>";

	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		eServiceDVB *service=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
		if (service)
		{
			name = filter_string(service->service_name);
			provider = filter_string(service->service_provider);
		}
	}
	vpid = eString().sprintf("%04x", Decoder::current.vpid);
	apid = eString().sprintf("%04x", Decoder::current.apid);
	pcrpid = eString().sprintf("%04x", Decoder::current.pcrpid);
	tpid = eString().sprintf("%04x", Decoder::current.tpid);
	tsid = eString().sprintf("%04x", sapi->service.getTransportStreamID().get());
	onid = eString().sprintf("%04x", sapi->service.getOriginalNetworkID().get());
	sid = eString().sprintf("%04x", sapi->service.getServiceID().get());
	pmt = eString().sprintf("%04x", Decoder::current.pmtpid);

	FILE *bitstream = 0;

	if (Decoder::current.vpid != -1)
		bitstream = fopen("/proc/bus/bitstream", "rt");
	if (bitstream)
	{
		char buffer[100];
		int xres = 0, yres = 0, aspect = 0;
		while (fgets(buffer, 100, bitstream))
		{
			if (!strncmp(buffer, "H_SIZE:  ", 9))
				xres=atoi(buffer+9);
			if (!strncmp(buffer, "V_SIZE:  ", 9))
				yres=atoi(buffer+9);
			if (!strncmp(buffer, "A_RATIO: ", 9))
				aspect=atoi(buffer+9);
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
	}
	result += "<name>" + name + "</name>";
	result += "<provider>" + provider + "</provider>";
	result += "<vpid>" + vpid + "</vpid>";
	result += "<apid>" + apid + "</apid>";
	result += "<pcrpid>" + pcrpid + "</pcrpid>";
	result += "<tpid>" + tpid + "</tpid>";
	result += "<tsid>" + tsid + "</tsid>";
	result += "<onid>" + onid + "</onid>";
	result += "<sid>" + sid + "</sid>";
	result += "<pmt>" + pmt + "</pmt>";
	result += "<videoformat>" + vidform + "</videoformat>";
	result += "</content>";
	
	return result;
}

static eString getAudioChannelsXML(eString request, eString dirpath, eString opts, eHTTPConnection *content) //(eString eventID)
{
	content->local_header["Content-Type"]="text/xml; charset=utf-8";
	content->local_header["Cache-Control"] = "no-cache";

	std::stringstream result;

	result << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><content>";
	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		std::list<eDVBServiceController::audioStream> &astreams(sapi->audioStreams);
		for (std::list<eDVBServiceController::audioStream>::iterator it(astreams.begin())
			;it != astreams.end(); ++it)
		{
			if (it->pmtentry->elementary_PID == Decoder::current.apid)
				result << eString().sprintf("<audio pid=\"0x%04x\">", it->pmtentry->elementary_PID);
			else
				result << eString().sprintf("<audio pid=\"0x%04x\">", it->pmtentry->elementary_PID);

			result << removeBadChars(it->text);
			result << "</audio>";
		}
	}
	else
		result << "<audio>none</audio>";

	result << "</content>";
	
	return result.str();
}

static eString getcurepgXML(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::stringstream result;
	eString description, ext_description, genre;
	int genreCategory = 0;
	result << std::setfill('0');

	eService* current;
	eServiceReference ref;

	content->local_header["Content-Type"]="text/xml; charset=utf-8";
	content->local_header["Cache-Control"] = "no-cache";

	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString type = opt["type"];

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><content id=\"sapi\">No EPG available</content>";

	//eString serviceRef = opt["ref"];
	ref = sapi->service;

	current = eDVB::getInstance()->settings->getTransponders()->searchService(ref);

	if (!current)
		return "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><content id=\"current\">No EPG available</content>";

	eEPGCache::getInstance()->Lock();
	const timeMap* evt = eEPGCache::getInstance()->getTimeMap((eServiceReferenceDVB&)ref);

	if (!evt)
		return "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><content id=\"evt\">No EPG available</content>";
	else
	{
		timeMap::const_iterator It;

		result << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><content>";
		
		int i = 0;
		for(It=evt->begin(); It!= evt->end(); ++It)
		{
			ext_description = "";
			EITEvent event(*It->second);
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

			tm* t = localtime(&event.start_time);

			if (type == "extended")
			{
				if (!genre)
					genre = "n/a";

				
				result << "<epg id=\"" << i << "\">" 
					<< "<date>" 
					<< std::setw(2) << t->tm_mday << '.'
					<< std::setw(2) << t->tm_mon+1 << "</date><time>"
					<< std::setw(2) << t->tm_hour << ':'
					<< std::setw(2) << t->tm_min << ' '
					<< "</time>\n";

				eString tmp = filter_string(description);
				tmp.strReplace("\'", "\\\'");
				tmp.strReplace("\"", "\\\"");
				tmp.strReplace("&", "&amp;");

#ifndef DISABLE_FILE
				result << "<ref>" << ref2string(ref) << "</ref>"
					<< "<start>" << event.start_time << "</start>"
					<< "<duration>" << event.duration << "</duration>";
				
				result  << "<descr>" << tmp << "</descr>"
					<< "<channel>" << filter_string(current->service_name)  << "</channel>";
#endif
				eString ext_tmp = filter_string(ext_description);
				ext_tmp.strReplace("&", "&amp;");

				result << "<event>" << filter_string(tmp) << "</event>"
					<< "<genre>" << genre	<< "</genre>"
					<< "<description>" << filter_string(ext_tmp) << "</description></epg>\n";
			}
			else
			{
				result << "<epg id=\"" << i << "\">" 
					<< eString().sprintf("<eventid>%x", event.event_id) << "</eventid><date>"
					<< std::setw(2) << t->tm_mday << '.'
					<< std::setw(2) << t->tm_mon+1 << "</date><time>"
					<< std::setw(2) << t->tm_hour << ':'
					<< std::setw(2) << t->tm_min << "</time>"
					<< "<description>" << description << "</description></epg>\n";					
			}
			i++;
		}
	}
	eEPGCache::getInstance()->Unlock();
	
	result << "</content>";

	return result.str();
}

static eString getepgdetailsXML(eString request, eString dirpath, eString opts, eHTTPConnection *content) //(eString eventID)
{
	std::stringstream result;
	result << std::setfill('0');

	eServiceReference ref;

	eService *current = NULL;
	eString ext_description;
	std::stringstream record;
	int eventid;
	eString description = "No description available";


	content->local_header["Content-Type"]="text/xml; charset=utf-8";
	content->local_header["Cache-Control"] = "no-cache";

	std::map<eString, eString> opt = getRequestOptions(opts, '=');
	eString eventID = opt["id"];

	sscanf(eventID.c_str(), "%x", &eventid);

	// search for the event... to get the description...
	result << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><content>";
	//result << <ref>" << ref2string(ref) << "</ref><id>" << eventid << "</id>";
	
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		eServiceReference ref = sapi->service;
		//eServiceReference ref(string2ref(serviceRef));
		current = eDVB::getInstance()->settings->getTransponders()->searchService((eServiceReferenceDVB&)ref);
		
		//result << "<current>" << current << "</current>";
		eEPGCache::getInstance()->Lock();
		if (current)
		{
			EITEvent *event = eEPGCache::getInstance()->lookupEvent((eServiceReferenceDVB&)ref, eventid);
			if (event)
			{
				for (ePtrList<Descriptor>::iterator d(event->descriptor); d != event->descriptor.end(); ++d)
				{
					if (d->Tag() == DESCR_SHORT_EVENT)
					{
						description = ((ShortEventDescriptor*)*d)->event_name;
						eDebug("[ENIGMA_DYN] getEPGDetails: found description = %s", description.c_str());
					}
					if (d->Tag() == DESCR_EXTENDED_EVENT)
					{
						ext_description += ((ExtendedEventDescriptor*)*d)->text;
						eDebug("[ENIGMA_DYN] getEPGDetails: found extended description = %s", ext_description.c_str());
					}
				}
				
				if (!ext_description)
					ext_description = "No detailed information available";

					result << "<path>" << ref2string(ref) << "</path>"
					<< "<channel>" << filter_string(current->service_name) << "</channel>"
					<< "<eventid>" << std::hex << event->event_id << std::dec << "</eventid>"
					<< "<time>" << event->start_time << "</time>"
					<< "<duration>" << event->duration << "</duration>"
					<< "<descr>" << filter_string(description) << "</descr>"
					<< "<description>" << filter_string(ext_description) << "</description>";

				delete event;
			}
		}
	}

	result << "</content>";
	eEPGCache::getInstance()->Unlock();
	
	return result.str();
}

static eString getIPaddr()
{
	eString tmp;
	int sd;
	struct ifreq ifr;
	sd=socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0)
		return "?.?.?.?-socket-error";
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_addr.sa_family = AF_INET; // fixes problems with some linux vers.
	strncpy(ifr.ifr_name, "eth0", sizeof(ifr.ifr_name));
	if (ioctl(sd, SIOCGIFADDR, &ifr) < 0)
		return "?.?.?.?-ioctl-error";
	close(sd);

	return eString().sprintf("%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
}

static eString mplayer(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString vpid, apid;

	vpid = eString().sprintf("%04x", Decoder::current.vpid);
	apid = eString().sprintf("%04x", Decoder::current.apid);

	content->local_header["Content-Type"]="video/mpegfile";
	content->local_header["Cache-Control"] = "no-cache";	
	content->local_header["vpid"] = vpid;
	content->local_header["apid"] = apid;
		
	return "http://" + getIPaddr() + ":31339/" + vpid  +"," + apid;
}


void ezapXMLInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/xml", doStatusXML, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/zap", zaplist, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/epg", getcurepgXML, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/epgdetails", getepgdetailsXML, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/imginfo", getimageinfoXML, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/audio", getAudioChannelsXML, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/mplayer", mplayer, lockWeb);
}
#endif

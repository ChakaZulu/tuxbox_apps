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
extern eString getEITC(eString result);
extern eString getCurService();
extern eString firmwareLevel(eString verid);
extern bool onSameTP(const eServiceReferenceDVB& ref1, const eServiceReferenceDVB &ref2); // implemented in timer.cpp

eString removeXMLBadChars(eString s)
{
//	s.strReplace("\x00", "");
	s.strReplace("\x01", "");
	s.strReplace("\x02", "");
	s.strReplace("\x03", "");
	s.strReplace("\x04", "");
	s.strReplace("\x05", "");
	s.strReplace("\x06", "");
	s.strReplace("\x07", "");
	s.strReplace("\x08", "");
	s.strReplace("\x09", "");
	s.strReplace("\x0a", "");
	s.strReplace("\x0b", "");
	s.strReplace("\x0c", "");
	s.strReplace("\x0d", "");
	s.strReplace("\x0e", "");
	s.strReplace("\x0f", "");
	s.strReplace("\x10", "");
	s.strReplace("\x11", "");
	s.strReplace("\x12", "");
	s.strReplace("\x13", "");
	s.strReplace("\x14", "");
	s.strReplace("\x15", "");
	s.strReplace("\x16", "");
	s.strReplace("\x17", "");
	s.strReplace("\x18", "");
	s.strReplace("\x19", "");
	s.strReplace("\x1a", "");
	s.strReplace("\x1b", "");
	s.strReplace("\x1c", "");
	s.strReplace("\x1d", "");
	s.strReplace("\x1e", "");
	s.strReplace("\x1f", "");
	return s;
}

struct countTimer
{
	int &count;
	bool repeating;
	countTimer(int &count,bool repeating)
		:count(count), repeating(repeating)
	{
	}

	void operator()(ePlaylistEntry *se)
	{
		if (se->type&ePlaylistEntry::isRepeating)
		{
			if (repeating)
				++count;
		}
		else
		{
			if (!repeating)
				++count;
		}
	}
};

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

//TODO:
//<boquets bname="Servicenumbers" bpath="4097:7:0:33fc5:0:0:0:0:0:0:%2fvar%2ftuxbox%2fconfig%2fenigma%2fuserbouquet%2e33fc5%2etv"
//  <name path="1:0:1:a:2:85:c00000:0:0:0:">Test1</name>
//</boquets>

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
			//result += "<a href=\"/xml?mode=zapto,path=" + serviceRef + "\">";
		else{
			result += "<name path=\"" + serviceRef + "\">";
			//result += "<a href=\"/xml?mode=zap,path=" + serviceRef + "\">";
		}
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

struct getXMLEntryString
{
	std::stringstream &result;
	bool repeating;

	getXMLEntryString(std::stringstream &result, bool repeating)
		:result(result), repeating(repeating)
	{
	}

	void operator()(ePlaylistEntry* se)
	{
		if (!repeating && se->type & ePlaylistEntry::isRepeating)
			return;
		if (repeating && !(se->type & ePlaylistEntry::isRepeating))
			return;
		tm startTime = *localtime(&se->time_begin);
		time_t time_end = se->time_begin + se->duration;
		tm endTime = *localtime(&time_end);

		eString description = se->service.descr;
		eString channel = getLeft(description, '/');
		if (!channel)
		{
			eService *service = eDVB::getInstance()->settings->getTransponders()->searchService(se->service);
			if (service)
				channel = filter_string(service->service_name);
		}
		if (!channel)
			channel = "No channel available";

		description = getRight(description, '/');
		if (!description)
			description = "No description available";

		result 	<< std::setw(2) << startTime.tm_mday << '.'
			<< std::setw(2) << startTime.tm_mon+1 << ". - "
			<< std::setw(2) << startTime.tm_hour << ':'
			<< std::setw(2) << startTime.tm_min
			<< " / "
			<< std::setw(2) << endTime.tm_mday << '.'
			<< std::setw(2) << endTime.tm_mon+1 << ". - "
		 	<< std::setw(2) << endTime.tm_hour << ':'
			<< std::setw(2) << endTime.tm_min
			<< "<br/>"
			<< channel
			<< "<br/>"
			<< description
			<< "<br/>";
	}
};

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
		"<content>\n"
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
	result += "</content>\n";
	
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

			result << removeXMLBadChars(it->text);
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

	eString serviceRef = opt["ref"];
//	if (serviceRef)
//		ref = string2ref(serviceRef);
//	else
		ref = sapi->service;

	eDebug("[ENIGMA_DYN] getcurepg: opts = %s, serviceRef = %s", opts.c_str(), serviceRef.c_str());

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
static eString xml_web_root(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;

	std::map<eString,eString> opt = getRequestOptions(opts, ',');
	eString mode = opt["mode"];
	eString spath = opt["path"];

	content->local_header["Content-Type"]="text/xml; charset=utf-8";
	content->local_header["Cache-Control"] = "no-cache";

	if (mode == "zap")
	{
		if (opts.find("path") == eString::npos)
			spath = zap[ZAPMODETV][ZAPSUBMODEBOUQUETS];
		
		result = readFile(TEMPLATE_DIR + "wapzap.tmp");
		result.strReplace("#BODY#", getXMLZapContent(spath));
	}
	else
	if (mode == "zapto")
	{
		eServiceReference current_service = string2ref(spath);

		if (!(current_service.flags&eServiceReference::isDirectory))	// is playable
			eZapMain::getInstance()->playService(current_service, eZapMain::psSetMode|eZapMain::psDontAdd);

		result = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><content>Zap complete.</content>";
	}
	else
	{
		result = readFile(TEMPLATE_DIR + "wap.tmp");
		result = getEITC(result);
		result.strReplace("#SERVICE#", getCurService());
	}

	return result;
}

void ezapXMLInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/xml", xml_web_root, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/status", doStatusXML, false);
	dyn_resolver->addDyn("GET", "/xml/epg", getcurepgXML, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/epgdetails", getepgdetailsXML, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/imginfo", getimageinfoXML, lockWeb);
	dyn_resolver->addDyn("GET", "/xml/audio", getAudioChannelsXML, lockWeb);
}

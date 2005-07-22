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
#include <linux/input.h>
#include <iomanip>
#include <iostream>
#include <fstream>

#include <enigma.h>
#include <timer.h>
#include <enigma_main.h>
#include <enigma_plugins.h>
#include <enigma_standby.h>
#include <sselect.h>
#include <upgrade.h>
#include <math.h>

#include <lib/dvb/frontend.h>
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
#include <lib/gdi/fb.h>
#include <lib/gdi/glcddc.h>
#include <lib/gdi/gfbdc.h>
#include <lib/gdi/epng.h>
#include <lib/gui/emessage.h>
#include <lib/system/http_dyn.h>
#include <lib/system/econfig.h>
#include <lib/system/info.h>
#include <lib/system/dmfp.h>
#include <lib/system/file_eraser.h>
#include <enigma_dyn.h>
#include <enigma_dyn_utils.h>
#include <enigma_dyn_misc.h>
#include <enigma_processutils.h>
#include <epgwindow.h>
#include <streaminfo.h>

extern bool onSameTP(const eServiceReferenceDVB& ref1, const eServiceReferenceDVB &ref2); // implemented in timer.cpp
extern bool canPlayService(const eServiceReference& ref); // implemented in timer.cpp
extern bool playService(const eServiceReference& ref);
extern eString getAudioChannels(void);
extern eString getSubChannels(void);
extern eString firmwareLevel(eString verid);

static eString doStatus(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString name, provider, vpid, apid, pcrpid, tpid, vidform("n/a"), tsid, onid, sid, pmt;

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result;
	time_t atime;
	time(&atime);
	atime += eDVB::getInstance()->time_difference;
	result = "<html>\n"
		CHARSETMETA
		"<head>\n"
		"<title>Enigma Status</title>\n"
		"<link rel=stylesheet type=\"text/css\" href=\"/webif.css\">\n"
		"</head>\n"
		"<body>\n"
		"<h1>Enigma status</h1>\n"
		"<table>\n"
		"<tr><td>Current time:</td><td>" + eString(ctime(&atime)) + "</td></tr>\n"
		"<tr><td>WebIf-Version:</td><td>" + eString(WEBIFVERSION) + "</td></tr>\n"
		"<tr><td>Standby:</td><td>";
		if (eZapMain::getInstance()->isSleeping())
			result += "ON";
		else
			result += "OFF";
	result += "</td></tr>\n";
#ifndef DISABLE_FILE
	result += "<tr><td>Recording:</td><td>";
		if (eZapMain::getInstance()->isRecording())
			result += "ON";
		else
			result += "OFF";
#else
	result += "<tr><td>Recording:</td><td>OFF";
#endif
	result += "</td></tr>\n";
	result += "<tr><td>Mode:</td><td>" + eString().sprintf("%d", eZapMain::getInstance()->getMode()) + "</td></tr>\n";

	eString sRef;
	if (eServiceInterface::getInstance()->service)
		sRef = eServiceInterface::getInstance()->service.toString();
	result += "<tr><td>Current service reference:</td><td>" + sRef + "</td></tr>\n";

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
	vpid = eString().sprintf("%04xh (%dd)", Decoder::current.vpid, Decoder::current.vpid);
	apid = eString().sprintf("%04xh (%dd)", Decoder::current.apid, Decoder::current.apid);
	pcrpid = eString().sprintf("%04xh (%dd)", Decoder::current.pcrpid, Decoder::current.pcrpid);
	tpid = eString().sprintf("%04xh (%dd)", Decoder::current.tpid, Decoder::current.tpid);
	tsid = eString().sprintf("%04xh", sapi->service.getTransportStreamID().get());
	onid = eString().sprintf("%04xh", sapi->service.getOriginalNetworkID().get());
	sid = eString().sprintf("%04xh", sapi->service.getServiceID().get());
	pmt = eString().sprintf("%04xh", Decoder::current.pmtpid);

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
	result += "<tr><td>name:</td><td>" + name + "</td></tr>\n";
	result += "<tr><td>provider:</td><td>" + provider + "</td></tr>\n";
	result += "<tr><td>vpid:</td><td>" + vpid + "</td></tr>\n";
	result += "<tr><td>apid:</td><td>" + apid + "</td></tr>\n";
	result += "<tr><td>pcrpid:</td><td>" + pcrpid + "</td></tr>\n";
	result += "<tr><td>tpid:</td><td>" + tpid + "</td></tr>\n";
	result += "<tr><td>tsid:</td><td>" + tsid + "</td></tr>\n";
	result += "<tr><td>onid:</td><td>" + onid + "</td></tr>\n";
	result += "<tr><td>sid:</td><td>" + sid + "</td></tr>\n";
	result += "<tr><td>pmt:</td><td>" + pmt + "</td></tr>\n";
	result += "<tr><td>vidformat:<td>" + vidform + "</td></tr>\n";

	result += "</table>\n"
		"</body>\n"
		"</html>\n";
	return result;
}

static eString switchService(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";

	int service_id = -1, dvb_namespace = -1, original_network_id = -1, transport_stream_id = -1, service_type = -1;
	unsigned int optval = opt.find("=");
	if (optval != eString::npos)
		opt = opt.mid(optval + 1);
	if (opt.length())
		sscanf(opt.c_str(), "%x:%x:%x:%x:%x", &service_id, &dvb_namespace, &transport_stream_id, &original_network_id, &service_type);

	eString result;

	if ((service_id != -1) && (original_network_id != -1) && (transport_stream_id != -1) && (service_type != -1))
	{
		eServiceInterface *iface = eServiceInterface::getInstance();
		if (!iface)
			return "-1";
		eServiceReferenceDVB *ref = new eServiceReferenceDVB(eDVBNamespace(dvb_namespace), eTransportStreamID(transport_stream_id), eOriginalNetworkID(original_network_id), eServiceID(service_id), service_type);
#ifndef DISABLE_FILE
		if (!canPlayService(*ref))
		{
			delete ref;
			return "-1";
		}
#endif
		if (playService(*ref))
			result = "0";
		else
			result = "-1";
		delete ref;
	}
	else
		result = "-1";

	return result;
}

static eString audioChannels(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	return getAudioChannels();
}

static eString videoChannels(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	return getSubChannels();
}

static eString getPMT(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	//"x-application/PMT";
	PMT *pmt = eDVB::getInstance()->getPMT();
	if (!pmt)
		return "result=ERROR\n";
	eString result = "result=+ok";
	result += "PMT" + eString().sprintf("(%04x)\n", pmt->pid);
	result += "program_number=" + eString().sprintf("%04x\n", pmt->program_number);
	result += "PCR_PID=" + eString().sprintf("%04x\n", pmt->PCR_PID);
	result += "program_info\n";
	for (ePtrList<Descriptor>::iterator d(pmt->program_info); d != pmt->program_info.end(); ++d)
		result += d->toString();
	for (ePtrList<PMTEntry>::iterator s(pmt->streams); s != pmt->streams.end(); ++s)
	{
		result += "PMTEntry\n";
		result += "stream_type=" + eString().sprintf("%02x\n", s->stream_type);
		result += "elementary_PID=" + eString().sprintf("%04x\n", s->elementary_PID);
		result += "ES_info\n";
		for (ePtrList<Descriptor>::iterator d(s->ES_info); d != s->ES_info.end(); ++d)
			result += d->toString();
	}
	pmt->unlock();
	return result;
}

static eString getEIT(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	//"x-application/PMT";
	EIT *eit = eDVB::getInstance()->getEIT();
	if (!eit)
		return "result=ERROR\n";
	eString result = "result=+ok";
	result += "EIT" + eString().sprintf("(%04x)\n", eit->service_id);
	result += "original_network_id=" + eString().sprintf("%04x\n", eit->original_network_id);
	result += "transport_stream_id=" + eString().sprintf("%04x\n", eit->transport_stream_id);
	result += "events\n";
	for (ePtrList<EITEvent>::iterator s(eit->events); s != eit->events.end(); ++s)
	{
		result += "EITEvent\n";
		result += "event_id=" + eString().sprintf("%04x\n", s->event_id);
		result += "start_time=" + eString().sprintf("%04x\n", s->start_time);
		result += "duration=" + eString().sprintf("%04x\n", s->duration);
		result += "running_status=" + eString().sprintf("%d\n", s->running_status);
		result += "free_CA_mode=" + eString().sprintf("%d\n", s->free_CA_mode);
		result += "descriptors\n";
		for (ePtrList<Descriptor>::iterator d(s->descriptor); d != s->descriptor.end(); ++d)
			result += d->toString();
	}
	eit->unlock();
	return result;
}

static eString version(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
        eString versionFile = "/.version";

        if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7020)
                versionFile = "/etc/image-version";

	content->local_header["Content-Type"]="text/plain";
	return firmwareLevel(getAttribute(versionFile, "version"));
}

static eString channels_getcurrent(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/plain; charset=utf-8";

	if (eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI())
		if (eServiceDVB *current=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service))
			return current->service_name.c_str();

	return "-1";
}

static eString xmessage(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	std::map<eString, eString> opts = getRequestOptions(opt, '&');

	if (opts.find("timeout") == opts.end())
		return "E: no timeout set";

	if (opts.find("caption") == opts.end())
		return "E: no caption set";

	if (opts.find("body") == opts.end())
		return "E: no body set";

	int type = -1;
	if (opts.find("type") != opts.end())
		type=atoi(opts["type"].c_str());

	int timeout = atoi(opts["timeout"].c_str());

	eZapMain::getInstance()->postMessage(eZapMessage(1, opts["caption"], opts["body"], timeout), type != -1);

	return eString("+ok");
}

static eString reload_settings(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if (!eDVB::getInstance())
		return "-no dvb\n";
	if (eDVB::getInstance()->settings)
	{
		eDVB::getInstance()->settings->loadServices();
		eDVB::getInstance()->settings->loadBouquets();
		eZap::getInstance()->getServiceSelector()->actualize();
		eServiceReference::loadLockedList((eZapMain::getInstance()->getEplPath()+"/services.locked").c_str());
		return "+ok";
	}
	return "-no settings to load\n";
}

static eString reload_encoding_table(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	return eString::readEncodingFile() ? "-couldn't open encoding file" : "+ok";
}

#ifndef DISABLE_FILE
static eString load_recordings(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eZapMain::getInstance()->loadRecordings();
	return "+ok";
}

static eString save_recordings(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eZapMain::getInstance()->saveRecordings();
	return "+ok";
}
#endif

static eString load_timerList(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eTimerManager::getInstance()->loadTimerList();
	return "+ok";
}

static eString save_timerList(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eTimerManager::getInstance()->saveTimerList();
	return "+ok";
}

static eString load_playlist(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eZapMain::getInstance()->loadPlaylist();
	return "+ok";
}

static eString save_playlist(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eZapMain::getInstance()->savePlaylist();
	return "+ok";
}

static eString load_userBouquets(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eZapMain::getInstance()->loadUserBouquets();
	return "+ok";
}

static eString save_userBouquets(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eZapMain::getInstance()->saveUserBouquets();
	return "+ok";
}

static eString listDirectory(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString answer;
	content->local_header["Content-Type"]="text/plain; charset=utf-8";
	answer.sprintf(
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<directory path=\"%s\" dircount=\"\" filecount=\"\" linkcount=\"\">\n",
		opt.length() ? opt.c_str() : "?");
	DIR *d = 0;
	if (opt.length())
	{
		if (opt[opt.length() - 1] != '/')
			opt += '/';
		d = opendir(httpUnescape(opt).c_str());
	}
	if (d)
	{
		char buffer[255];
		int dircount, filecount, linkcount;
		dircount = filecount = linkcount = 0;
		while (struct dirent64 *e = readdir64(d))
		{
			eString filename = opt;
			filename += e->d_name;

			struct stat64 s;
			if (lstat64(filename.c_str(), &s) < 0)
				continue;
			if (S_ISLNK(s.st_mode))
			{
				int count = readlink(filename.c_str(), buffer, 255);
				eString dest(buffer, count);
				answer += eString().sprintf("\t<object type=\"link\" name=\"%s\" dest=\"%s\"/>\n", e->d_name, dest.c_str());
				++linkcount;
			}
			else if (S_ISDIR(s.st_mode))
			{
				answer += eString().sprintf("\t<object type=\"directory\" name=\"%s\"/>\n", e->d_name);
				++dircount;
			}
			else if (S_ISREG(s.st_mode))
			{
				answer+=eString().sprintf("\t<object type=\"file\" name=\"%s\" size=\"%lld\"/>\n",
					e->d_name,
					s.st_size);
				++filecount;
			}
		}
		unsigned int pos = answer.find("dircount=\"");
		answer.insert(pos + 10, eString().sprintf("%d", dircount));
		pos = answer.find("filecount=\"");
		answer.insert(pos + 11, eString().sprintf("%d", filecount));
		pos = answer.find("linkcount=\"");
		answer.insert(pos + 11, eString().sprintf("%d", linkcount));
		closedir(d);
		answer += "</directory>\n";
		return answer;
	}
	else
		return eString().sprintf("E: couldn't read directory %s", opt.length() ? opt.c_str() : "?");
}

static eString makeDirectory(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if ( mkdir(httpUnescape(opt).c_str(),0755) )
		return eString().sprintf("E: create directory %s failed", opt.c_str());
	return "+ok";
}

static eString removeDirectory(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if ( rmdir(httpUnescape(opt).c_str()) )
		return eString().sprintf("E: remove directory %s failed", opt.c_str());
	return "+ok";
}

static eString removeFile(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if ( unlink(httpUnescape(opt).c_str()) )
		return eString().sprintf("E: remove file %s failed", opt.c_str());
	return "+ok";
}

static eString moveFile(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	std::map<eString,eString> opts=getRequestOptions(opt, '&');
	if (opts.find("source") == opts.end() || !opts["source"].length())
		return "E: option source missing or empty source given";
	if (opts.find("dest") == opts.end() || !opts["dest"].length())
		return "E: option dest missing or empty dest given";
	if ( rename(opts["source"].c_str(), opts["dest"].c_str()) )
		return eString().sprintf("E: cannot move %s to %s", opts["source"].c_str(), opts["dest"].c_str());
	return "+ok";
}

static eString createSymlink(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	std::map<eString,eString> opts=getRequestOptions(opt, '&');
	if (opts.find("source") == opts.end() || !opts["source"].length())
		return "E: option source missing or empty source given";
	if (opts.find("dest") == opts.end() || !opts["dest"].length())
		return "E: option dest missing or empty dest given";
	if ( ::link( opts["source"].c_str(), opts["dest"].c_str()) )
		return eString().sprintf("E: cannot create symlink %s to %s", opts["source"].c_str(), opts["dest"].c_str());
	return "+ok";
}

static eString getCurrentVpidApid(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if (opt == "getpids")
		return eString().sprintf("%d\n%d\n", Decoder::current.vpid, Decoder::current.apid);
	else if (opt == "getallpids")
	{
		std::stringstream str;
		str << std::setfill('0');
		if ( Decoder::current.vpid != -1 )
			str << std::setw(5) << Decoder::current.vpid << std::endl;
		eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
		if (!sapi || !sapi->service)
			return "error\n";
		std::list<eDVBServiceController::audioStream> &audioStreams = sapi->audioStreams;
		for (std::list<eDVBServiceController::audioStream>::iterator it=audioStreams.begin(); it != audioStreams.end(); ++it)
			str << std::setw(5) << it->pmtentry->elementary_PID << ' ' << it->text << std::endl;
		if ( Decoder::current.tpid != -1 )
			str << std::setw(5) << Decoder::current.tpid << " vtxt\n";
		if ( Decoder::current.pmtpid != -1 )
			str << std::setw(5) << Decoder::current.pmtpid << " pmt\n";
		return str.str();
	}
	else
		return "ok\n";
}

static eString neutrino_getonidsid(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi || !sapi->service)
		return "200\n";

	int onidsid = (sapi->service.getOriginalNetworkID().get() << 8)
		| sapi->service.getServiceID().get();

	return eString().sprintf("%d\n", onidsid);
}

struct addToString
{
	eString &dest;
	eServiceReferenceDVB &current;
	addToString(eString &dest, eServiceReferenceDVB &current)
		:dest(dest), current(current)
	{
	}
	void operator()(const eServiceReference& s)
	{
		if (onSameTP(current,(eServiceReferenceDVB&)s))
		{
			dest += s.toString();
			eServiceDVB *service = eTransponderList::getInstance()->searchService(s);
			if (service)
			{
				dest+=';';
				dest+=filter_string(service->service_name);
				for(int i = 0; i < (int)eServiceDVB::cacheMax; ++i)
				{
					int d=service->get((eServiceDVB::cacheID)i);
					if (d != -1)
						dest+=eString().sprintf(";%02d%04x", i, d);
				}
			}
			dest += '\n';
		}
	}
};

static eString getTransponderServices(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/plain; charset=utf-8";
	eServiceReferenceDVB cur = (eServiceReferenceDVB&)eServiceInterface::getInstance()->service;
	if (cur.type == eServiceReference::idDVB && !cur.path)
	{
		eString result;
		eTransponderList::getInstance()->forEachServiceReference(addToString(result,cur));
		if (result)
			return result;
		else
			return "E: no other services on the current transponder";
	}
	return "E: no DVB service is running.. or this is a playback";
}

struct listContent: public Object
{
	eString &result;
	eServiceInterface *iface;
	bool listCont;
	listContent(const eServiceReference &service, eString &result, bool listCont)
		:result(result), iface(eServiceInterface::getInstance()), listCont(listCont)
	{
		Signal1<void, const eServiceReference&> cbSignal;
		CONNECT(cbSignal, listContent::addToString);
		iface->enterDirectory(service, cbSignal);
		iface->leaveDirectory(service);
	}
	void addToString(const eServiceReference& ref)
	{
		// sorry.. at moment we dont show any directory.. or locked service in webif
		if (ref.isLocked() && eConfig::getInstance()->pLockActive())
			return;

		eService *service = iface ? iface->addRef(ref) : 0;
		result += ref.toString();
		result += ";";
		if (ref.descr)
			result += filter_string(ref.descr);
		else if (service)
		{
			result += filter_string(service->service_name);
			if (ref.type == eServiceReference::idDVB && !(ref.flags & eServiceReference::isDirectory))
			{
				result += ';';
				result += filter_string(((eServiceDVB*)service)->service_provider);
			}
		}
		else
		{
			result += "unnamed service";
			if (ref.type == eServiceReference::idDVB && !(ref.flags & eServiceReference::isDirectory))
				result += ";unnamed provider";
		}
		if (ref.type == eServiceReference::idDVB && !(ref.flags & eServiceReference::isDirectory))
		{
			const eServiceReferenceDVB& dvb_ref = (const eServiceReferenceDVB&)ref;
			eTransponder *tp = eTransponderList::getInstance()->searchTS(
				dvb_ref.getDVBNamespace(),
				dvb_ref.getTransportStreamID(),
				dvb_ref.getOriginalNetworkID());
			if (tp && tp->satellite.isValid())
			{
				result += ';';
				result += eString().setNum(tp->satellite.orbital_position);
			}
		}
		result += "\n";
		if (service)
			iface->removeRef(ref);
		if (listCont && ref.flags & eServiceReference::isDirectory)
			listContent(ref, result, false);
	}
};

static eString getServices(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/plain; charset=utf-8";
	std::map<eString,eString> opts=getRequestOptions(opt, '&');

	if (!opts["ref"])
		return "E: no ref given";

	bool listCont = opts["listContent"] == "true";

	eString result;
	eServiceReference ref(opts["ref"]);
	listContent t(ref, result, listCont);

	if (result)
		return result;

	return "E: error during list services";
}

struct appendonidsidnamestr
{
	eString &str;
	appendonidsidnamestr(eString &s)
		:str(s)
	{
	}
	void operator()(eServiceDVB& s)
	{
		str += filter_string(eString().sprintf("%d %s\n",
			(s.original_network_id.get() << 8) | s.service_id.get(),
			s.service_name.c_str()));
	}
};

static eString neutrino_getchannellist(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString channelstring;

	eTransponderList::getInstance()->forEachService(appendonidsidnamestr(channelstring));

	return channelstring;
}

static eString startPlugin(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	std::map<eString, eString> opts = getRequestOptions(opt, '&');

	if (opts.find("name") == opts.end())
		return "E: no plugin name given";

	eZapPlugins plugins(-1);
	eString path;
	if (opts.find("path") != opts.end())
	{
		path = opts["path"];
		if (path.length() && (path[path.length()-1] != '/'))
			path += '/';
	}
	if (ePluginThread::getInstance())
		ePluginThread::getInstance()->kill(true);

	return plugins.execPluginByName((path + opts["name"]).c_str());
}

static eString audio(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString result;
	eString volume = opt["volume"];
	if (volume)
	{
		int vol = atoi(volume.c_str());
		eAVSwitch::getInstance()->changeVolume(1, vol);
		result += "Volume set.<br>\n";
	}
	eString mute = opt["mute"];
	if (mute)
	{
		eAVSwitch::getInstance()->toggleMute();
		result += "mute set<br>\n";
	}
	result += eString().sprintf("volume: %d<br>\nmute: %d<br>\n", eAVSwitch::getInstance()->getVolume(), eAVSwitch::getInstance()->getMute());
	return result;
}

static eString getCurrentServiceRef(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if (eServiceInterface::getInstance()->service)
		return eServiceInterface::getInstance()->service.toString();
	else
		return "E:no service running";
}


static eString getstreaminfo(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::stringstream result;
	eString name,
		provider,
		vpid,
		apid,
		pcrpid,
		tpid,
		vidform("n/a"),
		tsid,
		onid,
		sid,
		pmt;

	content->local_header["Content-Type"]="text/html; charset=utf-8";

	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "not available";

	eServiceDVB *service=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
	if (service)
	{
		name = filter_string(service->service_name);
		provider = filter_string(service->service_provider);
	}
	vpid = eString().sprintf("%04xh (%dd)", Decoder::current.vpid, Decoder::current.vpid);
	apid = eString().sprintf("%04xh (%dd)", Decoder::current.apid, Decoder::current.apid);
	pcrpid = eString().sprintf("%04xh (%dd)", Decoder::current.pcrpid, Decoder::current.pcrpid);
	tpid = eString().sprintf("%04xh (%dd)", Decoder::current.tpid, Decoder::current.tpid);
	tsid = eString().sprintf("%04xh", sapi->service.getTransportStreamID().get());
	onid = eString().sprintf("%04xh", sapi->service.getOriginalNetworkID().get());
	sid = eString().sprintf("%04xh", sapi->service.getServiceID().get());
	pmt = eString().sprintf("%04xh", Decoder::current.pmtpid);

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

	result << "<html>" CHARSETMETA "<head><title>Stream Info</title><link rel=\"stylesheet\" type=\"text/css\" href=\"/webif.css\"></head><body bgcolor=#ffffff>"
		"<!-- " << sapi->service.toString() << "-->" << std::endl <<
		"<table cellspacing=5 cellpadding=0 border=0>"
		"<tr><td>Name:</td><td>" << name << "</td></tr>"
		"<tr><td>Provider:</td><td>" << provider << "</td></tr>";
	eString sRef;
	if (eServiceInterface::getInstance()->service)
		sRef = eServiceInterface::getInstance()->service.toString();
	result << "<tr><td>Service reference:</td><td>" << sRef << "</td></tr>"
		"<tr><td>VPID:</td><td>" << vpid << "</td></tr>"
		"<tr><td>APID:</td><td>" << apid << "</td></tr>"
		"<tr><td>PCRPID:</td><td>" << pcrpid << "</td></tr>"
		"<tr><td>TPID:</td><td>" << tpid << "</td></tr>"
		"<tr><td>TSID:</td><td>" << tsid << "</td></tr>"
		"<tr><td>ONID:</td><td>" << onid << "</td></tr>"
		"<tr><td>SID:</td><td>" << sid << "</td></tr>"
		"<tr><td>PMT:</td><td>" << pmt << "</td></tr>"
		"<tr><td>Video Format:<td>" << vidform << "</td></tr>"
		"</table>"
		"</body>"
		"</html>";

	return result.str();
}

void ezapMiscInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb)
{
	dyn_resolver->addDyn("GET", "/cgi-bin/ls", listDirectory, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/mkdir", makeDirectory, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/rmdir", removeDirectory, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/rm", removeFile, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/mv", moveFile, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/ln", createSymlink, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/status", doStatus, true); //always pw protected for dreamtv
	dyn_resolver->addDyn("GET", "/cgi-bin/switchService", switchService, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/getPMT", getPMT, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/getEIT", getEIT, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/xmessage", xmessage, lockWeb);
	dyn_resolver->addDyn("GET", "/version", version, lockWeb);
	dyn_resolver->addDyn("GET", "/channels/getcurrent", channels_getcurrent, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadSettings", reload_settings, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadEncodingTable", reload_encoding_table, lockWeb);
#ifndef DISABLE_FILE
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadRecordings", load_recordings, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/saveRecordings", save_recordings, lockWeb);
#endif
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadPlaylist", load_playlist, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/savePlaylist", save_playlist, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadUserBouquets", load_userBouquets, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/saveUserBouquets", save_userBouquets, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadTimerList", load_timerList, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/saveTimerList", save_timerList, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/startPlugin", startPlugin, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/audio", audio, lockWeb);
// functions needed by dreamtv
	dyn_resolver->addDyn("GET", "/cgi-bin/audioChannels", audioChannels, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/videoChannels", videoChannels, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/currentTransponderServices", getTransponderServices, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/getServices", getServices, lockWeb);
	dyn_resolver->addDyn("GET", "/control/zapto", getCurrentVpidApid, false); // this dont really zap.. only used to return currently used pids;
	dyn_resolver->addDyn("GET", "/control/getonidsid", neutrino_getonidsid, lockWeb);
	dyn_resolver->addDyn("GET", "/control/channellist", neutrino_getchannellist, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/currentService", getCurrentServiceRef, lockWeb);
// function needed by bitcntrl
	dyn_resolver->addDyn("GET", "/cgi-bin/streaminfo", getstreaminfo, lockWeb);
}


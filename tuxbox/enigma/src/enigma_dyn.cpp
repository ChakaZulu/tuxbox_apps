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
#include <timer.h>
#include <enigma_main.h>
#include <enigma_plugins.h>
#include <enigma_standby.h>
#include <sselect.h>
#include <upgrade.h>

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
#include <enigma_dyn.h>
#include <enigma_dyn_utils.h>
#include <enigma_dyn_mount.h>

using namespace std;

#define WEBXFACEVERSION "1.3.2"

int smallScreen = 0;

int currentBouquet = 0;
int currentChannel = -1;

int zapMode = ZAPMODETV;
int zapSubMode = ZAPSUBMODEBOUQUETS;

extern bool onSameTP(const eServiceReferenceDVB& ref1, const eServiceReferenceDVB &ref2); // implemented in timer.cpp

eString zap[4][5] =
{
	{"TV", ";0:7:1:0:0:0:0:0:0:0:", /* Satellites */ ";1:15:fffffffc:12:0:0:0:0:0:0:", /* Providers */ ";1:15:ffffffff:12:ffffffff:0:0:0:0:0:", /* Bouquets */ ";4097:7:0:6:0:0:0:0:0:0:"},
	{"Radio", ";0:7:2:0:0:0:0:0:0:0:", /* Satellites */ ";1:15:fffffffc:4:0:0:0:0:0:0:", /* Providers */ ";1:15:ffffffff:4:ffffffff:0:0:0:0:0:", /* Bouquets */ ";4097:7:0:4:0:0:0:0:0:0:"},
	{"Data", ";0:7:6:0:0:0:0:0:0:0:", /* Satellites */ ";1:15:fffffffc:ffffffe9:0:0:0:0:0:0:", /* Providers */ ";1:15:ffffffff:ffffffe9:ffffffff:0:0:0:0:0:", /* Bouquets */ ""},
	{"Recordings", ";4097:7:0:1:0:0:0:0:0:0:", /* Satellites */ "", /* Providers */ "", /* Bouquets */ ""}
};

eString firmwareLevel(eString verid)
{
	eString result = "unknown";

	if (verid)
	{
		int type = atoi(verid.left(1).c_str());
		char *typea[3];
		typea[0] = "release";
		typea[1] = "beta";
		typea[2] = "internal";
		eString ver = verid.mid(1, 3);
		eString date = verid.mid(4, 8);
//		eString time = verid.mid(12, 4);
		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000)
			result = eString(typea[type%3]) + eString(" ") + ver[0] + "." + ver[1] + "." + ver[2]+ ", " + date.mid(6, 2) + "." + date.mid(4, 2) + "." + date.left(4);
		else
			result = eString().sprintf("%s %c.%d. %s", typea[type%3], ver[0], atoi(eString().sprintf("%c%c", ver[1], ver[2]).c_str()), (date.mid(6, 2) + "." + date.mid(4, 2) + "." + date.left(4)).c_str());
	}
	return result;
}

eString getMsgWindow(eString title, eString msg)
{
	eString result = readFile(TEMPLATE_DIR + "msgWindow.tmp");
	result.strReplace("#TITLE#", title);
	result.strReplace("#MSG#", msg);
	return result;
}

static eString tvMessageWindow(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	return readFile(TEMPLATE_DIR + "sendMessage.tmp");
}

static eString getControlPlugins(void)
{
	std::stringstream result;
	struct dirent **e;
	eString line;
	int n = scandir("/lib/tuxbox/plugins", &e, 0, alphasort);
	if (n > 0)
	{
		result << "<table width=100% border=1 cellspacing=0 cellpadding=0>";
		for (int i = 0; i < n; i++)
		{
			line = e[i]->d_name;
			if (line.find(".cfg") != eString::npos)
			{
				result  << "<tr>"
					<< "<td width=100>"
					<< button(100, "Start", GREEN, "javascript:startPlugin('" + getLeft(line, '.') + ".cfg')")
					<< "</td>"
					<< "<td>"
					<< getAttribute("/lib/tuxbox/plugins/" + line, "name")
					<< "</td>"
					<< "<td>"
					<< getAttribute("/lib/tuxbox/plugins/" + line, "desc")
					<< "</td>"
					<< "</tr>";
			}
		}
		result << "</table>";
		result << "<br>";
		result << button(100, "Stop", RED, "javascript:stopPlugin()");
	}
	else
		result << "No plugins found.";
	return result.str();
}

static int getOSDShot(eString mode)
{
	gPixmap *p = 0;
#ifndef DISABLE_LCD
	if (mode == "lcd")
		p = &gLCDDC::getInstance()->getPixmap();
	else
#endif
		p = &gFBDC::getInstance()->getPixmap();

	if (p)
		if (!savePNG("/tmp/osdshot.png", p))
			return 0;

	return -1;
}

static eString osdshot(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString,eString> opt=getRequestOptions(opts, '&');

	if (getOSDShot(opt["mode"]) == 0)
	{
		content->local_header["Location"]="/root/tmp/osdshot.png";
		content->code = 307;
		return "+ok";
	}
	else
		return "-error";
}

static eString doStatus(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result;
	time_t atime;
	time(&atime);
	atime += eDVB::getInstance()->time_difference;
	result="<html>\n"
		CHARSETMETA
		"<head>\n"
		"<title>enigma status</title>\n"
		"<link rel=stylesheet type=\"text/css\" href=\"/webif.css\">\n"
		"</head>\n"
		"<body>\n"
		"<h1>Enigma status</h1>\n"
		"<table>\n"
		"<tr><td>current time:</td><td>" + eString(ctime(&atime)) + "</td></tr>\n"
		"</table>\n"
		"</body>\n"
		"</html>\n";
	return result;
}

#ifndef DISABLE_FILE
static eString pause(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eZapMain::getInstance()->pause();
	return "+ok";
}

static eString play(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eZapMain::getInstance()->play();
	return "+ok";
}

static eString stop(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eZapMain::getInstance()->stop();
	return "+ok";
}

static eString record(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString command = opt["command"];
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	if (command == "start")
	{
		eZapMain::getInstance()->recordDVR(1,0);
		return "<html>" CHARSETMETA "<head><title>Record</title></head><body>Recording started...</body></html>";
	}
	else
	{
		eZapMain::getInstance()->recordDVR(0,0);
		return "<html>" CHARSETMETA "<head><title>Record</title></head><body>Recording stopped.</body></html>";
	}
}
#endif

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
		if (eDVB::getInstance()->recorder && !ref->path)
		{
			int canHandleTwoScrambledServices = 0;
			eConfig::getInstance()->getKey("/ezap/ci/handleTwoServices", canHandleTwoScrambledServices);

			if (!canHandleTwoScrambledServices && eDVB::getInstance()->recorder->scrambled)
			{
				delete ref;
				return "-1";
			}
			if (!onSameTP(*ref,eDVB::getInstance()->recorder->recRef))
			{
				delete ref;
				return "-1";
			}
		}
#endif
		eZapMain::getInstance()->playService(*ref, eZapMain::psSetMode|eZapMain::psDontAdd);
		delete ref;
		result = "0";
	}
	else
		result = "-1";

	return result;
}

static eString admin(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString command = opt["command"];
	eString requester = opt["requester"];
	if (command)
	{
		if (command == "shutdown")
		{
			if (eSystemInfo::getInstance()->canShutdown())
			{
				eZap::getInstance()->quit();
				if (requester == "webif")
					return "<html>" CHARSETMETA "<head><title>Shutdown</title></head><body>Shutdown initiated...</body></html>";
				else
					return "<html>" CHARSETMETA "<head><title>Shutdown</title></head><body>Shutdown initiated.</body></html>";
			}
		}
		else if (command == "reboot")
		{
			eZap::getInstance()->quit(4);
			if (requester == "webif")
				return "<html>" CHARSETMETA "<head><title>Reboot</title></head><body>Reboot initiated...</body></html>";
			else
				return "<html>" CHARSETMETA "<head><title>Reboot</title></head><body>Reboot initiated.</body></html>";
		}
		else if (command == "restart")
		{
			eZap::getInstance()->quit(2);
			if (requester == "webif")
				return "<html>" CHARSETMETA "<head><title>Restart Enigma</title></head><body>Restart initiated...</body></html>";
			else
				return "<html>" CHARSETMETA "<head><title>Restart of enigma is initiated.</title></head><body>Restart initiated</body></html>";
		}
		else if (command == "wakeup")
		{
			if (eZapStandby::getInstance())
			{
				eZapMain::getInstance()->wakeUp();
				if (requester == "webif")
					return "<html>" CHARSETMETA "<head><title>Wakeup</title></head><body>Enigma is waking up...</body></html>";
				else
					return "<html>" CHARSETMETA "<head><title>Wakeup</title></head><body>enigma is waking up.</body></html>";
			}
			if (requester == "webif")
				return "<html>" CHARSETMETA "<head><title>Wakeup</title></head><body>Enigma doesn't sleep.</body></html>";
			else
				return "<html>" CHARSETMETA "<head><title>Wakeup</title></head><body>enigma doesn't sleep :)</body></html>";
		}
		else if (command == "standby")
		{
			if (eZapStandby::getInstance())
				if (requester == "webif")
					return "<html>" CHARSETMETA "<head><title>Standby</title></head><body>Enigma is already sleeping.</body></html>";
				else
					return "<html>" CHARSETMETA "<head><title>Standby</title></head><body>enigma is already sleeping</body></html>";
			eZapMain::getInstance()->gotoStandby();
			if (requester == "webif")
				return "<html>" CHARSETMETA "<head><title>Standby</title></head><body>Standby initiated...</body></html>";
			else
				return "<html>" CHARSETMETA "<head><title>Standby</title></head><body>enigma is sleeping now</body></html>";
		}
	}
	return "<html>" CHARSETMETA "<head><title>Error</title></head><body>Unknown admin command.(valid commands are: shutdown, reboot, restart, standby, wakeup) </body></html>";
}

static eString admin2(eString command)
{
	if (command == "shutdown")
	{
		if (eSystemInfo::getInstance()->canShutdown())
			eZap::getInstance()->quit();
	}
	else
	if (command == "reboot")
		eZap::getInstance()->quit(4);
	else
	if (command == "restart")
		eZap::getInstance()->quit(2);
	else
	if (command == "wakeup")
		eZapMain::getInstance()->wakeUp();
	else
	if (command == "standby")
		eZapMain::getInstance()->gotoStandby();

	return "<?xml version=\"1.0\"?><!DOCTYPE wml PUBLIC \"-//WAPFORUM//DTD WML 1.1//EN\" \"http://www.wapforum.org/DTD/wml_1.1.xml\"><wml><card title=\"Info\"><p>Command " + command + " initiated.</p></card></wml>";
}


#ifndef DISABLE_FILE
static eString videocontrol(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString command = opt["command"];
	if (command == "rewind")
	{
		// not working... different solution required
		// eZapMain::getInstance()->startSkip(eZapMain::skipReverse);
	}
	else
	if (command == "forward")
	{
		// not working... different solution required
		// eZapMain::getInstance()->startSkip(eZapMain::skipForward);
	}
	else
	if (command == "stop")
	{
		eZapMain::getInstance()->stop();
	}
	else
	if (command == "pause")
	{
		eZapMain::getInstance()->pause();
	}
	else
	if (command == "play")
	{
		eZapMain::getInstance()->play();
	}

	content->code=204;
	content->code_descr="No Content";
	return NOCONTENT;
}
#endif

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

static eString setAudio(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	int apid = -1;
	sscanf(opt["audio"].c_str(), "0x%04x", &apid);

	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if ( sapi )
	{
		std::list<eDVBServiceController::audioStream> &astreams(sapi->audioStreams);
		std::list<eDVBServiceController::audioStream>::iterator it(astreams.begin());
		for (;it != astreams.end(); ++it)
			if (it->pmtentry->elementary_PID == apid)
			{
				eServiceHandler *service=eServiceInterface::getInstance()->getService();
				if (service)
					service->setPID(it->pmtentry);
				break;
			}
	}

	return "<script language=\"javascript\">window.close();</script>";
}

static eString setScreen(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	smallScreen = 0;
	if (opt["size"] == "0")
	 	smallScreen = 1;

	content->code=204;
	content->code_descr="No Content";
	return NOCONTENT;
}

static eString selectAudio(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString requester = opt["requester"];

	content->local_header["Content-Type"]="text/html; charset=utf-8";

	eString audioChannels;
	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		std::list<eDVBServiceController::audioStream> &astreams( sapi->audioStreams );
		for (std::list<eDVBServiceController::audioStream>::iterator it(astreams.begin())
			;it != astreams.end(); ++it )
		{
			if (requester == "webif")
			{
				if (it->pmtentry->elementary_PID == Decoder::current.apid)
					audioChannels += eString().sprintf("<option selected value=\"0x%04x\">", it->pmtentry->elementary_PID);
				else
					audioChannels += eString().sprintf("<option value=\"0x%04x\">", it->pmtentry->elementary_PID);
			}
			else
				audioChannels += eString().sprintf("<option value=\"0x%04x\">", it->pmtentry->elementary_PID);

			audioChannels += it->text;
			audioChannels += "</option>";
		}
	}
	else
		audioChannels = "<option>no audio data available</option>";

	eString result = readFile(TEMPLATE_DIR + "audioSelection.tmp");
	result.strReplace("#AUDIOCHANS#", audioChannels);

	return result;
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

static eString getCurService()
{
	eString result = "&nbsp;";

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		eService *current=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
		if (current)
			result = current->service_name.c_str();
	}
	return filter_string(result);
}

static eString getChannelNavi(void)
{
	eString result = "&nbsp;";

	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();

	if (sapi && sapi->service)
	{
		result = button(100, "AUDIO", OCKER, "javascript:selectAudio()");
		if (getCurService() != "&nbsp;")
		{
			result += button(100, "EPG", GREEN, "javascript:openEPG()");
			if (smallScreen == 0)
			{
				result += button(100, "Info", PINK, "javascript:openChannelInfo()");
				result += button(100, "Stream Info", YELLOW, "javascript:openSI()");
			}
#ifndef DISABLE_FILE
			result += button(100, "Record", RED, "javascript:DVRrecord('start')");
			result += button(100, "Stop", BLUE, "javascript:DVRrecord('stop')");
#endif
		}
	}

	return result;
}

static eString getZapNavi(eString mode, eString path)
{
	eString result;
	result += button(100, "TV", RED, "?path=" + zap[ZAPMODETV][ZAPMODECATEGORY]);
	result += button(100, "Radio", GREEN, "?path=" + zap[ZAPMODERADIO][ZAPMODECATEGORY]);
	result += button(100, "Data", BLUE, "?path=" + zap[ZAPMODEDATA][ZAPMODECATEGORY]);
#ifndef DISABLE_FILE
	result += button(100, "Recordings", OCKER,  "?path=;4097:7:0:1:0:0:0:0:0:0:");
#endif
	result += "<br><br>";
	return result;
}

static eString getLeftNavi(eString mode, eString path)
{
	eString result;
	if (mode.find("zap") == 0)
	{
		if (smallScreen == 0)
		{
			if (zap[zapMode][ZAPSUBMODESATELLITES])
			{
				result += button(110, "Satellites", LEFTNAVICOLOR, "?path=" + zap[zapMode][ZAPSUBMODESATELLITES]);
				result += "<br>";
			}
			if (zap[zapMode][ZAPSUBMODEPROVIDERS])
			{
				result += button(110, "Providers", LEFTNAVICOLOR, "?path=" + zap[zapMode][ZAPSUBMODEPROVIDERS]);
				result += "<br>";
			}
			if (zap[zapMode][ZAPSUBMODEBOUQUETS])
			{
				result += button(110, "Bouquets", LEFTNAVICOLOR, "?path=" + zap[zapMode][ZAPSUBMODEBOUQUETS]);
				result += "<br>";
			}
		}
		else
		{
			result += button(110, "TV", LEFTNAVICOLOR, "?path=;0:7:1:0:0:0:0:0:0:0:");
			result += "<br>";
			result += button(110, "Radio", LEFTNAVICOLOR, "?path=;0:7:2:0:0:0:0:0:0:0:");
			result += "<br>";
			result += button(110, "Data", LEFTNAVICOLOR, "?path=;0:7:6:0:0:0:0:0:0:0:");
			result += "<br>";
			result += button(110, "Root", LEFTNAVICOLOR, "?path=;2:47:0:0:0:0:%2f");
#ifndef DISABLE_FILE
			result += "<br>";
			result += button(110, "Harddisk", LEFTNAVICOLOR, "?path=;2:47:0:0:0:0:%2fhdd%2f");
			result += "<br>";
			result += button(110, "Recordings", LEFTNAVICOLOR, "?path=;4097:7:0:1:0:0:0:0:0:0:");
#endif
		}
	}
	else
	if (mode.find("control") == 0)
	{
		if (eSystemInfo::getInstance()->canShutdown())
		{
			result += button(110, "Shutdown", LEFTNAVICOLOR, "javascript:admin(\'/cgi-bin/admin?command=shutdown\')");
			result += "<br>";
		}
		result += button(110, "Restart", LEFTNAVICOLOR, "javascript:admin(\'/cgi-bin/admin?command=restart\')");
		result += "<br>";
		result += button(110, "Reboot", LEFTNAVICOLOR, "javascript:admin(\'/cgi-bin/admin?command=reboot\')");
		result += "<br>";
		result += button(110, "Standby", LEFTNAVICOLOR, "javascript:admin(\'/cgi-bin/admin?command=standby\')");
		result += "<br>";
		result += button(110, "Wakeup", LEFTNAVICOLOR, "javascript:admin(\'/cgi-bin/admin?command=wakeup\')");
		result += "<br>";
		result += button(110, "OSDshot", LEFTNAVICOLOR, "?mode=controlFBShot");
#ifndef DISABLE_LCD
		result += "<br>";
		result += button(110, "LCDshot", LEFTNAVICOLOR, "?mode=controlLCDShot");
#endif
		if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000
			|| eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7020)
		{
			result += "<br>";
			result += button(110, "Screenshot", LEFTNAVICOLOR, "?mode=controlScreenShot");
		}
		result += "<br>";
		result += button(110, "Message", LEFTNAVICOLOR, "javascript:sendMessage2TV()");
		result += "<br>";
		result += button(110, "Plugins", LEFTNAVICOLOR, "?mode=controlPlugins");
		result += "<br>";
		result += button(110, "Timer", LEFTNAVICOLOR, "?mode=controlTimerList");
	}
	else
#ifndef DISABLE_FILE
	if (mode.find("config") == 0)
	{
		result += button(110, "Mount Manager", LEFTNAVICOLOR, "?mode=configMountMgr");
		result += "<br>";
		result += button(110, "HDD", LEFTNAVICOLOR, "?mode=configHDD");
		result += "<br>";
		result += button(110, "USB", LEFTNAVICOLOR, "?mode=configUSB");
		result += "<br>";
	}
	else
#endif
	if (mode.find("updates") == 0)
	{
		result += button(110, "Internet", LEFTNAVICOLOR, "?mode=updatesInternet");
	}
	else
	if (mode.find("help") == 0)
	{
		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000)
		{
			result += button(110, "DMM Sites", LEFTNAVICOLOR, "?mode=helpDMMSites");
			result += "<br>";
			result += button(110, "Other Sites", LEFTNAVICOLOR, "?mode=helpOtherSites");
			result += "<br>";
		}
		result += button(110, "Boards", LEFTNAVICOLOR, "?mode=helpForums");
		result += "<br>";
	}

	return result;
}

static eString getTopNavi(eString mode, eString path)
{
	eString result;
	result += button(100, "ZAP", TOPNAVICOLOR, "?mode=zap");
	result += button(100, "CONTROL", TOPNAVICOLOR, "?mode=control");
	if (smallScreen == 0)
	{
		result += button(100, "CONFIG", TOPNAVICOLOR, "?mode=config");
		result += button(100, "UPDATES", TOPNAVICOLOR, "?mode=updates");
	}
	result += button(100, "HELP", TOPNAVICOLOR, "?mode=help");

	return result;
}

static eString version(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/plain";
	eString result;
	result = "enigma";
//	result.sprintf("EliteDVB Version : %s\r\n, eZap Version : doof\r\n",eDVB::getInstance()->getVersion().c_str());
	return result;
}

#ifndef DISABLE_FILE
static eString setFakeRecordingState(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	int state = (opt == "on") ? 1 : 0;
	eZapMain::getInstance()->setFakeRecordingState(state);
	return "+ok";
}
#endif

static eString channels_getcurrent(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/plain; charset=utf-8";

	if (eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI())
		if (eServiceDVB *current=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service))
			return current->service_name.c_str();

	return "-1";
}

static eString getVolume()
{
	return eString().setNum((63 - eAVSwitch::getInstance()->getVolume()) * 100 / 63, 10);
}

static eString setVolume(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	eString mute = opt["mute"];
	eString volume = opt["volume"];

	if (mute)
		eAVSwitch::getInstance()->toggleMute();

	if (volume)
	{
		int vol = atoi(volume.c_str());
		if (vol > 10) vol = 10;
		if (vol < 1) vol = 1;

		float temp = (float)vol;
		temp = temp * 6.3;
		vol = (int)temp;

		eAVSwitch::getInstance()->changeVolume(1, 63 - vol);
	}

	content->code=204;
	content->code_descr="No Content";
	return NOCONTENT;
}

static eString setVideo(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	eString video = opt["position"];

	if (video)
	{
		int vid = atoi(video.c_str());
		if (vid > 10) vid = 10;
		if (vid < 1) vid = 1;

		eServiceHandler *handler=eServiceInterface::getInstance()->getService();
		if (handler)
		{
			int total = handler->getPosition(eServiceHandler::posQueryLength);
			int current = handler->getPosition(eServiceHandler::posQueryCurrent);
			int skipTime = (total / 10 * vid) - current;
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSeekBegin));
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSkip, skipTime * 1000));
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSeekEnd));
		}
	}

	content->code=204;
	content->code_descr="No Content";
	return NOCONTENT;
}

eString ref2string(const eServiceReference &r)
{
	return httpEscape(r.toString());
}

eServiceReference string2ref(const eString &service)
{
	eString str = httpUnescape(service);
	return eServiceReference(str);
}

static eString getIP()
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

#ifndef DISABLE_FILE
extern int freeRecordSpace(void);  // implemented in enigma_main.cpp

static eString getDiskSpace(void)
{
	eString result = "unknown";

	result = "Remaining Disk: ";
	int fds = freeRecordSpace();
	if (fds != -1)
	{
		if (fds < 1024)
			result += eString().sprintf("%d MB", fds);
		else
			result += eString().sprintf("%d.%02d GB", fds/1024, (int)((fds % 1024) / 10.34));
		result += "/";
		int min = fds / 33;
		if (min < 60)
			result += eString().sprintf("~%d min", min);
		else
			result += eString().sprintf("~%d h %02d min", min/60, min%60);
	}

	return result;
}
#endif

static eString getStats()
{
	eString result;
	eString apid, vpid;

#ifndef DISABLE_FILE
	result += getDiskSpace();
	result += "&nbsp;<img src=\"squ.png\">&nbsp;";
#endif
	int sec = atoi(readFile("/proc/uptime").c_str());
	result += eString().sprintf("%d:%02d h up", sec/3600, (sec%3600)/60);
	result += "&nbsp;<img src=\"squ.png\">&nbsp;";

	result += getIP();
	result += "&nbsp;<img src=\"squ.png\">&nbsp;";

	int lockWebIf = 1;
	eConfig::getInstance()->getKey("/ezap/webif/lockWebIf", lockWebIf);
	result += (lockWebIf == 1) ? "locked" : "unlocked";
	result += "&nbsp;<img src=\"squ.png\">&nbsp;";

	vpid = (Decoder::current.vpid == -1) ? "none" : vpid.sprintf("0x%x", Decoder::current.vpid);
	result += "vpid: " + vpid;
	result += "&nbsp;<img src=\"squ.png\">&nbsp;";

	apid = (Decoder::current.apid == -1) ? "none" : apid.sprintf("0x%x", Decoder::current.apid);
	result += "<a href=\"/audio.m3u\">apid: " + apid + "</a>";

	return result;
}

static eString getChannelStats()
{
	std::stringstream result;

	if (eZapMain::getInstance()->getAC3Logo())
		result << "<img src=\"dolby_on.png\" border=0>";
	else
		result << "<img src=\"dolby_off.png\" border=0>";

	result << "&nbsp;";

	if (eZapMain::getInstance()->getSmartcardLogo())
		result << "<img src=\"crypt_on.png\" border=0>";
	else
		result << "<img src=\"crypt_off.png\" border=0>";

	result << "&nbsp;";

	if (eZapMain::getInstance()->get16_9Logo())
		result << "<img src=\"format_on.png\" border=0>";
	else
		result << "<img src=\"format_off.png\" border=0>";

	return result.str();
}

static eString getRecordingStat()
{
	std::stringstream result;
#ifndef DISABLE_FILE
	if (eZapMain::getInstance()->isRecording())
		result << "<img src=\"blinking_red.gif\" border=0>";
	else
#endif
		result << "&nbsp;";

	return result.str();
}

static eString getEITC(eString result)
{
	eString now_time = "&nbsp;", now_duration = "&nbsp;", now_text = "&nbsp;", now_longtext = "&nbsp;";
	eString next_time = "&nbsp;", next_duration = "&nbsp;", next_text = "&nbsp;", next_longtext = "&nbsp;";

	EIT *eit = eDVB::getInstance()->getEIT();

	if (eit)
	{
		int p = 0;

		for (ePtrList<EITEvent>::iterator event(eit->events); event != eit->events.end(); ++event)
		{
			if (*event)
			{
				if (p == 0)
				{
					if (event->start_time != 0)
					{
						now_time.sprintf("%s", ctime(&event->start_time));
						now_time=now_time.mid(10, 6);
					}

					now_duration.sprintf("&nbsp;(%d&nbsp;min)&nbsp;", (int)(event->duration/60));
				}
				if (p == 1)
				{
					if (event->start_time != 0)
					{
 						next_time.sprintf("%s", ctime(&event->start_time));
						next_time=next_time.mid(10, 6);
						next_duration.sprintf("&nbsp;(%d&nbsp;min)&nbsp;", (int)(event->duration/60));
					}
				}
				for (ePtrList<Descriptor>::iterator descriptor(event->descriptor); descriptor != event->descriptor.end(); ++descriptor)
				{
					if (descriptor->Tag() == DESCR_SHORT_EVENT)
					{
						ShortEventDescriptor *ss =(ShortEventDescriptor*)*descriptor;
						switch(p)
						{
							case 0:
								now_text=ss->event_name;
								break;
							case 1:
								next_text=ss->event_name;
								break;
						}
					}
					if (descriptor->Tag() == DESCR_EXTENDED_EVENT)
					{
						ExtendedEventDescriptor *ss =(ExtendedEventDescriptor*)*descriptor;
						switch(p)
						{
							case 0:
								now_longtext+=ss->text;
								break;
							case 1:
								next_longtext+=ss->text;
								break;
						}
					}
				}
				p++;
		 	}
		}

		if (now_time != "&nbsp;")
		{
			now_longtext = filter_string(now_longtext);
			if (now_longtext.find("&nbsp;") == 0)
				now_longtext = now_longtext.right(now_longtext.length() - 6);

			next_longtext = filter_string(next_longtext);
			if (next_longtext.find("&nbsp;") == 0)
				next_longtext = next_longtext.right(next_longtext.length() - 6);

			result.strReplace("#NOWT#", now_time);
			result.strReplace("#NOWD#", now_duration);
			result.strReplace("#NOWST#", now_text);
			result.strReplace("#NOWLT#", now_longtext);
			result.strReplace("#NEXTT#", next_time);
			result.strReplace("#NEXTD#", next_duration);
			result.strReplace("#NEXTST#", next_text);
			result.strReplace("#NEXTLT#", next_longtext);
		}
		eit->unlock();
	}

	return result;
}

static eString getVolBar()
{
	std::stringstream result;
	int volume = atoi(getVolume().c_str());

	for (int i = 1; i <= 10; i++)
	{
		result << "<td width=\"15\" height=\"8\">"
			"<a href=\"javascript:setVol(" << i << ")\">";
		if (i <= volume / 10)
			result << "<img src=\"led_on.gif\" border=\"0\" width=\"15\" height=\"8\">";
		else
			result << "<img src=\"led_off.gif\" border=\"0\" width=\"15\" height=\"8\">";
		result << "</a>"
			"</td>";
	}

	return result.str();
}

static eString getMute()
{
	std::stringstream result;

	result << "<a href=\"javascript:Mute(" << eAVSwitch::getInstance()->getMute() << ")\">";
	if (eAVSwitch::getInstance()->getMute())
		result << "<img src=\"speak_off.gif\" border=0>";
	else
		result << "<img src=\"speak_on.gif\" border=0>";
	result << "</a>";

	return result.str();
}

static eString getVideoBar()
{
	std::stringstream result;
	int videopos = 0;
	int min = 0, sec = 0;
	int total = 0, current = 0;

	if (eServiceHandler *handler = eServiceInterface::getInstance()->getService())
	{
		total = handler->getPosition(eServiceHandler::posQueryLength);
		current = handler->getPosition(eServiceHandler::posQueryCurrent);
	}

	if ((total > 0) && (current != -1))
	{
		min = total - current;
		sec = min % 60;
		min /= 60;
		videopos = (current * 10) / total;
	}


	for (int i = 1; i <= 10; i++)
	{
		result << "<td width=\"15\" height=\"8\">"
			"<a href=\"javascript:setVol(" << i << ")\">";
		if (i <= videopos)
			result << "<img src=\"led_on.gif\" border=\"0\" width=\"15\" height=\"8\">";
		else
			result << "<img src=\"led_off.gif\" border=\"0\" width=\"15\" height=\"8\">";
		result << "</a>"
			"</td>";
	}

	result << "<td>&nbsp;&nbsp;-" << min << ":" << sec << "</td>";

	return result.str();
}

static eString getUpdates()
{
	std::stringstream result;

	eString myVersion = getAttribute("/.version", "version");
	eString myCatalogURL = getAttribute("/.version", "catalog");
	eString myComment = getAttribute("/.version", "comment");
	eString myImageURL = getAttribute("/.version", "url");

	result << "<h2>Installed Image Information</h2>";
	result << "<table width=100% border=1 cellpadding=0 cellspacing=0>";
	result << "<tr><td>Version</td><td>" << firmwareLevel(myVersion) << "</td></tr>";
	result << "<tr><td>URL</td><td>" << myImageURL << "</td></tr>";
	result << "<tr><td>Comment</td><td>" << myComment << "</td></tr>";
	result << "<tr><td>Catalog</td><td>" << myCatalogURL << "</td></tr>";
	result << "</table>";
	result << "<br>";
	result << "For information about available updates select one of the categories on the left.";

	return result.str();
}

static eString getUpdatesInternet()
{
	std::stringstream result;
	eString imageName = "&nbsp;", imageVersion = "&nbsp;", imageURL = "&nbsp;", imageCreator = "&nbsp;", imageMD5 = "&nbsp;";
	eString myCatalogURL = getAttribute("/.version", "catalog");

	result << "<h2>Available Images</h2>";

	if (myCatalogURL.length())
	{
		system(eString("wget -q -O /tmp/catalog.xml " + myCatalogURL).c_str());
		ifstream catalogFile("/tmp/catalog.xml");
		if (catalogFile)
		{
			result << "<table width=100% border=1 cellpadding=0 cellspacing=0>";
			eString line;
			while (getline(catalogFile, line, '\n'))
			{
				if (line.find("<image") != eString::npos)
				{
					if (imageVersion != "&nbsp;")
					{
						result  << "<tr>"
							<< "<td>" << imageVersion << "</td>"
							<< "<td>" << imageName << "</td>"
							<< "</tr>";
					}
					imageName = "&nbsp;";
					imageVersion = "&nbsp;";
					imageURL = "&nbsp;";
					imageCreator = "&nbsp;";
					imageMD5 = "&nbsp;";
				}
				else
				if (line.find("version=") != eString::npos)
				{
					imageVersion = getRight(line, '"');
					imageVersion = getLeft(imageVersion, '"');
				}
				else
				if (line.find("name=") != eString::npos)
				{
					imageName = getRight(line, '"');
					imageName = getLeft(imageName, '"');
				}
			}
			result << "</table>";
		}
		else
			result << "No image information available.";
	}
	else
		result << "No image information available.";

	return result.str();
}

#ifndef DISABLE_FILE
static eString deleteMovie(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString sref;

	sref = opt["ref"];
	eServiceReference ref = string2ref(sref);
	ePlaylist *recordings = eZapMain::getInstance()->getRecordings();
	if (::unlink(ref.path.c_str()) < 0)
	{
		eDebug("remove File %s failed (%m)", ref.path.c_str());
	}
	else
	{
		if (ref.path.right(3).upper() == ".TS")
		{
			for (std::list<ePlaylistEntry>::iterator it(recordings->getList().begin());
				it != recordings->getList().end(); ++it)
			{
				if (it->service.path == ref.path)
				{
					recordings->getList().erase(it);
					recordings->save();
					break;
				}
			}
			int ret = 0;
			int cnt = 1;
			do
			{
				ret = ::unlink(eString().sprintf("%s.%03d", ref.path.c_str(), cnt++).c_str());
			}
			while(!ret);

			::unlink(eString().sprintf("%s.eit", getLeft(ref.path, '.').c_str()).c_str());
		}
	}
	content->code=204;
	content->code_descr="No Content";
	return NOCONTENT;
}
#endif

struct countDVBServices: public Object
{
	int &count;
	countDVBServices(const eServiceReference &bouquetRef, int &count)
		:count(count)
	{
		Signal1<void, const eServiceReference&> cbSignal;
		CONNECT(cbSignal, countDVBServices::countFunction);
		eServiceInterface::getInstance()->enterDirectory(bouquetRef, cbSignal);
		eServiceInterface::getInstance()->leaveDirectory(bouquetRef);
	}
	void countFunction(const eServiceReference& ref)
	{
		if (ref.path
			|| ref.flags & eServiceReference::isDirectory
			|| ref.type != eServiceReference::idDVB)
			return;
		++count;
	}
};

class eWapNavigatorListDirectory: public Object
{
	eString &result;
	eString origpath;
	eString path;
	eServiceInterface &iface;
public:
	eWapNavigatorListDirectory(eString &result, eString origpath, eString path, eServiceInterface &iface): result(result), origpath(origpath), path(path), iface(iface)
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
			result += "<a href=\"/wap?mode=zapto,path=" + serviceRef + "\">";
		else
			result += "<a href=\"/wap?mode=zap,path=" + serviceRef + "\">";

		eService *service = iface.addRef(e);
		if (!service)
			result += "N/A";
		else
		{
			result += filter_string(service->service_name);
			iface.removeRef(e);
		}

		result += "</a>";
		result += "<br/>\n";
	}
};

class eWebNavigatorListDirectory: public Object
{
	eString &result;
	eString origpath;
	eString path;
	eServiceInterface &iface;
	int num;
public:
	eWebNavigatorListDirectory(eString &result, eString origpath, eString path, eServiceInterface &iface): result(result), origpath(origpath), path(path), iface(iface)
	{
		eDebug("path: %s", path.c_str());
		num = 0;
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
		result += "<tr bgcolor=\"";
		result += (num & 1) ? LIGHTGREY : DARKGREY;
		result += "\"><td width=50 align=center>";

		eString serviceRef = ref2string(e);
		if (!(e.flags & eServiceReference::isDirectory))
		{
			if (!e.path)
				result += button(50, "EPG", GREEN, "javascript:openEPG('" + serviceRef + "')");
			else
			if (serviceRef.find("%2fhdd%2fmovie%2f") != eString::npos)
			{
				result += "<a href=\"javascript:deleteMovie('";
				result += serviceRef;
				result += "')\"><img src=\"trash.gif\" height=22 border=0></a>";
			}
			else
				result += "&#160;";
			result += "</td><td><a href=\'javascript:switchChannel(\"" + serviceRef + "\")\'>";
		}
		else
		{
			int count = 0;
			countDVBServices bla(e, count);
			if (count)
				result += button(50, "EPG", GREEN, "javascript:openMultiEPG('" + serviceRef + "')");
			else
				result += "&#160;";
			result += eString("</td><td><a href=\"/")+ "?path=" + serviceRef + "\">";
		}

		eService *service = iface.addRef(e);
		if (!service)
			result += "N/A";
		else
		{
			result += filter_string(service->service_name);
			iface.removeRef(e);
		}

		result += "</a>";
		result += "</td></tr>\n";
		num++;
	}
};

class eWebNavigatorListDirectory2: public Object
{
	eString &result1;
	eString &result2;
	eString origpath;
	eString path;
	eServiceInterface &iface;
public:
	eWebNavigatorListDirectory2(eString &result1, eString &result2, eString origpath, eString path, eServiceInterface &iface): result1(result1), result2(result2), origpath(origpath), path(path), iface(iface)
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
		eString short_description, event_start, event_duration;
		eService *service = iface.addRef(e);
		if (service)
		{
			eEPGCache::getInstance()->Lock();
			const timeMap* evt = eEPGCache::getInstance()->getTimeMap((eServiceReferenceDVB&)e);
			if (evt)
			{
				timeMap::const_iterator It;
				for (It = evt->begin(); (It != evt->end() && short_description == ""); It++)
				{
					EITEvent event(*It->second);
					time_t now = time(0) + eDVB::getInstance()->time_difference;
					//tm start = *localtime(&now);
					if ((now >= event.start_time) && (now <= event.start_time + event.duration))
					{
						for (ePtrList<Descriptor>::iterator d(event.descriptor); d != event.descriptor.end(); ++d)
						{
							Descriptor *descriptor=*d;
							if (descriptor->Tag() == DESCR_SHORT_EVENT)
							{
								short_description = ((ShortEventDescriptor*)descriptor)->event_name;
								tm t = *localtime(&event.start_time);
								event_start = eString().sprintf("%02d:%02d", t.tm_hour, t.tm_min);
								event_duration = eString().sprintf("%d", event.duration / 60);
								break; /* we have everything we wanted */
							}
						}
					}
				}
			}
			eEPGCache::getInstance()->Unlock();

			result1 += "\"" + ref2string(e) + "\", ";
			eString tmp = filter_string(service->service_name);
			if (short_description)
				tmp = tmp + " - " + event_start + " (" + event_duration + ") " + filter_string(short_description);
			tmp.strReplace("\"", "'");
			result2 += "\"" + tmp + "\", ";
			iface.removeRef(e);
		}
	}
};

static eString getZapContent(eString mode, eString path)
{
	eString result;
	eString tpath;

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

		eServiceReference current_service=string2ref(tpath);
		eServiceInterface *iface = eServiceInterface::getInstance();

		if (!(current_service.flags&eServiceReference::isDirectory))	// is playable
		{
			eZapMain::getInstance()->playService(current_service, eZapMain::psSetMode|eZapMain::psDontAdd);
			result += "<script language=\"javascript\">window.close();</script>";
		}
		else
		{
			eWebNavigatorListDirectory navlist(result, path, tpath, *iface);
			Signal1<void, const eServiceReference&> signal;
			signal.connect(slot(navlist, &eWebNavigatorListDirectory::addEntry));
			result += "<table width=\"100%\" cellspacing=\"2\" cellpadding=\"1\" border=\"0\">\n";
			iface->enterDirectory(current_service, signal);
			result += "</table>\n";
			eDebug("entered");
			iface->leaveDirectory(current_service);
			eDebug("exited");
		}
	}

	return result;
}

static eString getZapContent2(eString mode, eString path)
{
	eString result, result1, result2;
	eString tpath;
	eString bouquets, bouquetrefs, channels, channelrefs;
	std::stringstream tmp;

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
		eWebNavigatorListDirectory2 navlist(result1, result2, path, tpath, *iface);
		Signal1<void, const eServiceReference&> signal;
		signal.connect(slot(navlist, &eWebNavigatorListDirectory2::addEntry));
		iface->enterDirectory(current_service, signal);
		tmp.str(result1.left(result1.length() - 1));
		bouquetrefs = result1.left(result1.length() - 2);
		bouquets = result2.left(result2.length() - 2);
		eDebug("entered");
		iface->leaveDirectory(current_service);
		eDebug("exited");

		// go thru all bouquets to get the channels
		int i = 0;
		while (tmp)
		{
			result1 = ""; result2 =""; tpath = "";
			tmp >> tpath;
			if (tpath)
			{
				tpath = tpath.mid(1, tpath.length() - 3);
				eServiceReference current_service = string2ref(tpath);

				eWebNavigatorListDirectory2 navlist(result1, result2, tpath, tpath, *iface);
				Signal1<void, const eServiceReference&> signal;
				signal.connect(slot(navlist, &eWebNavigatorListDirectory2::addEntry));

				channels += "channels[";
				channels += eString().sprintf("%d", i);
				channels += "] = new Array(";
				channelrefs += "channelRefs[";
				channelrefs += eString().sprintf("%d", i);
				channelrefs += "] = new Array(";

				iface->enterDirectory(current_service, signal);

				channels += result2.left(result2.length() - 2);
				channels += ");";
				channelrefs += result1.left(result1.length() - 2);
				channelrefs += ");";

				eDebug("entered");
				iface->leaveDirectory(current_service);
				eDebug("exited");
				i++;
			}
		}

		result = readFile(HTDOCS_DIR + "zapdata.js");
		result.strReplace("#BOUQUETS#", bouquets);
		result.strReplace("#BOUQUETREFS#", bouquetrefs);
		result.strReplace("#CHANNELS#", channels);
		result.strReplace("#CHANNELREFS#", channelrefs);
		result.strReplace("#CURRENTBOUQUET#", eString().sprintf("%d", currentBouquet));
		result.strReplace("#CURRENTCHANNEL#", eString().sprintf("%d", currentChannel));
		int autobouquetchange = 0;
		eConfig::getInstance()->getKey("/elitedvb/extra/autobouquetchange", autobouquetchange);
		result.strReplace("#AUTOBOUQUETCHANGE#", eString().sprintf("%d", autobouquetchange));
	}

	return result;
}

static eString getWapZapContent(eString path)
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
		eWapNavigatorListDirectory navlist(result, path, tpath, *iface);
		Signal1<void, const eServiceReference&> signal;
		signal.connect(slot(navlist, &eWapNavigatorListDirectory::addEntry));
		iface->enterDirectory(current_service, signal);
		eDebug("entered");
		iface->leaveDirectory(current_service);
		eDebug("exited");
	}

	return result;
}

static eString getZapContent3(eString mode, eString path)
{
	eString result, result1, result2;
	eString bouquets, bouquetrefs, channels, channelrefs;
	eString tpath;

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

		eWebNavigatorListDirectory2 navlist(result1, result2, path, tpath, *iface);
		Signal1<void, const eServiceReference&> signal;
		signal.connect(slot(navlist, &eWebNavigatorListDirectory2::addEntry));

		channels += "channels[0] = new Array(";
		channelrefs += "channelRefs[0] = new Array(";

		iface->enterDirectory(current_service, signal);
		eDebug("entered");

		channels += result2.left(result2.length() - 2);
		channels += ");";
		channelrefs += result1.left(result1.length() - 2);
		channelrefs += ");";

		iface->leaveDirectory(current_service);
		eDebug("exited");
	}

	bouquets = "\"dummy bouquet\"";
	bouquetrefs = "\"dummy bouquet ref\"";

	eString tmpFile = readFile(HTDOCS_DIR + "zapdata.js");
	tmpFile.strReplace("#BOUQUETS#", bouquets);
	tmpFile.strReplace("#BOUQUETREFS#", bouquetrefs);
	tmpFile.strReplace("#CHANNELS#", channels);
	tmpFile.strReplace("#CHANNELREFS#", channelrefs);
	tmpFile.strReplace("#CURRENTBOUQUET#", eString().sprintf("%d", currentBouquet));
	tmpFile.strReplace("#CURRENTCHANNEL#", eString().sprintf("%d", currentChannel));
	tmpFile.strReplace("#AUTOBOUQUETCHANGE#", "0");

	result = readFile(TEMPLATE_DIR + "rec.tmp");
	result.strReplace("#ZAPDATA#", tmpFile);
	return result;
}

static eString getZap(eString mode, eString path)
{
	eString result;

	result += getZapNavi(mode, path);
#ifndef DISABLE_FILE
	if (path == ";4097:7:0:1:0:0:0:0:0:0:") // recordings
	{
		eString tmpFile = readFile(TEMPLATE_DIR + "videocontrols.tmp");
		tmpFile.strReplace("#VIDEOBAR#", getVideoBar());
		result += tmpFile;
		result += getZapContent3(mode, path);
	}
	else
#endif
	{
		if (smallScreen == 0)
		{
			eString tmp = readFile(TEMPLATE_DIR + "zap.tmp");
			tmp.strReplace("#ZAPDATA#", getZapContent2(mode, path));
			result += tmp;
		}
		else
		{
			result += getEITC(readFile(TEMPLATE_DIR + "eit_small.tmp"));
			result.strReplace("#SERVICENAME#", filter_string(getCurService()));
			result += getZapContent(mode, path);
		}
	}

	return result;
}

#ifndef DISABLE_FILE
static eString getDiskInfo(void)
{
	std::stringstream result;
	eString sharddisks = "none";
	if (eSystemInfo::getInstance()->hasHDD())
	{
		for (int c = 'a'; c < 'h'; c++)
		{
			char line[1024];
			int ok = 1;
			FILE *f = fopen(eString().sprintf("/proc/ide/hd%c/media", c).c_str(), "r");
			if (!f)
				continue;
			if ((!fgets(line, 1024, f)) || strcmp(line, "disk\n"))
				ok = 0;
			fclose(f);
			if (ok)
			{
				FILE *f = fopen(eString().sprintf("/proc/ide/hd%c/model", c).c_str(), "r");
				if (!f)
					continue;
				*line = 0;
				fgets(line, 1024, f);
				fclose(f);
				if (!*line)
					continue;
				line[strlen(line) - 1] = 0;
				sharddisks = line;
				f = fopen(eString().sprintf("/proc/ide/hd%c/capacity", c).c_str(), "r");
				if (!f)
					continue;
				int capacity = 0;
				fscanf(f, "%d", &capacity);
				fclose(f);
				sharddisks += " (";
				if (c & 1)
					sharddisks += "master";
				else
					sharddisks += "slave";
				if (capacity)
					sharddisks += eString().sprintf(", %d MB", capacity / 2048);
				sharddisks += ")";
			}
		}
		result  << "<tr>"
			"<td>Harddisk:</td>"
			"<td>" << sharddisks << "</td>"
			"</tr>";
	}
	return result.str();
}

static eString getConfigHDD(void)
{
	std::stringstream result;
	result  << "<table border=0 cellspacing=0 cellpadding=0>"
		<< getDiskInfo()
		<< "</table>"
		<< "<br>"
		<< readFile(TEMPLATE_DIR + "configHDD.tmp");
	return result.str();
}

static eString getUSBInfo(void)
{
	std::stringstream result;
	eString usbStick = "none";
	system("cat /proc/scsi/usb-storage-0/0 > /tmp/usbstick.tmp"); // 2.4.20
	system("cat /proc/scsi/usb-storage/0 >> /tmp/usbstick.tmp");  // 2.6.5
	eString line;
	ifstream infile("/tmp/usbstick.tmp");
	if (infile)
	{
		usbStick = "";
		while (getline(infile, line, '\n'))
		{
			if (line.find("Vendor:") != eString::npos)
				usbStick += "Vendor =" + getRight(line, ':');
			if (line.find("Product:") != eString::npos)
			{
				usbStick += ", ";
				usbStick += "Product =" + getRight(line, ':');
			}
			if (line.find("Attached:") != eString::npos)
			{
				usbStick += ", ";
				usbStick += "Attached =" + getRight(line, ':');
			}
		}
		result 	<< "<tr>"
			"<td>USB Stick:</td>"
			"<td>" << usbStick << "</td>"
			"</tr>";
		unlink("/tmp/usbstick.tmp");
	}
	return result.str();
}

static eString getConfigUSB(void)
{
	std::stringstream result;
	result  << "<table border=0 cellspacing=0 cellpadding=0>"
		<< getUSBInfo()
		<< "</table>"
		<< "<br>"
		<< readFile(TEMPLATE_DIR + "configUSB.tmp");
	return result.str();
}

static eString setConfigUSB(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString swapUSB = opt["swapusb"];
	eString swapUSBFile = opt["swapusbfile"];
	eString bootUSB = opt["bootUSB"];
	eString bootUSBImage = opt["bootusbimage"];

	content->code=204;
	content->code_descr="No Content";
	return NOCONTENT;
}

static eString setConfigHDD(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString swapHDD = opt["swaphdd"];
	eString swapHDDFile = opt["swaphddfile"];
	eString bootHDD = opt["bootHDD"];
	eString bootHDDImage = opt["boothddimage"];

	content->code=204;
	content->code_descr="No Content";
	return NOCONTENT;
}
#endif

static eString msgWindow(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString title = opt["title"];
	eString msg = opt["msg"];
	return getMsgWindow(title, msg);
}

static eString getLinuxVersion()
{
	system("uname -a > /tmp/linux.tmp");
	return readFile("/tmp/linux.tmp");
}

static eString aboutDreambox(void)
{
	std::stringstream result;
	result << "<table border=0 cellspacing=0 cellpadding=0>";

	if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000)
		result << "<img src=\"dm7000.jpg\" width=\"620\" border=\"0\"><br><br>";

	result  << "<tr><td>Model:</td><td>" << eSystemInfo::getInstance()->getModel() << "</td></tr>"
		<< "<tr><td>Manufacturer:</td><td>" << eSystemInfo::getInstance()->getManufacturer() << "</td></tr>"
		<< "<tr><td>Processor:</td><td>" << eSystemInfo::getInstance()->getCPUInfo() << "</td></tr>";

#ifndef DISABLE_FILE
	result << getDiskInfo();
	result << getUSBInfo();
#endif
	result	<< "<tr><td>Linux:</td><td>" << getLinuxVersion() << "</td></tr>"
		<< "<tr><td>Firmware:</td><td>" << firmwareLevel(getAttribute("/.version", "version")) << "</td></tr>";
		if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000)
			result << "<tr><td>FP Firmware:</td><td>" << eString().sprintf(" 1.%02d", eDreamboxFP::getFPVersion()) << "</td></tr>";
		result << "<tr><td>Web Interface:</td><td>" << WEBXFACEVERSION << "</td></tr>"
		<< "</table>";

	return result.str();
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

struct getEntryString
{
	std::stringstream &result;
	bool repeating;

	getEntryString(std::stringstream &result, bool repeating)
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
			channel = _("No channel available");

		description = getRight(description, '/');
		if (!description)
			description = _("No description available");

		result << "<tr>"
			<< "<td align=center><a href=\"javascript:deleteTimerEvent(\'"
			<< "ref=" << ref2string(se->service)
			<< "&start=" << se->time_begin
			<< "&type=" << se->type
			<< "&force=no"
			<< "\')\"><img src=\"trash.gif\" border=0 height=20></a></td>";
		if (!repeating)
		{
			result << "<td align=center><a href=\"javascript:editTimerEvent(\'"
				<< "ref=" << ref2string(se->service)
				<< "&start=" << se->time_begin
				<< "&duration=" << se->duration
				<< "&ID=" << std::hex << se->event_id << std::dec
				<< "&channel=" << channel
				<< "&descr=" << description
				<< "&type=" << se->type
				<< "\')\"><img src=\"edit.gif\" border=0 height=20></a></td>";
		}
		else
			result << "<td>&nbsp;</td>";

		if (se->type & ePlaylistEntry::stateFinished)
			result << "<td align=center><img src=\"on.gif\"></td>";
		else if (se->type & ePlaylistEntry::stateError)
			result << "<td align=center><img src=\"off.gif\"></td>";
		else
			result << "<td>&nbsp;</td>";

		result << "<td>";
		if (se->type & ePlaylistEntry::isRepeating)
		{
			if (se->type & ePlaylistEntry::Su)
				result << "Su ";
			if (se->type & ePlaylistEntry::Mo)
				result << "Mo ";
			if (se->type & ePlaylistEntry::Tue)
				result << "Tue ";
			if (se->type & ePlaylistEntry::Wed)
				result << "Wed ";
			if (se->type & ePlaylistEntry::Thu)
				result << "Thu ";
			if (se->type & ePlaylistEntry::Fr)
				result << "Fr ";
			if (se->type & ePlaylistEntry::Sa)
				result << "Sa";
			result  << "</td><td>"
				<< std::setw(2) << startTime.tm_hour << ':'
				<< std::setw(2) << startTime.tm_min << " - ";
		}
		else
		{
			result 	<< std::setw(2) << startTime.tm_mday << '.'
				<< std::setw(2) << startTime.tm_mon+1 << ". - "
				<< std::setw(2) << startTime.tm_hour << ':'
				<< std::setw(2) << startTime.tm_min
				<< "</td><td>"
				<< std::setw(2) << endTime.tm_mday << '.'
				<< std::setw(2) << endTime.tm_mon+1 << ". - ";
		}

		result 	<< std::setw(2) << endTime.tm_hour << ':'
			<< std::setw(2) << endTime.tm_min
			<< "</td><td>" << channel
			<< "</td><td>" << description
			<< "</td></tr>";
	}
};

static eString genTimerListTableBody(int type)
{
	std::stringstream result;
	result << std::setfill('0');
	if (!eTimerManager::getInstance()->getTimerCount())
		result << "<tr><td>" << eString(_("No timer events available")) << "</td></tr>";
	else
		eTimerManager::getInstance()->forEachEntry(getEntryString(result, type));

	return result.str();
}

static eString getControlTimerList()
{
	eString tableBody;
	eString result = readFile(TEMPLATE_DIR + "timerListBody.tmp");

	// regular timers
	int count = 0;
	eTimerManager::getInstance()->forEachEntry(countTimer(count, false));
	if (count)
		tableBody = genTimerListTableBody(0);
	else
		tableBody = "<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>" + eString(_("No regular timer events available")) + "</td></tr>";
	result.strReplace("#TIMER_REGULAR#", tableBody);

	// repeating timers
	count = 0;
	eTimerManager::getInstance()->forEachEntry(countTimer(count, true));
	if (count)
		tableBody = genTimerListTableBody(1);
	else
		tableBody = "<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>" + eString(_("No repeating timer events available")) + "</td></tr>";
	result.strReplace("#TIMER_REPEATED#", tableBody);

	// buttons
	result.strReplace("#BUTTONCLEANUP#", button(100, "Cleanup", BLUE, "javascript:cleanupTimerList()"));
	result.strReplace("#BUTTONCLEAR#", button(100, "Clear", RED, "javascript:clearTimerList()"));
	result.strReplace("#BUTTONADD#", button(100, "Add", GREEN, "javascript:showAddTimerEventWindow()"));

	return result;
}

struct getWapEntryString
{
	std::stringstream &result;
	bool repeating;

	getWapEntryString(std::stringstream &result, bool repeating)
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
			channel = _("No channel available");

		description = getRight(description, '/');
		if (!description)
			description = _("No description available");

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

static eString wapTimerList(void)
{
	std::stringstream result;
	eString tmp = readFile(TEMPLATE_DIR + "wapTimerList.tmp");

	int count = 0;
	eTimerManager::getInstance()->forEachEntry(countTimer(count, false));
	if (count)
	{
		result << std::setfill('0');
		if (!eTimerManager::getInstance()->getTimerCount())
			result << eString(_("No timer events available"));
		else
			eTimerManager::getInstance()->forEachEntry(getWapEntryString(result, 0));
	}
	else
		result << eString(_("No timer events available"));

	tmp.strReplace("#BODY#", result.str());

	return tmp;
}

static eString showTimerList(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString result;
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	result = readFile(TEMPLATE_DIR + "timerList.tmp");
	result.strReplace("#BODY#", getControlTimerList());
	return result;
}

static eString getControlScreenShot(void)
{
	eString result;

	int ret = system("grabpic bmp > /tmp/screenshot.bmp");
	eDebug("ret is %d", ret);
	if ( ret >> 8 )
		result = "grabpic tool is required but not existing or working";
	else
	{
		FILE *bitstream = 0;
		int xres = 0, yres = 0, yres2 = 0, aspect = 0, winxres = 620, winyres = 0, rh = 0, rv = 0;
		if (smallScreen == 1)
			winxres = 240;
		if (Decoder::current.vpid != -1)
			bitstream=fopen("/proc/bus/bitstream", "rt");
		if (bitstream)
		{
			char buffer[100];
			while (fgets(buffer, 100, bitstream))
			{
				if (!strncmp(buffer, "H_SIZE:  ", 9))
					xres = atoi(buffer+9);
				if (!strncmp(buffer, "V_SIZE:  ", 9))
					yres = atoi(buffer+9);
				if (!strncmp(buffer, "A_RATIO: ", 9))
					aspect = atoi(buffer+9);
			}
			fclose(bitstream);
			switch (aspect)
			{
				case 1:
					// square
					rh = 4; rv = 4; break;
				case 2:
					// 4:3
					rh = 4; rv = 3; break;
				case 3:
					// 16:9
					rh = 16; rv = 9; break;
				case 4:
					// 20:9
					rh = 20; rv = 9; break;
			}
		}
		yres2 = xres * rv / rh;
		winyres = yres2 * winxres / xres;

		eDebug("[SCREENSHOT] xres = %d, yres = %d, rh = %d, rv = %d, winxres = %d, winyres = %d\n", xres, yres2, rh, rv, winxres, winyres);

		result = "<img width=\"" +  eString().sprintf("%d", winxres);
		result += "\" height=\"" + eString().sprintf("%d", winyres);
		result += "\" src=\"/root/tmp/screenshot.bmp\" border=1>";
		result += "<br>";
		result += "Original format: " + eString().sprintf("%d", xres) + "x" + eString().sprintf("%d", yres);
		result += " (" + eString().sprintf("%d", rh) + ":" + eString().sprintf("%d", rv) + ")";
	}

	return result;
}

static eString getContent(eString mode, eString path)
{
	eString result, tmp;

	if (mode == "zap")
	{
		tmp = "ZAP";
		if (smallScreen == 0)
		{
			if (path == ";4097:7:0:1:0:0:0:0:0:0:")
				tmp += ": Recordings";
			else
			{
				if (zapMode >= 0)
					tmp += ": " + zap[zapMode][ZAPMODENAME];
				if (zapSubMode >= 0)
				{
					switch(zapSubMode)
					{
						case ZAPSUBMODESATELLITES:
							tmp += " - Satellites";
							break;
						case ZAPSUBMODEPROVIDERS:
							tmp += " - Providers";
							break;
						case ZAPSUBMODEBOUQUETS:
							tmp += " - Bouquets";
							break;
					}
				}
			}
		}

		result = getTitle(tmp);
		result += getZap(mode, path);
	}
	else
	if (mode == "config")
	{
		result = getTitle("CONFIG");
		result += "Select one of the configuration categories on the left";
	}
	else
#ifndef DISABLE_FILE
	if (mode == "configMountMgr")
	{
		result = getTitle("CONFIG: Mount Manager");
		result += getConfigMountMgr();
	}
	else
	if (mode == "configHDD")
	{
		result = getTitle("CONFIG: HDD");
		result += getConfigHDD();
	}
	else
	if (mode == "configUSB")
	{
		result = getTitle("CONFIG: USB");
		result += getConfigUSB();
	}
	else
#endif
	if (mode == "help")
	{
		result = getTitle("HELP");
		result += aboutDreambox();
	}
	else
	if (mode == "helpDMMSites")
	{
		result = getTitle("HELP: DMM Sites");
		result += readFile(TEMPLATE_DIR + "helpDMMSites.tmp");
	}
	else
	if (mode == "helpOtherSites")
	{
		result = getTitle("HELP: Other Sites");
		result += readFile(TEMPLATE_DIR + "helpOtherSites.tmp");
	}
	else
	if (mode == "helpForums")
	{
		result = getTitle("HELP: Boards");
		result += readFile(TEMPLATE_DIR + "helpForums.tmp");
	}
	else
	if (mode == "control")
	{
		result = getTitle("CONTROL");
		result += "Control your box using the commands on the left";
	}
	else
	if (mode == "controlFBShot")
	{
		result = getTitle("CONTROL: OSDShot");
		if (!getOSDShot("fb"))
			if (smallScreen == 0)
				result += "<img width=\"620\" src=\"/root/tmp/osdshot.png\" border=0>";
			else
				result += "<img width=\"240\" src=\"/root/tmp/osdshot.png\" border=0>";
	}
	else
#ifndef DISABLE_LCD
	if (mode == "controlLCDShot")
	{
		result = getTitle("CONTROL: LCDShot");
		if (!getOSDShot("lcd"))
			if (smallScreen == 0)
				result += "<img width=\"620\" src=\"/root/tmp/osdshot.png\" border=0>";
			else
				result += "<img width=\"240\" src=\"/root/tmp/osdshot.png\" border=0>";
	}
	else
#endif
	if (mode == "controlScreenShot")
	{
		result = getTitle("CONTROL: Screenshot");
		result += getControlScreenShot();
	}
	else
	if (mode == "controlTimerList")
	{
		result = getTitle("CONTROL: Timer");
		result += getControlTimerList();
	}
	else
	if (mode == "controlPlugins")
	{
		result = getTitle("CONTROL: Plugins");
		result += getControlPlugins();
	}
	else
	if (mode == "updates")
	{
		result = getTitle("UPDATES");
		result += getUpdates();
	}
	else
	if (mode == "updatesInternet")
	{
		result = getTitle("UPDATES: Internet");
		result += getUpdatesInternet();
	}
	else
	{
		result = getTitle("GENERAL");
		result += mode + " is not available yet";
	}

	return result;
}

static eString getEITC2(eString result)
{
	eString now_time = "&nbsp;", now_duration = "&nbsp;", now_text = "&nbsp;",
		next_time = "&nbsp;", next_duration = "&nbsp;", next_text = "&nbsp;";

	EIT *eit = eDVB::getInstance()->getEIT();
	if (eit)
	{
		int p = 0;

		for (ePtrList<EITEvent>::iterator event(eit->events); event != eit->events.end(); ++event)
		{
			if (*event)
			{
				if (p == 0)
				{
					if (event->start_time != 0)
					{
						now_time.sprintf("%s", ctime(&event->start_time));
						now_time = now_time.mid(10, 6);
					}

					now_duration.sprintf("%d", (int)(event->duration/60));
				}
				if (p == 1)
				{
					if (event->start_time != 0)
					{
 						next_time.sprintf("%s", ctime(&event->start_time));
						next_time = next_time.mid(10,6);
						next_duration.sprintf("%d", (int)(event->duration/60));
					}
				}
				for (ePtrList<Descriptor>::iterator descriptor(event->descriptor); descriptor != event->descriptor.end(); ++descriptor)
				{
					if (descriptor->Tag() == DESCR_SHORT_EVENT)
					{
						ShortEventDescriptor *ss =(ShortEventDescriptor*)*descriptor;
						switch(p)
						{
							case 0:
								now_text = ss->event_name;
								break;
							case 1:
								next_text = ss->event_name;
								break;
						}
						if (p)  // we have all we need
							break;
					}
				}
				p++;
		 	}
		}
		eit->unlock();
	}

	now_text = now_text.left(30);
	next_text = next_text.left(30);
	result.strReplace("#NOWT#", now_time);
	if (now_duration != "&nbsp;")
		now_duration = "(" + now_duration + ")";
	result.strReplace("#NOWD#", now_duration);
	result.strReplace("#NOWST#", now_text);
	result.strReplace("#NEXTT#", next_time);
	if (next_duration != "&nbsp;")
		next_duration = "(" + next_duration + ")";
	result.strReplace("#NEXTD#", next_duration);
	result.strReplace("#NEXTST#", next_text);
	result.strReplace("#VOLBAR#", getVolBar());
	result.strReplace("#MUTE#", getMute());
	result.strReplace("#SERVICENAME#", getCurService());
	result.strReplace("#STATS#", getStats());
	result.strReplace("#EMPTYCELL#", "&nbsp;");
	result.strReplace("#CHANSTATS#", getChannelStats());
	result.strReplace("#RECORDING#", getRecordingStat());

	return result;
}

static eString audiom3u(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="audio/mpegfile";
	return "http://" + getIP() + ":31338/" + eString().sprintf("%02x\n", Decoder::current.apid);
}


static eString getcurepg(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString result;
	eService* current;

	content->local_header["Content-Type"]="text/html; charset=utf-8";

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "not available";

	eServiceReference ref(opt);

	current=eDVB::getInstance()->settings->getTransponders()->searchService(ref?ref:sapi->service);
	if (!current)
		return eString("epg not ready yet");

	result+=eString("<html>" CHARSETMETA "<head><title>epgview</title><link rel=\"stylesheet\" type=\"text/css\" href=\"/webif.css\"></head><body bgcolor=#000000>");
	result+=eString("<span class=\"title\">");
	result+=filter_string(current->service_name);
	result+=eString("</span>");
	result+=eString("<br>\n");

	eEPGCache::getInstance()->Lock();
	const timeMap* evt=ref ?
		eEPGCache::getInstance()->getTimeMap((eServiceReferenceDVB&)ref)
			:
		eEPGCache::getInstance()->getTimeMap(sapi->service);

	if (!evt)
	{
		eEPGCache::getInstance()->Unlock();
		return eString("epg not ready yet");
	}

	timeMap::const_iterator It;

	for(It=evt->begin(); It!= evt->end(); It++)
	{
		EITEvent event(*It->second);
		for(ePtrList<Descriptor>::iterator d(event.descriptor); d != event.descriptor.end(); ++d)
		{
			Descriptor *descriptor=*d;
			if (descriptor->Tag()==DESCR_SHORT_EVENT)
			{
				tm* t = localtime(&event.start_time);
				result += eString().sprintf("<!-- ID: %04x -->", event.event_id);
				result += eString().sprintf("<span class=\"epg\">%02d.%02d - %02d:%02d ", t->tm_mday, t->tm_mon+1, t->tm_hour, t->tm_min);
				result += ((ShortEventDescriptor*)descriptor)->event_name;
				result += "</span><br>\n";
			}
		}
	}
	result += "</body></html>";
	eEPGCache::getInstance()->Unlock();
	return result;
}

#define CHANNELWIDTH 200

class eMEPG: public Object
{
	int hours;
	int d_min;
	eString multiEPG;
	time_t start;
	time_t end;
	int tableWidth;
public:
	int getTableWidth(void)
	{
		return tableWidth;
	}

	time_t adjust2FifteenMinutes(time_t seconds)
	{
		int minutes = seconds / 60;
		int quarterHours = minutes / 15;
		if (minutes % 15 > 7)
			quarterHours++;
		return quarterHours * 15 * 60;
	}

	void getcurepg(const eServiceReference &ref)
	{
		std::stringstream result;
		result << std::setfill('0');
		eService* current;

		eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
		if (sapi)
		{
			current = eDVB::getInstance()->settings->getTransponders()->searchService(ref);
			if (current)
			{
				eEPGCache::getInstance()->Lock();
				const timeMap* evt = eEPGCache::getInstance()->getTimeMap((eServiceReferenceDVB&)ref);
				if (evt)
				{
					int tablePos = 0;
					time_t tableTime = start;
					result << "<tr>"
						<< "<td id=\"channel\" width=" << eString().sprintf("%d", CHANNELWIDTH) << ">"
						<< "<span class=\"channel\">"
						<< filter_string(current->service_name)
						<< "</span>"
						<< "</td>";
					tablePos += CHANNELWIDTH;

					timeMap::const_iterator It;

					for (It = evt->begin(); It != evt->end(); It++)
					{
						eString ext_description;
						eString short_description;
						EITEvent event(*It->second);
						for (ePtrList<Descriptor>::iterator d(event.descriptor); d != event.descriptor.end(); ++d)
						{
							Descriptor *descriptor=*d;
							if (descriptor->Tag() == DESCR_SHORT_EVENT)
							{
								short_description = ((ShortEventDescriptor*)descriptor)->event_name;
							}
							if (d->Tag() == DESCR_EXTENDED_EVENT)
							{
								ext_description += ((ExtendedEventDescriptor*)descriptor)->text;
							}
						}

						time_t eventStart = adjust2FifteenMinutes(event.start_time);
						time_t eventEnd = eventStart + adjust2FifteenMinutes(event.duration);

						int eventDuration = 0;
						int colUnits = 0;
						if ((eventStart > end) || (eventEnd < tableTime))
						{
							eventDuration = 0;
						}
						else
						if ((eventStart < tableTime) && (eventEnd > tableTime))
						{
							eventDuration = eventEnd - tableTime;
						}
						else
						if (eventStart == tableTime)
						{
							eventDuration = adjust2FifteenMinutes(event.duration);
						}
						else
						if ((eventStart > tableTime) && (eventStart < end))
						{
							eventDuration = eventStart - tableTime;
							colUnits = eventDuration / 60 / 15;
							result << "<td colspan=" << colUnits << ">&nbsp;</td>";
							tableTime = eventStart;
							tablePos += colUnits * 15 * d_min;
							eventDuration = adjust2FifteenMinutes(event.duration);
						}

						if ((eventDuration > 0) && (eventDuration < 15 * 60))
							eventDuration = 15 * 60;

						if (tableTime + eventDuration > end)
							eventDuration = end - tableTime;

						colUnits = eventDuration / 60 / 15;
						if (colUnits > 0)
						{
							result << "<td colspan=" << colUnits << ">";
#ifndef DISABLE_FILE
							result << "<a href=\"javascript:record('"
								<< "ref=" << ref2string(ref)
								<< "&ID=" << std::hex << event.event_id << std::dec
								<< "&start=" << event.start_time
								<< "&duration=" << event.duration
								<< "&descr=" << short_description
								<< "&channel=" << filter_string(current->service_name)
								<< "')\"><img src=\"timer.gif\" border=0></a>"
								<< "&nbsp;&nbsp;";
#endif
							tm* t = localtime(&event.start_time);
							result << std::setfill('0')
								<< "<span class=\"time\">"
								<< std::setw(2) << t->tm_mday << '.'
								<< std::setw(2) << t->tm_mon+1 << ". - "
								<< std::setw(2) << t->tm_hour << ':'
								<< std::setw(2) << t->tm_min << ' '
								<< "</span>"
								<< "<span class=\"duration\">"
								<< " (" << event.duration / 60 << " min)"
								<< "</span>"
								<< "<br><b>"
								<< "<a href=\'javascript:switchChannel(\"" << ref2string(ref) << "\")\'>"
								<< "<span class=\"event\">"
								<< short_description
								<< "</span>"
								<< "</a>"

								<< "</b><br>";

							if (eventDuration >= 15 * 60)
							{
								result << "<span class=\"description\">"
									<< filter_string(ext_description)
									<< "</span>";
							}

							result << "</td>\n";
							tablePos += colUnits * 15 * d_min;
							tableTime += eventDuration;
						}
					}
					if (tablePos < tableWidth)
						result << "<td colspan=" << (tableWidth - tablePos) / d_min / 15 << ">&nbsp;</td>";

					result << "</tr>\n";
				}
				eEPGCache::getInstance()->Unlock();

				multiEPG += result.str();
			}
		}
	}

	eMEPG(time_t start, const eServiceReference & bouquetRef)
		:hours(6)   // horizontally visible hours
		,d_min(5)  // distance on time scale for 1 minute
		,start(start)
		,end(start + hours * 3600)
		,tableWidth((end - start) / 60 * d_min + CHANNELWIDTH)
	{
		Signal1<void, const eServiceReference&> cbSignal;
		CONNECT(cbSignal, eMEPG::getcurepg);
		eServiceInterface::getInstance()->enterDirectory(bouquetRef, cbSignal);
		eServiceInterface::getInstance()->leaveDirectory(bouquetRef);
	}

	eString getMultiEPG()
	{
		return multiEPG;
	}

	eString getTimeScale()
	{
		std::stringstream result;

		result << "<tr>"
			<< "<th width=" << eString().sprintf("%d", CHANNELWIDTH) << ">"
			<< "CHANNEL"
			<< "<br>"
			<< "<img src=\"trans.gif\" border=\"0\" height=\"1\" width=\"" << eString().sprintf("%d", CHANNELWIDTH) << "\">"
			<< "</th>";

		for (time_t i = start; i < end; i += 15 * 60)
		{
			tm* t = localtime(&i);
			result << "<th width=" << d_min * 15 << ">"
				<< std::setfill('0')
				<< std::setw(2) << t->tm_mday << '.'
				<< std::setw(2) << t->tm_mon+1 << "."
				<< "<br>"
				<< std::setw(2) << t->tm_hour << ':'
				<< std::setw(2) << t->tm_min << ' '
				<< "<br>"
				<< "<img src=\"trans.gif\" border=\"0\" height=\"1\" width=\"" << eString().sprintf("%d", 15 * d_min) << "\">"
				<< "</th>";
		}
		result << "</tr>";

		return result.str();
	}
};

static eString getMultiEPG(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString>opt = getRequestOptions(opts, '&');
	eString refs = opt["ref"];
	eServiceReference bouquetRef = string2ref(refs);

	time_t start = time(0) + eDVB::getInstance()->time_difference;
	start -= ((start % 900) + (60 * 60)); // align to 15 mins & start 1 hour before now

	eMEPG mepg(start, bouquetRef);

	eString result = readFile(TEMPLATE_DIR + "mepg.tmp");
	result.strReplace("#TIMESCALE#", mepg.getTimeScale());
	result.strReplace("#BODY#", mepg.getMultiEPG());
	return result;
}

static eString getcurepg2(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::stringstream result;
	eString description, ext_description;
	result << std::setfill('0');

	eService* current;
	eServiceReference ref;

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString,eString> opt=getRequestOptions(opts, '&');

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "No EPG available";

	eString serviceRef = opt["ref"];
	if (!serviceRef)
		ref = sapi->service;
	else
		ref = string2ref(serviceRef);

	eDebug("[ENIGMA_DYN] getcurepg2: opts = %s, serviceRef = %s", opts.c_str(), serviceRef.c_str());

	current = eDVB::getInstance()->settings->getTransponders()->searchService(ref);

	if (!current)
		return "No EPG available";

	eEPGCache::getInstance()->Lock();
	const timeMap* evt = eEPGCache::getInstance()->getTimeMap((eServiceReferenceDVB&)ref);

	if (!evt)
		return "No EPG available";
	else
	{
		timeMap::const_iterator It;

		for(It=evt->begin(); It!= evt->end(); It++)
		{
			ext_description = "";
			EITEvent event(*It->second);
			for (ePtrList<Descriptor>::iterator d(event.descriptor); d != event.descriptor.end(); ++d)
			{
				Descriptor *descriptor = *d;
				if (descriptor->Tag() == DESCR_EXTENDED_EVENT)
				{
					ext_description += ((ExtendedEventDescriptor*)descriptor)->text;
				}
				if (descriptor->Tag() == DESCR_SHORT_EVENT)
				{
					description = ((ShortEventDescriptor*)descriptor)->event_name;
				}
			}

			tm* t = localtime(&event.start_time);
			result << "<tr valign=\"middle\">"
				<< "<td>"
				<< "<span class=\"time\">"
				<< std::setw(2) << t->tm_mday << '.'
				<< std::setw(2) << t->tm_mon+1 << ". - "
				<< std::setw(2) << t->tm_hour << ':'
				<< std::setw(2) << t->tm_min << ' '
				<< "</span>"
				<< "</td>";
#ifndef DISABLE_FILE
			result << "<td>"
				<< "<a href=\"javascript:record('"
				<< "ref=" << ref2string(ref)
				<< "&ID=" << std::hex << event.event_id << std::dec
				<< "&start=" << event.start_time
				<< "&duration=" << event.duration
				<< "&descr=" << filter_string(description)
				<< "&channel=" << filter_string(current->service_name)
				<< "')\"><img src=\"timer.gif\" border=0></a>"
				<< "</td>";
#endif
			result << "<td>"
				<< "<span class=\"event\">"
				<< filter_string(description)
				<< "</span>"
				<< "<br>"
				<< "<span class=\"description\">"
				<< filter_string(ext_description)
				<< "</span>"
				<< "</td>"
				<< "</tr>\n";
		}
	}
	eEPGCache::getInstance()->Unlock();

	eString tmp = readFile(TEMPLATE_DIR + "epg.tmp");
	tmp.strReplace("#CHANNEL#", filter_string(current->service_name));
	tmp.strReplace("#BODY#", result.str());
	return tmp;
}

static eString wapEPG(int page)
{
	std::stringstream result;
	eString description;
	result << std::setfill('0');

	eService* current;

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "No EPG available";

	eServiceReference ref = sapi->service;

	current = eDVB::getInstance()->settings->getTransponders()->searchService(ref);

	if (!current)
		return "No EPG available";

	eEPGCache::getInstance()->Lock();
	const timeMap* evt = eEPGCache::getInstance()->getTimeMap((eServiceReferenceDVB&)ref);

	if (!evt)
		return "No EPG available";
	else
	{
		timeMap::const_iterator It;

		int i = 0;
		for(It=evt->begin(); It!= evt->end(); It++)
		{
			if ((i >= page * 25) && (i < (page + 1) * 25))
			{
				EITEvent event(*It->second);
				for (ePtrList<Descriptor>::iterator d(event.descriptor); d != event.descriptor.end(); ++d)
				{
					Descriptor *descriptor = *d;
					if (descriptor->Tag() == DESCR_SHORT_EVENT)
						description = ((ShortEventDescriptor*)descriptor)->event_name;
				}

				tm* t = localtime(&event.start_time);

				result	<< std::setw(2) << t->tm_mday << '.'
					<< std::setw(2) << t->tm_mon+1 << ". - "
					<< std::setw(2) << t->tm_hour << ':'
					<< std::setw(2) << t->tm_min << ' '
					<< "<br/>";

				result << "<a href=\"/wap?mode=epgDetails"
							<< ",path=" << ref2string(ref)
							<< ",ID=" << std::hex << event.event_id << std::dec
							<< "\">"
							<< filter_string(description)
							<< "</a><br/>\n";
			}
			i++;
		}
		if (i >= (page + 1) * 25)
		{
			page++;
			result << "<a href=\"wap?mode=epg,page=" << eString().sprintf("%d", page) << "\">Next Page</a><br/>";
		}
	}
	eEPGCache::getInstance()->Unlock();

	eString tmp = readFile(TEMPLATE_DIR + "wapepg.tmp");
	tmp.strReplace("#CHANNEL#", filter_string(current->service_name));
	tmp.strReplace("#BODY#", result.str());
	return tmp;
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

	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	eString requester = opt["requester"];
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

	if (requester == "webif")
	{
		result << "<html>" CHARSETMETA "<head><title>Stream Info</title><link rel=\"stylesheet\" type=\"text/css\" href=\"/webif.css\"></head><body bgcolor=#ffffff>"
			"<!-- " << sapi->service << "-->" << std::endl <<
			"<table cellspacing=0 cellpadding=0 border=0>"
			"<tr><td>Name:</td><td>" << name << "</td></tr>"
			"<tr><td>Provider:</td><td>" << provider << "</td></tr>"
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
			"</body></html>";
	}
	else
	{
		result << "<html>" CHARSETMETA "<head><title>streaminfo</title><link rel=\"stylesheet\" type=\"text/css\" href=\"/si.css\"></head><body bgcolor=#000000>"
			"<!-- " << sapi->service.toString() << "-->\n"
			"<table cellspacing=0 cellpadding=0 border=0>"
			"<tr><td>name:</td><td>" + name + "</td></tr>"
			"<tr><td>provider:</td><td>" + provider + "</td></tr>"
			"<tr><td>vpid:</td><td>" + vpid + "</td></tr>"
			"<tr><td>apid:</td><td>" + apid + "</td></tr>"
			"<tr><td>pcrpid:</td><td>" + pcrpid + "</td></tr>"
			"<tr><td>tpid:</td><td>" + tpid + "</td></tr>"
			"<tr><td>tsid:</td><td>" + tsid + "</td></tr>"
			"<tr><td>onid:</td><td>" + onid + "</td></tr>"
			"<tr><td>sid:</td><td>" + sid + "</td></tr>"
			"<tr><td>pmt:</td><td>" + pmt + "</td></tr>"
			"<tr><td>vidformat:<td>" + vidform + "</td></tr>"
			"</table>"
			"</body></html>";
	}

	return result.str();
}

static eString getchannelinfo(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result = getEITC(readFile(TEMPLATE_DIR + "eit.tmp"));
	result.strReplace("#SERVICENAME#", getCurService());

	return result;
}

static eString message(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	std::map<eString, eString> opts = getRequestOptions(opt, '&');
	eString msg = opts["msg"];
	eString result = "-error";
	if (msg == "")
		msg = opt;
	if (msg.length())
	{
		msg = httpUnescape(msg);
		eZapMain::getInstance()->postMessage(eZapMessage(1, "External Message", msg, 10), 0);
		result = "+ok";
	}
	if (opts.find("msg") == opts.end())
		return result;
	else
		return "<script language=\"javascript\">window.close();</script>";
}

static eString startPlugin(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	std::map<eString, eString> opts = getRequestOptions(opt, '&');
	eString requester = opts["requester"];
	eString result;

/*	if (opts.find("path") == opts.end())
		return "E: no path set";*/

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

	result = plugins.execPluginByName((path + opts["name"]).c_str());
	if (requester == "webif")
	{
		content->code=204;
		content->code_descr="No Content";
		result = NOCONTENT;
	}

	return result;
}

static eString stopPlugin(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	std::map<eString, eString> opts = getRequestOptions(opt, '&');
	eString requester = opts["requester"];
	eString result;
	
	if (ePluginThread::getInstance())
	{
		ePluginThread::getInstance()->kill(true);
		result = "+ok, plugin is stopped";
	}
	else
		result = "E: no plugin is running";
		
	if (requester == "webif")
	{
		content->code=204;
		content->code_descr="No Content";
		result = NOCONTENT;
	}

	return result;
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

static eString zapTo(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString,eString> opt = getRequestOptions(opts, '&');

	eString mode = opt["mode"];
	eString spath = opt["path"];
	eString curBouquet = opt["curBouquet"];
	eString curChannel = opt["curChannel"];
	if (opts.find("curBouquet") != eString::npos)
		currentBouquet = atoi(curBouquet.c_str());
	if (opts.find("curChannel") != eString::npos)
		currentChannel = atoi(curChannel.c_str());

	eServiceReference current_service = string2ref(spath);

	if (!(current_service.flags&eServiceReference::isDirectory))	// is playable
		eZapMain::getInstance()->playService(current_service, eZapMain::psSetMode|eZapMain::psDontAdd);

	content->code=204;
	content->code_descr="No Content";
	return NOCONTENT;
}

#if 0
is this still used???

#define NAVIGATOR_PATH "/cgi-bin/navigator"

class eNavigatorListDirectory: public Object
{
	eString &result;
	eString path;
	eServiceInterface &iface;
	int num;
public:
	eNavigatorListDirectory(eString &result, eString path, eServiceInterface &iface): result(result), path(path), iface(iface)
	{
		eDebug("path: %s", path.c_str());
		num = 0;
	}
	void addEntry(const eServiceReference &e)
	{
		result += "<tr><td bgcolor=\"#";
		if (num & 1)
			result += "c0c0c0";
		else
			result += "d0d0d0";
		result+="\"><font color=\"#000000\">";
		if (!(e.flags & eServiceReference::isDirectory))
			result += "[PLAY] ";

		result += eString("<a href=\"" NAVIGATOR_PATH) + "?path=" + path + ref2string(e) +"\">" ;

		eService *service = iface.addRef(e);
		if (!service)
			result += "N/A";
		else
			result += service->service_name;
		iface.removeRef(e);

		result += "</a></font></td></tr>\n";
		eDebug("+ok");
		num++;
	}
};

static eString navigator(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	std::map<eString, eString> opts = getRequestOptions(opt, '&');

	if (opts.find("path") == opts.end())
	{
		content->code=301;
		content->code_descr="Moved Permanently";
		content->local_header["Location"]=eString(NAVIGATOR_PATH) + "?path=" + ref2string(eServiceReference(eServiceReference::idStructure, eServiceReference::isDirectory, 0));
		return "redirecting..";
	}
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString spath = opts["path"];

	eServiceInterface *iface = eServiceInterface::getInstance();
	if (!iface)
		return "n/a\n";

	eString current;

	unsigned int pos;
	if ((pos=spath.rfind(';')) != eString::npos)
	{
		current = spath.mid(pos + 1);
		spath = spath.left(pos);
	} else
	{
		current = spath;
		spath = "";
	}

	eDebug("current service: %s", current.c_str());
	eServiceReference current_service(string2ref(current));

	eString res;

	res="<html>\n"
		CHARSETMETA
		"<head><title>Enigma Navigator</title></head>\n"
		"<body bgcolor=\"#f0f0f0\">\n"
		"<font color=\"#000000\">\n";

	res += eString("Current: ") + current + "<br>\n";
	res += "<hr>\n";
	res += eString("path: ") + spath + "<br>\n";

	if (!(current_service.flags&eServiceReference::isDirectory))	// is playable
	{
		eZapMain::getInstance()->playService(current_service, eZapMain::psSetMode|eZapMain::psDontAdd);
//		iface->play(current_service);
		res += "+ok, hear the music..";
	}
	else
	{
		eNavigatorListDirectory navlist(res, spath + ";" + current + ";", *iface);
		Signal1<void,const eServiceReference&> signal;
		signal.connect(slot(navlist, &eNavigatorListDirectory::addEntry));

		res += "<table width=\"100%\">\n";
		iface->enterDirectory(current_service, signal);
		res += "</table>\n";
		eDebug("entered");
		iface->leaveDirectory(current_service);
		eDebug("exited");
	}

	return res;
}
#endif

static eString getCurrentServiceRef(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if (eServiceInterface::getInstance()->service)
		return eServiceInterface::getInstance()->service.toString();
	else
		return "E:no service running";
}

static eString web_root(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;

	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	content->local_header["Content-Type"]="text/html; charset=utf-8";

	if (opts.find("screenWidth") != eString::npos)
	{
		eString sWidth = opt["screenWidth"];
		int screenWidth = atoi(sWidth.c_str());
		smallScreen = (screenWidth < 800) ? 1 : 0;
	}
	else
	{
		if ((opts.find("mode") == eString::npos) && (opts.find("path") == eString::npos))
			return readFile(TEMPLATE_DIR + "index.tmp");
	}

	if (smallScreen == 0)
	{
		result = readFile(TEMPLATE_DIR + "index_big.tmp");

		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000)
			result.strReplace("#BOX#", "Dreambox");
		else
			result.strReplace("#BOX#", "dBox");
	}
	else
	{
		eString mode = opt["mode"];
		eString spath = opt["path"];

		eDebug("[ENIGMA_DYN] web_root_small: mode = %s, spath = %s", mode.c_str(), spath.c_str());

		if (!spath)
			spath = eServiceStructureHandler::getRoot(eServiceStructureHandler::modeTV).toString();

		if (!mode)
			mode = "zap";

		result = readFile(TEMPLATE_DIR + "index_small.tmp");
		result.strReplace("#CONTENT#", getContent(mode, spath));
		result.strReplace("#VOLBAR#", getVolBar());
		result.strReplace("#TOPNAVI#", getTopNavi(mode, spath));
		result.strReplace("#CHANNAVI#", getChannelNavi());
		result.strReplace("#LEFTNAVI#", getLeftNavi(mode, spath));
		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000)
			result.strReplace("#TOPBALK#", "topbalk_small.png");
		else
		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::dbox2Nokia)
			result.strReplace("#TOPBALK#", "topbalk2_small.png");
		else
		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::dbox2Sagem)
			result.strReplace("#TOPBALK#", "topbalk3_small.png");
		else
//		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::dbox2Philips)
			result.strReplace("#TOPBALK#", "topbalk4_small.png");
	}

	return result;
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
		d = opendir(opt.c_str());
	}
	if (d)
	{
		char buffer[255];
		int dircount, filecount, linkcount;
		dircount = filecount = linkcount = 0;
		while (struct dirent *e = readdir(d))
		{
			eString filename = opt;
			filename += e->d_name;

			struct stat s;
			if (lstat(filename.c_str(), &s) < 0)
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
				answer+=eString().sprintf("\t<object type=\"file\" name=\"%s\" size=\"%d\"/>\n",
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
	if (opt.find("&&") == eString::npos)
	{
		if (system(eString().sprintf("mkdir %s", opt.c_str()).c_str()) >> 8)
			return eString().sprintf("E: create directory %s failed", opt.c_str());
		return "+ok";
	}
	return "E: invalid command";
}

static eString removeDirectory(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if (opt.find("&&") == eString::npos)
	{
		if (system(eString().sprintf("rmdir %s", opt.c_str()).c_str()) >> 8)
			return eString().sprintf("E: remove directory %s failed", opt.c_str());
		return "+ok";
	}
	return "E: invalid command";
}

static eString removeFile(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if (opt.find("&&") == eString::npos)
	{
		if (system(eString().sprintf("rm %s", opt.c_str()).c_str()) >> 8)
			return eString().sprintf("E: remove file %s failed", opt.c_str());
		return "+ok";
	}
	return "E: invalid command";
}

static eString moveFile(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if (opt.find("&&") == eString::npos)
	{
		std::map<eString,eString> opts=getRequestOptions(opt, '&');
		if (opts.find("source") == opts.end() || !opts["source"].length())
			return "E: option source missing or empty source given";
		if (opts.find("dest") == opts.end() || !opts["dest"].length())
			return "E: option dest missing or empty dest given";
		if (system(eString().sprintf("mv %s %s", opts["source"].c_str(), opts["dest"].c_str()).c_str()) >> 8)
			return eString().sprintf("E: cannot move %s to %s", opts["source"].c_str(), opts["dest"].c_str());
		return "+ok";
	}
	return "E: invalid command";
}

static eString createSymlink(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if (opt.find("&&") == eString::npos)
	{
		std::map<eString,eString> opts=getRequestOptions(opt, '&');
		if (opts.find("source") == opts.end() || !opts["source"].length())
			return "E: option source missing or empty source given";
		if (opts.find("dest") == opts.end() || !opts["dest"].length())
			return "E: option dest missing or empty dest given";
		if (system(eString().sprintf("ln -sf %s %s", opts["source"].c_str(), opts["dest"].c_str()).c_str()) >> 8)
			return eString().sprintf("E: cannot create symlink %s to %s", opts["source"].c_str(), opts["dest"].c_str());
		return "+ok";
	}
	return "E: invalid command";
}

#if 0
is this still used???
static eString neutrino_suck_zapto(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if (opt != "getpids")
		return eString("+ok");
	else
		return eString().sprintf("%u\n%u\n", Decoder::current.vpid, Decoder::current.apid);
}

static eString neutrino_suck_getonidsid(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi || !sapi->service)
		return "200\n";

	int onidsid = (sapi->service.getOriginalNetworkID().get() << 8)
		| sapi->service.getServiceID().get();

	return eString().sprintf("%d\n", onidsid);
}
#endif

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

#if 0
static eString neutrino_suck_getchannellist(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString channelstring;

	eTransponderList::getInstance()->forEachService(appendonidsidnamestr(channelstring));

	return channelstring;
}
#endif

static eString cleanupTimerList(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eTimerManager::getInstance()->cleanupEvents();
	eTimerManager::getInstance()->saveTimerList(); //not needed, but in case enigma crashes ;-)
	content->code=204;
	content->code_descr="No Content";
	return NOCONTENT;
}

static eString clearTimerList(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eTimerManager::getInstance()->clearEvents();
	eTimerManager::getInstance()->saveTimerList(); //not needed, but in case enigma crashes ;-)
	content->code=204;
	content->code_descr="No Content";
	return NOCONTENT;
}

static eString addTimerEvent(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString serviceRef = opt["ref"];
	eString eventID = opt["ID"];
	eString eventStartTime = opt["start"];
	eString eventDuration = opt["duration"];
	eString channel = httpUnescape(opt["channel"]);
	eString description = httpUnescape(opt["descr"]);
	if (description == "")
		description = _("No description available");

	int eventid;
	sscanf(eventID.c_str(), "%x", &eventid);
	eDebug("[ENIGMA_DYN] addTimerEvent: serviceRef = %s, ID = %s, start = %s, duration = %s\n", serviceRef.c_str(), eventID.c_str(), eventStartTime.c_str(), eventDuration.c_str());

	int timeroffset = 0;
	if ((eConfig::getInstance()->getKey("/enigma/timeroffset", timeroffset)) != 0)
		timeroffset = 0;

	int start = atoi(eventStartTime.c_str()) - (timeroffset * 60);
	int duration = atoi(eventDuration.c_str()) + (2 * timeroffset * 60);

	ePlaylistEntry entry(string2ref(serviceRef), start, duration, eventid, ePlaylistEntry::stateWaiting | ePlaylistEntry::RecTimerEntry | ePlaylistEntry::recDVR);
	entry.service.descr = channel + "/" + description;

	if (eTimerManager::getInstance()->addEventToTimerList(entry) == -1)
		result += _("Timer event could not be added because time of the event overlaps with an already existing event.");
	else
		result += _("Timer event was created successfully.");
	eTimerManager::getInstance()->saveTimerList(); //not needed, but in case enigma crashes ;-)
	return result;
}

static eString wapAddTimerEvent(eString opts)
{
	eString result;

	std::map<eString, eString> opt = getRequestOptions(opts, ',');
	eString serviceRef = opt["path"];
	eString eventID = opt["ID"];
	eString eventStartTime = opt["start"];
	eString eventDuration = opt["duration"];
	eString channel = httpUnescape(opt["channel"]);
	eString description = httpUnescape(opt["descr"]);
	if (description == "")
		description = _("No description available");

	int eventid;
	sscanf(eventID.c_str(), "%x", &eventid);

	int timeroffset = 0;
	if ((eConfig::getInstance()->getKey("/enigma/timeroffset", timeroffset)) != 0)
		timeroffset = 0;

	int start = atoi(eventStartTime.c_str()) - (timeroffset * 60);
	int duration = atoi(eventDuration.c_str()) + (2 * timeroffset * 60);

	ePlaylistEntry entry(string2ref(serviceRef), start, duration, eventid, ePlaylistEntry::stateWaiting | ePlaylistEntry::RecTimerEntry | ePlaylistEntry::recDVR);
	entry.service.descr = channel + "/" + description;

	if (eTimerManager::getInstance()->addEventToTimerList(entry) == -1)
		result += _("Timer event could not be added because time of the event overlaps with an already existing event.");
	else
		result += _("Timer event was created successfully.");
	eTimerManager::getInstance()->saveTimerList(); //not needed, but in case enigma crashes ;-)
	return result;
}

static eString deleteTimerEvent(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString serviceRef = opt["ref"];
	eString eventType = opt["type"];
	eString eventStartTime = opt["start"];
	eString force = opt["force"];
	eString result;

	eDebug("[ENIGMA_DYN] deleteTimerEvent: serviceRef = %s, type = %s, start = %s", serviceRef.c_str(), eventType.c_str(), eventStartTime.c_str());

	ePlaylistEntry e(
		string2ref(serviceRef),
		atoi(eventStartTime.c_str()),
		-1, -1, atoi(eventType.c_str()));

	int ret = eTimerManager::getInstance()->deleteEventFromTimerList(e, (force == "yes"));

	content->local_header["Content-Type"]="text/html; charset=utf-8";

	if ( ret == -1 )  // event currently running...
	{
		// ask user if he really wants to do this..
		// then call deleteEventFromtTimerList again.. with true as second parameter..
		// then the running event will aborted
		result = readFile(TEMPLATE_DIR + "queryDeleteTimer.tmp");
		opts.strReplace("force=no", "force=yes");
		if (opts.find("?") != 0)
			opts = "?" + opts;
		result.strReplace("#URL#", "/deleteTimerEvent" + opts);
	}
	else
	{
		result = readFile(TEMPLATE_DIR + "deleteTimerComplete.tmp");
		eTimerManager::getInstance()->saveTimerList(); //not needed, but in case enigma crashes ;-)
	}

	return result;
}

static eString genOptions(int start, int end, int delta, int selected)
{
	std::stringstream result;
	for (int i = start; i <= end; i += delta)
	{
		if (i == selected)
			result << "<option selected>";
		else
			result << "<option>";
		result << std::setfill('0') << std::setw(2);
		result << i;
		result << "</option>";
	}
	return result.str();
}

static eString changeTimerEvent(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');

	// to find old event in timerlist..
	eString serviceRef = opt["ref"];
	eString oldEventType = opt["old_type"];
	eString oldStartTime = opt["old_stime"];

	eString eventID = opt["ID"];
	eString sday = opt["sday"];
	eString smonth = opt["smonth"];
	eString shour = opt["shour"];
	eString smin = opt["smin"];
	eString eday = opt["eday"];
	eString emonth = opt["emonth"];
	eString ehour = opt["ehour"];
	eString emin = opt["emin"];
	eString description = httpUnescape(opt["descr"]);
	eString channel = httpUnescape(opt["channel"]);
	eString after_event = opt["after_event"];
	eString force = opt["force"];

	time_t now = time(0)+eDVB::getInstance()->time_difference;
	tm start = *localtime(&now);
	start.tm_mday = atoi(sday.c_str());
	start.tm_mon = atoi(smonth.c_str()) - 1;
	start.tm_hour = atoi(shour.c_str());
	start.tm_min = atoi(smin.c_str());
	start.tm_sec = 0;
	tm end = *localtime(&now);
	end.tm_mday = atoi(eday.c_str());
	end.tm_mon = atoi(emonth.c_str()) -1 ;
	end.tm_hour = atoi(ehour.c_str());
	end.tm_min = atoi(emin.c_str());
	end.tm_sec = 0;

	time_t eventStartTime = mktime(&start);
	time_t eventEndTime = mktime(&end);
	int duration = eventEndTime - eventStartTime;
	int oldType = atoi(oldEventType.c_str());

	int eventid;
	sscanf(eventID.c_str(), "%x", &eventid);
	// eDebug("[CHANGETIMER] start: %d.%d. - %d:%d, end: %d.%d. - %d:%d", start.tm_mday, start.tm_mon, start.tm_hour, start.tm_min,
	//								end.tm_mday, end.tm_mon, end.tm_hour, end.tm_min);

	eServiceReference ref = string2ref(serviceRef);

	ePlaylistEntry oldEvent(
		ref,
		atoi(oldStartTime.c_str()),
		-1, -1, oldType);

	if ( oldStartTime < now && eventStartTime >= now )
	{
		oldType &=
			~(ePlaylistEntry::stateRunning|
				ePlaylistEntry::statePaused|
				ePlaylistEntry::stateFinished|
				ePlaylistEntry::stateError|
				ePlaylistEntry::errorNoSpaceLeft|
				ePlaylistEntry::errorUserAborted|
				ePlaylistEntry::errorZapFailed|
				ePlaylistEntry::errorOutdated);
		oldType &= ~(ePlaylistEntry::doGoSleep|ePlaylistEntry::doShutdown);
		oldType |= ePlaylistEntry::stateWaiting;
		oldType |= atoi(after_event.c_str());
	}

	ref.descr = channel + "/" + description;
	ePlaylistEntry newEvent(
		ref,
		eventStartTime,
		duration,
		eventid,
		oldType);

	int ret = eTimerManager::getInstance()->modifyEventInTimerList(oldEvent, newEvent, (force == "yes"));

	if ( ret == -1 )  // event currently running...
	{
		// ask user if he wants to update only after_event action and duration
		// then call modifyEvent again.. with true as third parameter..
		result = readFile(TEMPLATE_DIR + "queryEditTimer.tmp");
		opts.strReplace("force=no", "force=yes");
		if (opts.find("?") != 0)
			opts = "?" + opts;
		result.strReplace("#URL#", "/changeTimerEvent" + opts);
	}
	else
	{
		result = "<script language=\"javascript\">window.close();</script>";
		eTimerManager::getInstance()->saveTimerList(); //not needed, but in case enigma crashes ;-)	
	}
	return result;
}

static eString addTimerEvent2(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString serviceRef = opt["ref"];
	eString sday = opt["sday"];
	eString smonth = opt["smonth"];
	eString shour = opt["shour"];
	eString smin = opt["smin"];
	eString eday = opt["eday"];
	eString emonth = opt["emonth"];
	eString ehour = opt["ehour"];
	eString emin = opt["emin"];
	eString description = httpUnescape(opt["descr"]);
	eString channel = httpUnescape(opt["channel"]);
	eString after_event = opt["after_event"];

	time_t now = time(0) + eDVB::getInstance()->time_difference;
	tm start = *localtime(&now);
	start.tm_mday = atoi(sday.c_str());
	start.tm_mon = atoi(smonth.c_str()) - 1;
	start.tm_hour = atoi(shour.c_str());
	start.tm_min = atoi(smin.c_str());
	start.tm_sec = 0;
	tm end = *localtime(&now);
	end.tm_mday = atoi(eday.c_str());
	end.tm_mon = atoi(emonth.c_str()) -1 ;
	end.tm_hour = atoi(ehour.c_str());
	end.tm_min = atoi(emin.c_str());
	end.tm_sec = 0;

	time_t eventStartTime = mktime(&start);
	time_t eventEndTime = mktime(&end);
	int duration = eventEndTime - eventStartTime;
	int eventid = -1;

	int timeroffset = 0;
	eConfig::getInstance()->getKey("/enigma/timeroffset", timeroffset);

	time_t start1 = eventStartTime - (timeroffset * 60);
	duration = duration + (2 * timeroffset * 60);

	int type = atoi(after_event.c_str());
	type |= ePlaylistEntry::stateWaiting | ePlaylistEntry::RecTimerEntry | ePlaylistEntry::recDVR;

	ePlaylistEntry entry(string2ref(serviceRef), start1, duration, eventid, type);
	entry.service.descr = channel + "/" + description;

	if (eTimerManager::getInstance()->addEventToTimerList(entry) == -1)
		result += _("Timer event could not be added because time of the event overlaps with an already existing event.");
	else
	{
		result += _("Timer event was created successfully.");
		eTimerManager::getInstance()->saveTimerList(); //not needed, but in case enigma crashes ;-)
	}

	return result;
}

static eString buildAfterEventOpts(int type)
{
	std::stringstream afterOpts;
	if ( type & ePlaylistEntry::doGoSleep || type & ePlaylistEntry::doShutdown )
		afterOpts << "<option value=\"0\">";
	else
		afterOpts << "<option selected value=\"0\">";
	afterOpts << _("Nothing")
		<< "</option>";
	if ( type & ePlaylistEntry::doGoSleep )
		afterOpts << "<option selected value=\"" << ePlaylistEntry::doGoSleep << "\">";
	else
		afterOpts << "<option value=\"" << ePlaylistEntry::doGoSleep << "\">";
	afterOpts << _("Standby")
		<< "</option>";
	if ( type & ePlaylistEntry::doShutdown )
		afterOpts << "<option selected value=\"" << ePlaylistEntry::doShutdown << "\">";
	else
		afterOpts << "<option value=\"" << ePlaylistEntry::doShutdown << "\">";
	afterOpts << _("Shutdown")
		<< "</option>";
	return afterOpts.str();
}

static eString editTimerEvent(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString serviceRef = opt["ref"];
	eString eventID = opt["ID"];
	eString eventStartTime = opt["start"];
	eString eventDuration = opt["duration"];
	eString description = httpUnescape(opt["descr"]);

	// this is only for renamed services ( or subservices )... changing this in the edit dialog has no effect to
	// the recording service
	eString channel = httpUnescape(opt["channel"]);

	eString eventType = opt["type"];

	time_t eventStart = atoi(eventStartTime.c_str());
	time_t eventEnd = eventStart + atoi(eventDuration.c_str());
	tm start = *localtime(&eventStart);
	tm end = *localtime(&eventEnd);
	int evType = atoi(eventType.c_str());

	// eDebug("[ENIGMA_DYN] editTimerEvent: serviceRef = %s, ID = %s, start = %s, duration = %s", serviceRef.c_str(), eventID.c_str(), eventStartTime.c_str(), eventDuration.c_str());

	// TODO: check if ( type & ePlaylistEntry::isRepeating )
	// .. then load another template.. with checkboxes for weekdays..
	eString result = readFile(TEMPLATE_DIR + "editTimerEvent.tmp");

	result.strReplace("#AFTERTEXT#", _("After Event:"));
	result.strReplace("#AFTEROPTS#", buildAfterEventOpts(evType));
	result.strReplace("#EVENTID#", eventID);
// these three values we need to find the old event in timerlist...
	result.strReplace("#SERVICEREF#", serviceRef);
	result.strReplace("#OLD_TYPE#", eventType);
	result.strReplace("#OLD_STIME#", eventStartTime);

	result.strReplace("#SDAYOPTS#", genOptions(1, 31, 1, start.tm_mday));
	result.strReplace("#SMONTHOPTS#", genOptions(1, 12, 1, start.tm_mon + 1));
	result.strReplace("#SHOUROPTS#", genOptions(0, 23, 1, start.tm_hour));
	result.strReplace("#SMINOPTS#", genOptions(0, 55, 5, (start.tm_min / 5) * 5));

	result.strReplace("#EDAYOPTS#", genOptions(1, 31, 1, end.tm_mday));
	result.strReplace("#EMONTHOPTS#", genOptions(1, 12, 1, end.tm_mon + 1));
	result.strReplace("#EHOUROPTS#", genOptions(0, 23, 1, end.tm_hour));
	int endTime = end.tm_min / 5 * 5;
	if (end.tm_min % 5 > 0)
		endTime += 5;
	result.strReplace("#EMINOPTS#", genOptions(0, 55, 5, endTime));
	result.strReplace("#CHANNEL#", channel);
	result.strReplace("#DESCRIPTION#", description);
	eTimerManager::getInstance()->saveTimerList(); //not needed, but in case enigma crashes ;-)
	return result;
}

static eString showAddTimerEventWindow(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');

	time_t now = time(0) + eDVB::getInstance()->time_difference;
	tm start = *localtime(&now);
	tm end = *localtime(&now);

	eString result = readFile(TEMPLATE_DIR + "addTimerEvent.tmp");

	result.strReplace("#AFTERTEXT#", _("After Event:"));
	result.strReplace("#AFTEROPTS#", buildAfterEventOpts(0));
	
	result.strReplace("#SDAYOPTS#", genOptions(1, 31, 1, start.tm_mday));
	result.strReplace("#SMONTHOPTS#", genOptions(1, 12, 1, start.tm_mon + 1));
	result.strReplace("#SHOUROPTS#", genOptions(0, 23, 1, start.tm_hour));
	result.strReplace("#SMINOPTS#", genOptions(0, 55, 5, (start.tm_min / 5) * 5));

	result.strReplace("#EDAYOPTS#", genOptions(1, 31, 1, end.tm_mday));
	result.strReplace("#EMONTHOPTS#", genOptions(1, 12, 1, end.tm_mon + 1));
	result.strReplace("#EHOUROPTS#", genOptions(0, 23, 1, end.tm_hour));
	result.strReplace("#EMINOPTS#", genOptions(0, 55, 5, (end.tm_min / 5) * 5));

	result.strReplace("#ZAPDATA#", getZapContent2("zap", zap[ZAPMODETV][ZAPSUBMODEBOUQUETS]));

	return result;
}

static eString EPGDetails(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;
	eService *current = NULL;
	eString ext_description;

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString serviceRef = opt["ref"];
	eString eventID = opt["ID"];
	int eventid;
	eString description = _("No description available");

	sscanf(eventID.c_str(), "%x", &eventid);
	eDebug("[ENIGMA_DYN] getEPGDetails: serviceRef = %s, ID = %04x", serviceRef.c_str(), eventid);

	// search for the event... to get the description...
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		eServiceReference ref(string2ref(serviceRef));
		current = eDVB::getInstance()->settings->getTransponders()->searchService((eServiceReferenceDVB&)ref);
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
				delete event;
			}
		}
	}
	if (!ext_description)
		ext_description = _("No detailed description available");

	result = readFile(TEMPLATE_DIR + "epgDetails.tmp");
	result.strReplace("#EVENT#", filter_string(description));
	result.strReplace("#BODY#", filter_string(ext_description));

	return result;
}

static eString wapEPGDetails(eString serviceRef, eString eventID)
{
	eString result;
	eService *current = NULL;
	eString ext_description;
	std::stringstream record;
	int eventid;
	eString description = _("No description available");

	sscanf(eventID.c_str(), "%x", &eventid);
	eDebug("[ENIGMA_DYN] getEPGDetails: serviceRef = %s, ID = %04x", serviceRef.c_str(), eventid);

	// search for the event... to get the description...
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		eServiceReference ref(string2ref(serviceRef));
		current = eDVB::getInstance()->settings->getTransponders()->searchService((eServiceReferenceDVB&)ref);
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
					ext_description = _("No detailed information available");
#ifndef DISABLE_FILE
				record << "<a href=\"/wap?mode=addTimerEvent"
					<< ",path=" << ref2string(ref)
					<< ",ID=" << std::hex << event->event_id << std::dec
					<< ",start=" << event->start_time
					<< ",duration=" << event->duration
					<< ",descr=" << filter_string(description)
					<< ",channel=" << filter_string(current->service_name)
					<< "\">Record</a>";
#endif
				delete event;
			}
		}
	}

	result = readFile(TEMPLATE_DIR + "wapEPGDetails.tmp");
	result.strReplace("#EVENT#", filter_string(description));
	result.strReplace("#RECORD#", record.str());
	result.strReplace("#BODY#", filter_string(ext_description));

	return result;
}

static eString blank(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	return readFile(TEMPLATE_DIR + "blank.tmp");
}

static eString header(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result = readFile(TEMPLATE_DIR + "header.tmp");
	if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000)
	{
		result.strReplace("#TOPBALK#", "topbalk.png");
	}
	else
	{
		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::dbox2Nokia)
			result.strReplace("#TOPBALK#", "topbalk2.png");
		else
		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::dbox2Sagem)
			result.strReplace("#TOPBALK#", "topbalk3.png");
		else
//		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::dbox2Philips)
			result.strReplace("#TOPBALK#", "topbalk4.png");
	}
	result.strReplace("#CHANNAVI#", getChannelNavi());
	return getEITC2(result);
}

static eString body(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;
	std::map<eString,eString> opt=getRequestOptions(opts, '&');
	content->local_header["Content-Type"]="text/html; charset=utf-8";

	eString mode = opt["mode"];
	if (!mode)
		mode = "zap";
	eString curBouquet = opt["curBouquet"];
	if (curBouquet)
		currentBouquet = atoi(curBouquet.c_str());
	eString curChannel = opt["curChannel"];
	if (curChannel)
		currentChannel = atoi(curChannel.c_str());
	eString spath = opt["path"];
	if (!spath)
	{
		zapMode = ZAPMODETV;
		zapSubMode = ZAPSUBMODEBOUQUETS;
		spath = zap[zapMode][zapSubMode];
	}
	else
	{
		for (int i = 0; i < 4; i++)
		{
			if (spath == zap[i][ZAPMODECATEGORY])
			{
				zapMode = i;
				switch(zapMode)
				{
					case ZAPMODETV:
					case ZAPMODERADIO:
						spath = zap[i][ZAPSUBMODEBOUQUETS];
						break;
					case ZAPMODEDATA:
						spath = zap[i][ZAPSUBMODESATELLITES];
						break;
				}
				currentBouquet = 0;
				currentChannel = -1;
				break;
			}
		}

		for (int i = 2; i < 5; i++)
		{
			if (spath == zap[zapMode][i])
			{
				zapSubMode = i;
				break;
			}
		}
	}

	result = readFile(TEMPLATE_DIR + "index2.tmp");
	result.strReplace("#TOPNAVI#", getTopNavi(mode, spath));
	result.strReplace("#LEFTNAVI#", getLeftNavi(mode, spath));
	result.strReplace("#CONTENT#", getContent(mode, spath));

	if (mode == "zap")
		result.strReplace("#ONLOAD#", "onLoad=init()");
	else
		result.strReplace("#ONLOAD#", "");

	return result;
}

static eString wap_web_root(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;

	std::map<eString,eString> opt = getRequestOptions(opts, ',');
	eString mode = opt["mode"];
	eString spath = opt["path"];

	content->local_header["Content-Type"]="text/vnd.wap.wml";

	if (mode == "admin")
	{
		eString command = opt["command"];
		result = admin2(command);
	}
	else
	if (mode == "zap")
	{
		if (opts.find("path") == eString::npos)
			spath = zap[ZAPMODETV][ZAPSUBMODEBOUQUETS];
		result = readFile(TEMPLATE_DIR + "wapzap.tmp");
		result.strReplace("#BODY#", getWapZapContent(spath));
	}
	else
	if (mode == "zapto")
	{
		eServiceReference current_service = string2ref(spath);

		if (!(current_service.flags&eServiceReference::isDirectory))	// is playable
			eZapMain::getInstance()->playService(current_service, eZapMain::psSetMode|eZapMain::psDontAdd);

		result = "<?xml version=\"1.0\"?><!DOCTYPE wml PUBLIC \"-//WAPFORUM//DTD WML 1.1//EN\" \"http://www.wapforum.org/DTD/wml_1.1.xml\"><wml><card title=\"Info\"><p>Zap complete.</p></card></wml>";
	}
	else
	if (mode == "epg")
	{
		eString page = opt["page"];
		result = wapEPG(atoi(page.c_str()));
	}
	else
	if (mode == "epgDetails")
	{
		eString eventID = opt["ID"];
		result = wapEPGDetails(spath, eventID);
	}
	else
	if (mode == "addTimerEvent")
	{
		result = wapAddTimerEvent(opts);
		result = "<?xml version=\"1.0\"?><!DOCTYPE wml PUBLIC \"-//WAPFORUM//DTD WML 1.1//EN\" \"http://www.wapforum.org/DTD/wml_1.1.xml\"><wml><card title=\"Info\"><p>" + result + "</p></card></wml>";

	}
	else
	if (mode == "timer")
	{
		result = wapTimerList();
	}
	else
	if (mode == "cleanupTimerList")
	{
		eTimerManager::getInstance()->cleanupEvents();
		eTimerManager::getInstance()->saveTimerList(); //not needed, but in case enigma crashes ;-)
		result = "<?xml version=\"1.0\"?><!DOCTYPE wml PUBLIC \"-//WAPFORUM//DTD WML 1.1//EN\" \"http://www.wapforum.org/DTD/wml_1.1.xml\"><wml><card title=\"Info\"><p>Timers cleaned up.</p></card></wml>";
	}
	else
	if (mode == "clearTimerList")
	{
		eTimerManager::getInstance()->clearEvents();
		eTimerManager::getInstance()->saveTimerList(); //not needed, but in case enigma crashes ;-)
		result = "<?xml version=\"1.0\"?><!DOCTYPE wml PUBLIC \"-//WAPFORUM//DTD WML 1.1//EN\" \"http://www.wapforum.org/DTD/wml_1.1.xml\"><wml><card title=\"Info\"><p>Timer list cleared.</p></card></wml>";
	}
	else
	{
		result = readFile(TEMPLATE_DIR + "wap.tmp");
		result = getEITC2(result);
		result.strReplace("#SERVICE#", getCurService());
	}

	return result;
}

void ezapInitializeDyn(eHTTPDynPathResolver *dyn_resolver)
{
	int lockWebIf = 1;
	if (eConfig::getInstance()->getKey("/ezap/webif/lockWebIf", lockWebIf))
		eConfig::getInstance()->setKey("/ezap/webif/lockWebIf", lockWebIf);
	
	eDebug("[ENIGMA_DYN] lockWebIf = %d", lockWebIf);
	bool lockWeb = (lockWebIf == 1) ? true : false;

	dyn_resolver->addDyn("GET", "/", web_root, lockWeb);
	dyn_resolver->addDyn("GET", "/wap", wap_web_root, lockWeb);
//	dyn_resolver->addDyn("GET", NAVIGATOR_PATH, navigator, lockWeb);

	dyn_resolver->addDyn("GET", "/cgi-bin/ls", listDirectory, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/mkdir", makeDirectory, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/rmdir", removeDirectory, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/rm", removeFile, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/mv", moveFile, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/ln", createSymlink, lockWeb);
#ifndef DISABLE_FILE
	dyn_resolver->addDyn("GET", "/cgi-bin/stop", stop, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/pause", pause, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/play", play, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/record", record, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/videocontrol", videocontrol, lockWeb);
#endif
	dyn_resolver->addDyn("GET", "/setVolume", setVolume, lockWeb);
	dyn_resolver->addDyn("GET", "/setVideo", setVideo, lockWeb);
	dyn_resolver->addDyn("GET", "/showTimerList", showTimerList, lockWeb);
	dyn_resolver->addDyn("GET", "/addTimerEvent", addTimerEvent, lockWeb);
	dyn_resolver->addDyn("GET", "/addTimerEvent2", addTimerEvent2, lockWeb);
	dyn_resolver->addDyn("GET", "/deleteTimerEvent", deleteTimerEvent, lockWeb);
	dyn_resolver->addDyn("GET", "/editTimerEvent", editTimerEvent, lockWeb);
	dyn_resolver->addDyn("GET", "/showAddTimerEventWindow", showAddTimerEventWindow, lockWeb);
	dyn_resolver->addDyn("GET", "/changeTimerEvent", changeTimerEvent, lockWeb);
	dyn_resolver->addDyn("GET", "/cleanupTimerList", cleanupTimerList, lockWeb);
	dyn_resolver->addDyn("GET", "/clearTimerList", clearTimerList, lockWeb);
	dyn_resolver->addDyn("GET", "/EPGDetails", EPGDetails, lockWeb);
	dyn_resolver->addDyn("GET", "/msgWindow", msgWindow, lockWeb);
	dyn_resolver->addDyn("GET", "/tvMessageWindow", tvMessageWindow, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/status", doStatus, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/switchService", switchService, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/zapTo", zapTo, false); // this dont really zap.. only used to return apid and vpid
	dyn_resolver->addDyn("GET", "/cgi-bin/admin", admin, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/audio", audio, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/selectAudio", selectAudio, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/setAudio", setAudio, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/setScreen", setScreen, lockWeb);
#ifndef DISABLE_FILE
	dyn_resolver->addDyn("GET", "/cgi-bin/setConfigUSB", setConfigUSB, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/setConfigHDD", setConfigHDD, lockWeb);
#endif
	dyn_resolver->addDyn("GET", "/cgi-bin/getPMT", getPMT, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/getEIT", getEIT, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/message", message, lockWeb);
	dyn_resolver->addDyn("GET", "/control/message", message, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/xmessage", xmessage, lockWeb);

	dyn_resolver->addDyn("GET", "/audio.m3u", audiom3u, lockWeb);
	dyn_resolver->addDyn("GET", "/version", version, lockWeb);
	dyn_resolver->addDyn("GET", "/header", header, lockWeb);
	dyn_resolver->addDyn("GET", "/body", body, lockWeb);
	dyn_resolver->addDyn("GET", "/blank", blank, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/getcurrentepg", getcurepg, lockWeb);
	dyn_resolver->addDyn("GET", "/getcurrentepg2", getcurepg2, lockWeb);
	dyn_resolver->addDyn("GET", "/getMultiEPG", getMultiEPG, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/streaminfo", getstreaminfo, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/channelinfo", getchannelinfo, lockWeb);
	dyn_resolver->addDyn("GET", "/channels/getcurrent", channels_getcurrent, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadSettings", reload_settings, lockWeb);
#ifndef DISABLE_FILE
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadRecordings", load_recordings, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/saveRecordings", save_recordings, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/deleteMovie", deleteMovie, lockWeb);
#endif
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadPlaylist", load_playlist, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/savePlaylist", save_playlist, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadUserBouquets", load_userBouquets, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/saveUserBouquets", save_userBouquets, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadTimerList", load_timerList, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/saveTimerList", save_timerList, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/startPlugin", startPlugin, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/stopPlugin", stopPlugin, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/osdshot", osdshot, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/currentService", getCurrentServiceRef, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/currentTransponderServices", getTransponderServices, lockWeb);
#ifndef DISABLE_FILE
	dyn_resolver->addDyn("GET", "/cgi-bin/setFakeRecordingState", setFakeRecordingState, lockWeb);
#endif
#if 0
	dyn_resolver->addDyn("GET", "/control/zapto", neutrino_suck_zapto, lockWeb);
	dyn_resolver->addDyn("GET", "/control/getonidsid", neutrino_suck_getonidsid, lockWeb);
	dyn_resolver->addDyn("GET", "/control/channellist", neutrino_suck_getchannellist, lockWeb);
#endif
	ezapMountInitializeDyn(dyn_resolver, lockWeb);
}


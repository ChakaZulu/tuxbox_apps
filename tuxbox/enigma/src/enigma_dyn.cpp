#include <enigma_dyn.h>

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

using namespace std;

#define TEMPLATE_DIR DATADIR+eString("/enigma/templates/")
#define CHARSETMETA "<META http-equiv=Content-Type content=\"text/html; charset=UTF-8\">\n"

#define BLUE "#12259E"
#define RED "#CB0303"
#define GREEN "#1FCB12"
#define YELLOW "#F5FF3C"
#define LIGHTGREY "#F4F4EC"
#define DARKGREY "#D9E0E7"
#define LEFTNAVICOLOR "#D9E0E7"
#define TOPNAVICOLOR "#D9E0E7"
#define OCKER "#FFCC33"

#define WEBXFACEVERSION "0.8"

extern eString getRight(const eString&, char); // implemented in timer.cpp
extern eString getLeft(const eString&, char);  // implemented in timer.cpp
extern bool onSameTP(const eServiceReferenceDVB& ref1, const eServiceReferenceDVB &ref2); // implemented in timer.cpp

eString getAttribute(eString filename, eString attribute)
{
	eString result = "&nbsp;";

	ifstream infile(filename.c_str());
	if (infile)
	{
		eString buffer;
		while (getline(infile, buffer, '\n'))
		{
			if (buffer.find(attribute + "=") == 0)
			{
				result = getRight(buffer, '=');
				if (result == "")
					result = "&nbsp;";
				break;
			}
		}
	}
	return result;
}

eString readFile(eString filename)
{
	eString result;
	eString line;

	ifstream infile(filename.c_str());
	if (infile)
		while (getline(infile, line, '\n'))
			result += line + "\n";

	return result;
}

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

eString button(int width, eString buttonText, eString buttonColor, eString buttonRef)
{
	eString ref1, ref2;

	std::stringstream result;
	if (buttonRef.find("javascript") == eString::npos)
	{
		ref1 = "\"self.location.href='";
		ref2 = "'\"";
	}
	result << "<button name=\"" << buttonText << "\""
		"type=\"button\" style='width: " << width <<
		"px; height: 22px; background-color: " << buttonColor <<
		"' value=\"" << buttonText <<
		"\" onclick=" << ref1 << buttonRef <<
		ref2 << "><span class=\"button\">" << buttonText <<
		"</span></button>";
	return result.str();
}

eString getTitle(eString title)
{
	std::stringstream result;
	result << "<span class=\"title\" style=\"width: 100%; background-color: " << LIGHTGREY << "\">"
		<< title
		<< "</span><br><br>";
	return result.str();
}

eString getMsgWindow(eString title, eString msg)
{
	eString result = readFile(TEMPLATE_DIR + "msgWindow.tmp");
	result.strReplace("#TITLE#", title);
	result.strReplace("#MSG#", msg);
	return result;
}

static int getHex(int c)
{
	c = toupper(c);
	c -= '0';
	if (c < 0)
		return -1;
	if (c > 9)
		c -= 'A' - '0' - 10;
	if (c > 0xF)
		return -1;
	return c;
}

eString httpUnescape(const eString &string)
{
	eString result;
	for (unsigned int i = 0; i < string.length(); ++i)
	{
		int c = string[i];
		switch (c)
		{
		case '%':
		{
			int value = '%';
			if ((i + 1) < string.length())
				value = getHex(string[++i]);
			if ((i + 1) < string.length())
			{
				value <<= 4;
				value += getHex(string[++i]);
			}
			result += value;
			break;
		}
		case '+':
			result += ' ';
			break;
		default:
			result += c;
			break;
		}
	}
	return result;
}

eString httpEscape(const eString &string)
{
	eString result;
	for (unsigned int i = 0; i < string.length(); ++i)
	{
		int c = string[i];
		int valid = 0;
		if ((c >= 'a') && (c <= 'z'))
			valid = 1;
		else if ((c >= 'A') && (c <= 'Z'))
			valid = 1;
		else if (c == ':')
			valid = 1;
		else if ((c >= '0') && (c <= '9'))
			valid = 1;

		if (valid)
			result += c;
		else
			result += eString().sprintf("%%%x", c);
	}
	return result;
}

std::map<eString, eString> getRequestOptions(eString opt)
{
	std::map<eString, eString> result;

	if (opt[0] == '?')
		opt = opt.mid(1);

	while (opt.length())
	{
		unsigned int e = opt.find("=");
		if (e == eString::npos)
			e = opt.length();
		unsigned int a = opt.find("&", e);
		if (a == eString::npos)
			a = opt.length();
		eString n = opt.left(e);

		unsigned int b = opt.find("&", e + 1);
		if (b == eString::npos)
			b = (unsigned)-1;
		eString r = httpUnescape(opt.mid(e + 1, b - e - 1));
		result.insert(std::pair<eString, eString>(n, r));
		opt = opt.mid(a + 1);
	}
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
	std::map<eString,eString> opt=getRequestOptions(opts);

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
		"  <title>enigma status</title>\n"
		"  <link rel=stylesheet type=\"text/css\" href=\"/webif.css\">\n"
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
	std::map<eString, eString> opt = getRequestOptions(opts);
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
	std::map<eString, eString> opt = getRequestOptions(opts);
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
				eZapStandby::getInstance()->wakeUp(0);
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

#ifndef DISABLE_FILE
static eString videocontrol(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts);
	eString command = opt["command"];
	if (command == "rewind")
	{
		// not working... different solution required
		eZapMain::getInstance()->startSkip(eZapMain::skipReverse);
	}
	else
	if (command == "forward")
	{
		// not working... different solution required
		eZapMain::getInstance()->startSkip(eZapMain::skipForward);
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

	return "<script language=\"javascript\">window.close();</script>";
}
#endif

static eString audio(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts);
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
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts);
	eString audioChannel = httpUnescape(opt["audio"]);

	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if ( sapi )
	{
		std::list<eDVBServiceController::audioStream> &astreams( sapi->audioStreams );
		std::list<eDVBServiceController::audioStream>::iterator it(astreams.begin());
		for (;it != astreams.end(); ++it )
			if ( it->text == audioChannel )
			{
				eDebug("blasel found... setpid");
				eServiceHandler *service=eServiceInterface::getInstance()->getService();
				if (service)
					service->setPID(it->pmtentry);
				break;
			}
			else
				eDebug("not found %s", it->text.c_str() );
	}

	return "<script language=\"javascript\">window.close();</script>";
}

static eString selectAudio(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";

	eString audioChannels;// = "<option>German</option><option>English</option>";
	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if ( sapi )
	{
		std::list<eDVBServiceController::audioStream> &astreams( sapi->audioStreams );
		for (std::list<eDVBServiceController::audioStream>::iterator it(astreams.begin())
			;it != astreams.end(); ++it )
		{
			audioChannels += "<option>";
			audioChannels += it->text;
			audioChannels += "</option>";
		}
	}
	else
		audioChannels = "<option>no audiodata avail</option>";

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
	std::map<eString,eString> opt = getRequestOptions(opts);
	eString mute = opt["mute"];
	eString volume = opt["volume"];

	content->local_header["Content-Type"]="text/html; charset=utf-8";

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

	return "<script language=\"javascript\">window.close();</script>";
}

static eString setVideo(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString,eString> opt = getRequestOptions(opts);
	eString video = opt["position"];

	content->local_header["Content-Type"]="text/html; charset=utf-8";

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

	return "<script language=\"javascript\">window.close();</script>";
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

eString filter_string(eString string)
{
	string.strReplace("\xc2\x86","");
	string.strReplace("\xc2\x87","");
	string.strReplace("\xc2\x8a"," ");
	return string;
}

static eString getLeftNavi(eString mode, eString path)
{
	eString result;
	if (mode.find("zap") == 0)
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
		if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000)
		{
			result += "<br>";
			result += button(110, "Screenshot", LEFTNAVICOLOR, "?mode=controlScreenShot");
		}
		result += "<br>";
		result += button(110, "Message", LEFTNAVICOLOR, "javascript:sendMessage2TV()");
		result += "<br>";
		result += button(110, "Plugins", LEFTNAVICOLOR, "?mode=controlPlugins");
		result += "<br>";
		result += button(110, "Timer List", LEFTNAVICOLOR, "?mode=controlTimerList");
	}
	else
	if (mode.find("config") == 0)
	{
		result += button(110, "HDD", LEFTNAVICOLOR, "?mode=configHDD");
		result += "<br>";
		result += button(110, "USB", LEFTNAVICOLOR, "?mode=configUSB");
		result += "<br>";
	}
	else
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
	result += button(100, "CONFIG", TOPNAVICOLOR, "?mode=config");
	result += button(100, "UPDATES", TOPNAVICOLOR, "?mode=updates");
	result += button(100, "HELP", TOPNAVICOLOR, "?mode=help");

	return result;
}

#ifndef DISABLE_FILE
extern int freeRecordSpace(void);  // implemented in enigma_main.cpp

static eString getDiskSpace(void)
{
	eString result;

	result += "Remaining Disk Space: ";
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
	else
		result += "unknown";

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

	vpid = (Decoder::current.vpid == -1) ? "none" : vpid.sprintf("0x%x", Decoder::current.vpid);
	result += "vpid: " + vpid;
	result += "&nbsp;<img src=\"squ.png\">&nbsp;";

	apid = (Decoder::current.apid == -1) ? "none" : apid.sprintf("0x%x", Decoder::current.apid);
	result += "<u><a href=\"/audio.m3u\">apid: " + apid + "</a></u>";

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

static eString getCurService()
{
	eString result = "n/a";

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		eService *current=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
		if (current)
			result = current->service_name.c_str();
	}
	return result;
}

static eString getEITC()
{
	eString result;

	EIT *eit = eDVB::getInstance()->getEIT();

	if (eit)
	{
		eString now_time = "&nbsp;", now_duration = "&nbsp;", now_text = "&nbsp;", now_longtext = "&nbsp;";
		eString next_time = "&nbsp;", next_duration = "&nbsp;", next_text = "&nbsp;", next_longtext = "&nbsp;";

		int p = 0;

		for(ePtrList<EITEvent>::iterator event(eit->events); event != eit->events.end(); ++event)
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
						next_time=next_time.mid(10,6);
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
								now_longtext+=ss->item_description;
								break;
							case 1:
								next_longtext+=ss->item_description;
								break;
						}
					}
				}
				p++;
		 	}
		}

		if (now_time != "")
		{
			result = readFile(TEMPLATE_DIR + "eit.tmp");
			result.strReplace("#NOWT#", now_time);
			result.strReplace("#NOWD#", now_duration);
			result.strReplace("#NOWST#", now_text);
			result.strReplace("#NOWLT#", filter_string(now_longtext));
			result.strReplace("#NEXTT#", next_time);
			result.strReplace("#NEXTD#", next_duration);
			result.strReplace("#NEXTST#", next_text);
			result.strReplace("#NEXTLT#", filter_string(next_longtext));
		}
		else
		{
			result = "eit undefined";
		}
		eit->unlock();
	}
	else
	{
		result = "";
	}

	return result;
}

static eString getVolBar()
{
	std::stringstream result;
	int volume = atoi(getVolume().c_str());

	result << "<table cellspacing=\"0\" cellpadding=\"0\" border=\"0\">"
		"<tr>";

	for (int i = 1; i <= (volume / 10); i++)
	{
		result << "<td width=15 height=8><a class=\"volgreen\" href=\"javascript:setVol(" << i << ")\">"
			"<img src=\"trans.gif\" border=0></a></span></td>";
	}
	for (int i = (volume / 10) + 1; i <= 10; i++)
	{
		result << "<td width=15 height=8><a class=\"volnot\" href=\"javascript:setVol(" << i << ")\">"
			"<img src=\"trans.gif\" border=0></a></span></td>";
	}

	result << "<td>"
		"<a class=\"mute\" href=\"javascript:Mute(" << eAVSwitch::getInstance()->getMute() << ")\">";
	if (eAVSwitch::getInstance()->getMute())
		result << "<img src=\"speak_off.gif\" border=0></a>";
	else
		result << "<img src=\"speak_on.gif\" border=0></a>";
	result << "</td>"

		<< "</tr>"
		<< "</table>";
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

	result << "<table cellspacing=\"0\" cellpadding=\"0\" border=\"0\">"
		"<tr>";

	for (int i = 1; i <= videopos; i++)
	{
		result << "<td width=15 height=8><a class=\"vidblue\" href=\"javascript:setVid(" << i << ")\">"
			"<img src=\"trans.gif\" border=0></a></span></td>";
	}
	for (int i = videopos + 1; i <= 10; i++)
	{
		result << "<td width=15 height=8><a class=\"vidnot\" href=\"javascript:setVid(" << i << ")\">"
			"<img src=\"trans.gif\" border=0></a></span></td>";
	}

	result << "<td>&nbsp;&nbsp;-" << min << ":" << sec << "</td>";
	result << "</tr>"
		"</table>";
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

static eString deleteMovie(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts);
	eString sref;

	content->local_header["Content-Type"]="text/html; charset=utf-8";

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
	return "<script language=\"javascript\">window.close();</script>";
}

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
	int num;
public:
	eWebNavigatorListDirectory2(eString &result1, eString &result2, eString origpath, eString path, eServiceInterface &iface): result1(result1), result2(result2), origpath(origpath), path(path), iface(iface)
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
		result1 += "<tr bgcolor=\"";
		result1 += (num & 1) ? LIGHTGREY : DARKGREY;
		result1 += "\"><td width=50 align=center>";

		eString serviceRef = ref2string(e);
		if (!(e.flags & eServiceReference::isDirectory))
		{
			if (!e.path)
				result1 += button(50, "EPG", GREEN, "javascript:openEPG('" + serviceRef + "')");
			else 
			if (serviceRef.find("%2fhdd%2fmovie%2f") != eString::npos)
			{
				result1 += "<a href=\"javascript:deleteMovie('";
				result1 += serviceRef;
				result1 += "')\"><img src=\"trash.gif\" height=22 border=0></a>";
			}
			else
				result1 += "&#160;";
			result1 += "</td><td><a href=\'javascript:switchChannel(\"" + serviceRef + "\")\'>";
			eService *service = iface.addRef(e);
			if (service)
			{
				result1 += filter_string(service->service_name);
				iface.removeRef(e);
			}
		}
		else
		{
			int count = 0;
			countDVBServices bla(e, count);
			if (count)
				result1 += button(50, "EPG", GREEN, "javascript:openMultiEPG('" + serviceRef + "')");
			else
				result1 += "&#160;";
			result1 += eString("</td><td><a href=\"/")+ "?path=" + serviceRef + "\">";
			eService *service = iface.addRef(e);
			if (service)
			{
				result1 += filter_string(service->service_name);
				iface.removeRef(e);
			}
		}


		result1 += "</a>";
		result1 += "</td></tr>\n";
		num++;
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
			eString tmpFile = readFile(TEMPLATE_DIR + "zap.tmp");
			eWebNavigatorListDirectory2 navlist(result1, result2, path, tpath, *iface);
			Signal1<void, const eServiceReference&> signal;
			signal.connect(slot(navlist, &eWebNavigatorListDirectory2::addEntry));
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

static eString getZap(eString mode, eString path)
{
	eString result;
	eString zap_result;

#ifndef DISABLE_FILE
	if (path == ";4097:7:0:1:0:0:0:0:0:0:")
	{
		eString tmpFile = readFile(TEMPLATE_DIR + "videocontrols.tmp");
		tmpFile.strReplace("#VIDEOBAR#", getVideoBar());
		result += tmpFile;
	}
#endif
	zap_result += getZapContent(mode, path);
	result += getEITC();
	eString curService = filter_string(getCurService());
	if (curService == "n/a")
		curService = eZapMain::getInstance()->dvrActive() ? "Digital Video Recorder" : "&nbsp;";
	result.strReplace("#SERVICENAME#", curService);


	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();

	if (sapi && sapi->service)
	{
		eString buttons = button(100, "AUDIO", OCKER, "javascript:selectAudio()");
		buttons += button(100, "EPG", GREEN, "javascript:openEPG()");
		buttons += button(100, "Info", YELLOW, "javascript:openSI()");
#ifndef DISABLE_FILE
		buttons += button(100, "Record", RED, "javascript:DVRrecord('start')");
		buttons += button(100, "Stop", BLUE, "javascript:DVRrecord('stop')");
#endif
		result.strReplace("#OPS#", buttons);
	}
	else
		result.strReplace("#OPS#", "");

	result += zap_result;

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
#endif //DISABLE_FILE

#ifndef DISABLE_FILE
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
#endif

static eString getUSBInfo(void)
{
	std::stringstream result;
	eString usbStick = "none";
	system("cat /proc/scsi/usb-storage-0/0 > /tmp/usbstick.tmp");
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
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts);
	eString swapUSB = opt["swapusb"];
	eString swapUSBFile = opt["swapusbfile"];
	eString bootUSB = opt["bootUSB"];
	eString bootUSBImage = opt["bootusbimage"];

	return getMsgWindow("Info", "Function is not supported by installed flash image.");
//	return "<script language=\"javascript\">window.close();</script>";
}

static eString setConfigHDD(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts);
	eString swapHDD = opt["swaphdd"];
	eString swapHDDFile = opt["swaphddfile"];
	eString bootHDD = opt["bootHDD"];
	eString bootHDDImage = opt["boothddimage"];

	return getMsgWindow("Info", "Function is not supported by installed flash image.");
//	return "<script language=\"javascript\">window.close();</script>";
}

static eString msgWindow(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts);
	eString title = opt["title"];
	eString msg = opt["msg"];
	return getMsgWindow(title, msg);
}

static eString aboutDreambox(void)
{
	std::stringstream result;
	result << "<table border=0 cellspacing=0 cellpadding=0>";

	if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000)
		result << "<img src=\"dm7000.jpg\" width=\"650\" border=\"0\"><br>";

	result  << "<tr><td>Model:</td><td>" << eSystemInfo::getInstance()->getModel() << "</td></tr>"
		<< "<tr><td>Manufacturer:</td><td>" << eSystemInfo::getInstance()->getManufacturer() << "</td></tr>"
		<< "<tr><td>Processor:</td><td>" << eSystemInfo::getInstance()->getCPUInfo() << "</td></tr>";

#ifndef DISABLE_FILE
	result << getDiskInfo();
#endif
	result << getUSBInfo();
	result  << "<tr><td>Firmware:</td><td>" << firmwareLevel(getAttribute("/.version", "version")) << "</td></tr>"
		<< "<tr><td>Web Interface:</td><td>" << WEBXFACEVERSION << "</td></tr>"
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

		result << "<tr>";
		if (!repeating)
		{
			result  << "<td align=center><a href=\"javascript:deleteTimerEvent(\'"
				<< "ref=" << ref2string(se->service)
				<< "&ID=" << std::hex << se->event_id << std::dec
				<< "&start=" << se->time_begin
				<< "&duration=" << se->duration
				<< "\')\"><img src=\"trash.gif\" border=0 height=20></a></td>";

			result << "<td align=center><a href=\"javascript:editTimerEvent(\'"
				<< "ref=" << ref2string(se->service)
				<< "&start=" << se->time_begin
				<< "&duration=" << se->duration
				<< "&ID=" << std::hex << se->event_id << std::dec
				<< "&channel=" << channel
				<< "&descr=" << description
				<< "\')\"><img src=\"edit.gif\" border=0 height=20></a></td>";
		}
		else
			result << "<td>&nbsp;</td><td>&nbsp;</td>";

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
				<< std::setw(2) << startTime.tm_mon+1 << " - "
				<< std::setw(2) << startTime.tm_hour << ':'
				<< std::setw(2) << startTime.tm_min
				<< "</td><td>"
				<< std::setw(2) << endTime.tm_mday << '.'
				<< std::setw(2) << endTime.tm_mon+1 << " - ";
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
	
	// repeated timers
	count = 0;
	eTimerManager::getInstance()->forEachEntry(countTimer(count, true));
	if (count)
		tableBody = genTimerListTableBody(1);
	else
		tableBody = "<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>" + eString(_("No repeated timer events available")) + "</td></tr>";
	result.strReplace("#TIMER_REPEATED#", tableBody);
	
	// buttons
	result.strReplace("#BUTTONCLEANUP#", button(100, "Cleanup", BLUE, "javascript:cleanupTimerList()"));
	result.strReplace("#BUTTONCLEAR#", button(100, "Clear", RED, "javascript:clearTimerList()"));
	
	return result;
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

	if (access("/dev/grabber", R_OK) == 0)
	{
		system("cat /dev/grabber > /tmp/screenshot.bmp");

		FILE *bitstream = 0;
		int xres = 0, yres = 0, yres2 = 0, aspect = 0, winxres = 650, winyres = 0, rh = 0, rv = 0;
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
	else
		result = "Module grabber.o is required but not installed";

	return result;
}

static eString getContent(eString mode, eString path)
{
	eString result;
	
	if (mode == "zap")
	{
		result = getTitle("ZAP");
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
	if (mode == "configHDD")
	{
		result = getTitle("CONFIG: HDD");
		result += getConfigHDD();
	}
	else
#endif
	if (mode == "configUSB")
	{
		result = getTitle("CONFIG: USB");
		result += getConfigUSB();
	}
	else
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
		result = getTitle("HELP: Forums");
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
			result += "<img width=\"650\" src=\"/root/tmp/osdshot.png\" border=0>";
	}
	else
#ifndef DISABLE_LCD
	if (mode == "controlLCDShot")
	{
		result = getTitle("CONTROL: LCDShot");
		if (!getOSDShot("lcd"))
			result += "<img width=\"650\" src=\"/root/tmp/osdshot.png\" border=0>";
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
		result = getTitle("CONTROL: Timer List");
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

static eString getEITC2()
{
	eString now_time = "&nbsp;", now_duration = "&nbsp;", now_text = "&nbsp;",
		next_time = "&nbsp;", next_duration = "&nbsp;", next_text = "&nbsp;",
		result;

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
	result = readFile(TEMPLATE_DIR + "header.tmp");
	result.strReplace("#NOWT#", now_time);
	if (now_duration != "&nbsp;")
		if (atoi(now_duration.c_str()) < 1000)
			now_duration += " min";
	result.strReplace("#NOWD#", now_duration);
	result.strReplace("#NOWST#", now_text);
	result.strReplace("#NEXTT#", next_time);
	if (next_duration != "&nbsp;")
		if (atoi(next_duration.c_str()) < 1000)
			next_duration += " min";
	result.strReplace("#NEXTD#", next_duration);
	result.strReplace("#NEXTST#", next_text);
	result.strReplace("#VOLBAR#", getVolBar());
	eString curService = filter_string(getCurService());
	if (curService == "n/a")
		curService = "&nbsp;";
	result.strReplace("#SERVICENAME#", curService);
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

	const timeMap* evt=ref ?
		eEPGCache::getInstance()->getTimeMap((eServiceReferenceDVB&)ref)
			:
		eEPGCache::getInstance()->getTimeMap(sapi->service);

	if (!evt)
		return eString("epg not ready yet");

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
	return result;
}

class eMEPG: public Object
{
	int hours;
	int d_min;
	eString multiEPG;
	time_t start;
	time_t end;
	int tableWidth;
public:
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
				const timeMap* evt = eEPGCache::getInstance()->getTimeMap((eServiceReferenceDVB&)ref);
				if (evt)
				{
					int tablePos = 0;
					time_t tableTime = start;
					result << "<table width=" << tableWidth << " border=1 rules=all>"
					"<tr>"
					"<td width=200>" << filter_string(current->service_name) << "</td>";
					tablePos += 200;

					timeMap::const_iterator It;

					for(It=evt->begin(); It!= evt->end(); It++)
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
								ext_description += ((ExtendedEventDescriptor*)descriptor)->item_description;
							}
						}

						time_t eventStart = event.start_time;
						time_t eventEnd = event.start_time + event.duration;
						int eventDuration = 0;
						int colWidth = 0;
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
							eventDuration = event.duration;
						}
						else
						if ((eventStart > tableTime) && (eventStart < end))
						{
							eventDuration = eventStart - tableTime;
							colWidth = eventDuration / 60 * d_min;
							result << "<td width=" << colWidth << ">&nbsp;</td>";
							tableTime = eventStart;
							tablePos += colWidth;
							eventDuration = event.duration;
						}

						if ((eventDuration > 0) && (eventDuration < 15 * 60))
							eventDuration = 15 * 60;

						if (tableTime + eventDuration > end)
							eventDuration = end - tableTime;

						colWidth = eventDuration  / 60 * d_min;
						if (colWidth > 0)
						{
							result << "<td width=" << colWidth << ">";
#ifndef DISABLE_FILE
							result << "<a href=\"javascript:record('"
								<< "ref=" << ref2string(ref)
								<< "&ID=" << std::hex << event.event_id << std::dec
								<< "&start=" << event.start_time
								<< "&duration=" << event.duration
								<< "&descr=" << short_description
								<< "&channel=" << filter_string(current->service_name)
								<< "')\"><img src=\"kalarm.png\" border=0></a>"
								<< "&nbsp;&nbsp;";
#endif
							tm* t = localtime(&event.start_time);
							result << std::setfill('0')
								<< std::setw(2) << t->tm_mday << '.'
								<< std::setw(2) << t->tm_mon+1 << ". - "
								<< std::setw(2) << t->tm_hour << ':'
								<< std::setw(2) << t->tm_min << ' '
								<< " (" << event.duration / 60 << " min)"

								<< "<br><b>"

								<< "<a href=\'javascript:switchChannel(\"" << ref2string(ref) << "\")\'>"
								<< short_description
								<< "</a>"

								<< "</b><br>\n";

							if (eventDuration >= 15 * 60)
								result << filter_string(ext_description);

							result << "</td>";
							tablePos += colWidth;
							tableTime += eventDuration;
						}
					}
					if (tablePos < tableWidth)
						result << "<td width=" << tableWidth - tablePos << ">&nbsp;</td>";

					result << "</tr></table>";
				}

				multiEPG += result.str();
			}
		}
	}

	eMEPG(time_t start, const eServiceReference & bouquetRef)
		:hours(6)   // horizontally visible hours
		,d_min(10)  // distance on time scale for 1 minute
		,start(start)
		,end(start + hours * 3600)
		,tableWidth((end - start) / 60 * d_min + 200)
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

		result << "<table width=" << tableWidth << " border=1 rules=all>"
			"<tr>"
			"<td width=200>Channel</td>";

		for (time_t i = start; i < end; i += 15 * 60)
		{
			tm* t = localtime(&i);
			result << "<td width=" << d_min * 15 << ">"
				<< std::setfill('0')
				<< std::setw(2) << t->tm_mday << '.'
				<< std::setw(2) << t->tm_mon+1 << "."
				<< "<br>"
				<< std::setw(2) << t->tm_hour << ':'
				<< std::setw(2) << t->tm_min << ' '
				<< "</td>";
		}

		result << "</tr>"
			"</table>";
		return result.str();
	}
};

static eString getMultiEPG(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString>opt = getRequestOptions(opts);
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
	result << std::setfill('0');

	eService* current;

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString,eString> opt=getRequestOptions(opts);
	eString serviceRef = opt["ref"];

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "sapi is not available.";

	eServiceReference ref = (serviceRef == "undefined") ? sapi->service : string2ref(serviceRef);

	eDebug("[ENIGMA_DYN] getcurepg2: opts = %s, serviceRef = %s", opts.c_str(), serviceRef.c_str());

	current = eDVB::getInstance()->settings->getTransponders()->searchService(ref);

	if (!current)
		return "EPG is not yet ready.";

	const timeMap* evt = eEPGCache::getInstance()->getTimeMap((eServiceReferenceDVB&)ref);

	if (!evt)
		result << "EPG is not yet available.";
	else
	{
		timeMap::const_iterator It;

		for(It=evt->begin(); It!= evt->end(); It++)
		{
			EITEvent event(*It->second);
			for (ePtrList<Descriptor>::iterator d(event.descriptor); d != event.descriptor.end(); ++d)
			{
				Descriptor *descriptor=*d;
				if (descriptor->Tag() == DESCR_SHORT_EVENT)
				{
					tm* t = localtime(&event.start_time);
#ifndef DISABLE_FILE
					result << "<a href=\"javascript:record('"
						<< "ref=" << ref2string(ref)
						<< "&ID=" << std::hex << event.event_id << std::dec
						<< "&start=" << event.start_time
						<< "&duration=" << event.duration
						<< "&descr=" << ((ShortEventDescriptor*)descriptor)->event_name
						<< "&channel=" << filter_string(current->service_name)
						<< "')\"><img src=\"kalarm.png\" border=0></a>"
						<< "&nbsp;&nbsp;";
#endif
					result << std::setw(2) << t->tm_mday << '.'
						<< std::setw(2) << t->tm_mon+1 << ". - "
						<< std::setw(2) << t->tm_hour << ':'
						<< std::setw(2) << t->tm_min << ' '
						<< "<a href=\"javascript:EPGDetails('"
						<< "ref=" << ref2string(ref)
						<< "&ID=" << std::hex << event.event_id << std::dec
						<< "')\">"
						<< ((ShortEventDescriptor*)descriptor)->event_name
						<< "</a><br>\n";
				}
			}
		}
	}

	eString tmp2 = readFile(TEMPLATE_DIR + "epg.tmp");
	tmp2.strReplace("#CHANNEL#", filter_string(current->service_name));
	tmp2.strReplace("#BODY#", result.str());
	return tmp2;
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

	std::map<eString,eString> opt = getRequestOptions(opts);
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

static eString message(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	std::map<eString, eString> opts = getRequestOptions(opt);
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
	std::map<eString, eString> opts = getRequestOptions(opt);
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
		result = "<script language=\"javascript\">window.close();</script>";

	return result;
}

static eString stopPlugin(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	std::map<eString, eString> opts = getRequestOptions(opt);
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
		result = "<script language=\"javascript\">window.close();</script>";
		
	return result;
}

static eString xmessage(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	std::map<eString, eString> opts = getRequestOptions(opt);

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
	std::map<eString, eString> opts = getRequestOptions(opt);

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
	std::map<eString,eString> opt=getRequestOptions(opts);
	content->local_header["Content-Type"]="text/html; charset=utf-8";

	eString mode = opt["mode"];
	eString spath = opt["path"];

	eDebug("[ENIGMA_DYN] web_root: mode = %s, spath = %s", mode.c_str(), spath.c_str());

	if (!spath)
		spath = eServiceStructureHandler::getRoot(eServiceStructureHandler::modeTV).toString();
		//ref2string(eServiceReference(eServiceReference::idStructure, eServiceReference::isDirectory, 0));

	if (!mode)
		mode = "zap";

	result = readFile(TEMPLATE_DIR + "index.tmp");
	result.strReplace("#CONTENT#", getContent(mode, spath));
	result.strReplace("#HEADER#", getEITC2());
	result.strReplace("#TOPNAVI#", getTopNavi(mode, spath));
	result.strReplace("#LEFTNAVI#", getLeftNavi(mode, spath));
	if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000)
		result.strReplace("#TOPBALK#", "topbalk.png");
	else
	if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::dbox2Nokia)
		result.strReplace("#TOPBALK#", "topbalk2.png");
	else
	if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::dbox2Sagem)
		result.strReplace("#TOPBALK#", "topbalk3.png");
	else
//	if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::dbox2Philips)
		result.strReplace("#TOPBALK#", "topbalk4.png");

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
		std::map<eString,eString> opts=getRequestOptions(opt);
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
		std::map<eString,eString> opts=getRequestOptions(opt);
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
			dest+=s.toString();
			eServiceDVB *service = eTransponderList::getInstance()->searchService(s);
			if (service)
			{
				for(int i=0; i < (int)eServiceDVB::cacheMax; ++i)
				{
					int d=service->get((eServiceDVB::cacheID)i);
					if (d != -1)
						dest+=eString().sprintf(";%02d%04x", i, d);
				}
			}
			dest+='\n';
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

static eString neutrino_suck_getchannellist(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString channelstring;

	eTransponderList::getInstance()->forEachService(appendonidsidnamestr(channelstring));

	return channelstring;
}

static eString cleanupTimerList(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString result;
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	result += "<script language=\"javascript\">window.close();</script>";
	eTimerManager::getInstance()->cleanupEvents();
	eTimerManager::getInstance()->saveTimerList(); //not needed, but in case enigma crashes ;-)
	return result;
}

static eString clearTimerList(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString result;
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	result += "<script language=\"javascript\">window.close();</script>";
	eTimerManager::getInstance()->clearEvents();
	eTimerManager::getInstance()->saveTimerList(); //not needed, but in case enigma crashes ;-)
	return result;
}

static eString addTimerEvent(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts);
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

static eString deleteTimerEvent(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts);
	eString serviceRef = opt["ref"];
	eString eventID = opt["ID"];
	eString eventStartTime = opt["start"];
	eString eventDuration = opt["duration"];

	int eventid;
	sscanf(eventID.c_str(), "%x", &eventid);
	printf("[ENIGMA_DYN] deleteTimerEvent: serviceRef = %s, ID = %s, start = %s, duration = %s\n", serviceRef.c_str(), eventID.c_str(), eventStartTime.c_str(), eventDuration.c_str());

	EITEvent evt;
	evt.start_time = atoi(eventStartTime.c_str());
	evt.duration = atoi(eventDuration.c_str());
	evt.event_id = eventid;
	eServiceReference ref = string2ref(serviceRef);
	eTimerManager::getInstance()->deleteEventFromTimerList(&ref, &evt);
	eTimerManager::getInstance()->saveTimerList(); //not needed, but in case enigma crashes ;-)
	return "<script language=\"javascript\">window.close();</script>";
}

static eString genOptions(int start, int end, int delta, int selected)
{
	std::stringstream result;
	// printf("[GENOPTIONS] start = %d, end = %d, delta = %d, selected = %d\n", start, end, delta, selected);
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
	std::map<eString, eString> opt = getRequestOptions(opts);
	eString serviceRef = opt["ref"];
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

	int eventid;
	sscanf(eventID.c_str(), "%x", &eventid);
	// printf("[CHANGETIMER] start: %d.%d. - %d:%d, end: %d.%d. - %d:%d\n", start.tm_mday, start.tm_mon, start.tm_hour, start.tm_min,
	//								end.tm_mday, end.tm_mon, end.tm_hour, end.tm_min);

	EITEvent evt;
	evt.start_time = eventStartTime;
	evt.duration = duration;
	evt.event_id = eventid;
	printf("[CHANGETIMER] startTime = %d, startDuration = %d, eventID = %x\n", evt.start_time, evt.duration, evt.event_id);
	eServiceReference ref = string2ref(serviceRef);
	eTimerManager::getInstance()->modifyEventInTimerList(&ref, &evt, channel + "/" + description);
	eTimerManager::getInstance()->saveTimerList(); //not needed, but in case enigma crashes ;-)
	return "<script language=\"javascript\">window.close();</script>";
}

static eString editTimerEvent(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts);
	eString serviceRef = opt["ref"];
	eString eventID = opt["ID"];
	eString eventStartTime = opt["start"];
	eString eventDuration = opt["duration"];
	eString description = httpUnescape(opt["descr"]);
	eString channel = httpUnescape(opt["channel"]);

	time_t eventStart = atoi(eventStartTime.c_str());
	time_t eventEnd = eventStart + atoi(eventDuration.c_str());
	tm start = *localtime(&eventStart);
	tm end = *localtime(&eventEnd);

	// printf("[ENIGMA_DYN] editTimerEvent: serviceRef = %s, ID = %s, start = %s, duration = %s\n", serviceRef.c_str(), eventID.c_str(), eventStartTime.c_str(), eventDuration.c_str());

	eString result = readFile(TEMPLATE_DIR + "editTimerEvent.tmp");

	result.strReplace("#EVENTID#", eventID);
	result.strReplace("#SERVICEREF#", serviceRef);

	result.strReplace("#SDAYOPTS#", genOptions(1, 31, 1, start.tm_mday));
	result.strReplace("#SMONTHOPTS#", genOptions(1, 12, 1, start.tm_mon + 1));
	result.strReplace("#SHOUROPTS#", genOptions(0, 23, 1, start.tm_hour));
	result.strReplace("#SMINOPTS#", genOptions(0, 55, 5, (start.tm_min / 5) * 5));

	result.strReplace("#EDAYOPTS#", genOptions(1, 31, 1, end.tm_mday));
	result.strReplace("#EMONTHOPTS#", genOptions(1, 12, 1, end.tm_mon + 1));
	result.strReplace("#EHOUROPTS#", genOptions(0, 23, 1, end.tm_hour));
	result.strReplace("#EMINOPTS#", genOptions(0, 55, 5, (end.tm_min / 5) * 5));

	result.strReplace("#CHANNEL#", channel);
	result.strReplace("#DESCRIPTION#", description);
	eTimerManager::getInstance()->saveTimerList(); //not needed, but in case enigma crashes ;-)
	return result;
}

static eString EPGDetails(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;
	eService *current = NULL;
	eString ext_description;

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts);
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
						ext_description += ((ExtendedEventDescriptor*)*d)->item_description;
						eDebug("[ENIGMA_DYN] getEPGDetails: found extended description = %s", ext_description.c_str());
					}
				}
				delete event;
			}
		}
	}
	if (!ext_description)
		ext_description = _("No detailed description available");
	description = filter_string(description);
	ext_description = filter_string(ext_description);

	result = "<html>" + eString(CHARSETMETA) + "<head><title>EPG Details</title><link rel=\"stylesheet\" type=\"text/css\" href=\"/webif.css\"></head><body bgcolor=#ffffff>";
	result += "<span class=\"title\"><b>" + description + "</b></span>";
	result += "<p>";
	result += ext_description;
	result += "</body>";
	result += "</html>";

	return result;
}

void ezapInitializeDyn(eHTTPDynPathResolver *dyn_resolver)
{
	dyn_resolver->addDyn("GET", "/", web_root, true);
	dyn_resolver->addDyn("GET", NAVIGATOR_PATH, navigator, true);

	dyn_resolver->addDyn("GET", "/cgi-bin/ls", listDirectory);
	dyn_resolver->addDyn("GET", "/cgi-bin/mkdir", makeDirectory, true);
	dyn_resolver->addDyn("GET", "/cgi-bin/rmdir", removeDirectory, true);
	dyn_resolver->addDyn("GET", "/cgi-bin/rm", removeFile, true);
	dyn_resolver->addDyn("GET", "/cgi-bin/mv", moveFile, true);
	dyn_resolver->addDyn("GET", "/cgi-bin/ln", createSymlink, true);
#ifndef DISABLE_FILE
	dyn_resolver->addDyn("GET", "/cgi-bin/stop", stop);
	dyn_resolver->addDyn("GET", "/cgi-bin/pause", pause);
	dyn_resolver->addDyn("GET", "/cgi-bin/play", play);
	dyn_resolver->addDyn("GET", "/cgi-bin/record", record);
	dyn_resolver->addDyn("GET", "/cgi-bin/videocontrol", videocontrol);
#endif
	dyn_resolver->addDyn("GET", "/setVolume", setVolume);
	dyn_resolver->addDyn("GET", "/setVideo", setVideo);
	dyn_resolver->addDyn("GET", "/showTimerList", showTimerList, true);
	dyn_resolver->addDyn("GET", "/addTimerEvent", addTimerEvent, true);
	dyn_resolver->addDyn("GET", "/deleteTimerEvent", deleteTimerEvent, true);
	dyn_resolver->addDyn("GET", "/editTimerEvent", editTimerEvent, true);
	dyn_resolver->addDyn("GET", "/changeTimerEvent", changeTimerEvent, true);
	dyn_resolver->addDyn("GET", "/cleanupTimerList", cleanupTimerList, true);
	dyn_resolver->addDyn("GET", "/clearTimerList", clearTimerList, true);
	dyn_resolver->addDyn("GET", "/EPGDetails", EPGDetails);
	dyn_resolver->addDyn("GET", "/msgWindow", msgWindow);
	dyn_resolver->addDyn("GET", "/tvMessageWindow", tvMessageWindow);
	dyn_resolver->addDyn("GET", "/cgi-bin/status", doStatus);
	dyn_resolver->addDyn("GET", "/cgi-bin/switchService", switchService);
	dyn_resolver->addDyn("GET", "/cgi-bin/admin", admin);
	dyn_resolver->addDyn("GET", "/cgi-bin/audio", audio);
	dyn_resolver->addDyn("GET", "/cgi-bin/selectAudio", selectAudio);
	dyn_resolver->addDyn("GET", "/cgi-bin/setAudio", setAudio);
	dyn_resolver->addDyn("GET", "/cgi-bin/setConfigUSB", setConfigUSB);
	dyn_resolver->addDyn("GET", "/cgi-bin/setConfigHDD", setConfigHDD);
	dyn_resolver->addDyn("GET", "/cgi-bin/getPMT", getPMT);
	dyn_resolver->addDyn("GET", "/cgi-bin/getEIT", getEIT);
	dyn_resolver->addDyn("GET", "/cgi-bin/message", message);
	dyn_resolver->addDyn("GET", "/control/message", message);
	dyn_resolver->addDyn("GET", "/cgi-bin/xmessage", xmessage);

	dyn_resolver->addDyn("GET", "/audio.m3u", audiom3u);
	dyn_resolver->addDyn("GET", "/version", version);
	dyn_resolver->addDyn("GET", "/cgi-bin/getcurrentepg", getcurepg);
	dyn_resolver->addDyn("GET", "/getcurrentepg2", getcurepg2);
	dyn_resolver->addDyn("GET", "/getMultiEPG", getMultiEPG);
	dyn_resolver->addDyn("GET", "/cgi-bin/streaminfo", getstreaminfo);
	dyn_resolver->addDyn("GET", "/channels/getcurrent", channels_getcurrent);
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadSettings", reload_settings);
#ifndef DISABLE_FILE
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadRecordings", load_recordings);
	dyn_resolver->addDyn("GET", "/cgi-bin/saveRecordings", save_recordings);
	dyn_resolver->addDyn("GET", "/cgi-bin/deleteMovie", deleteMovie);
#endif
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadPlaylist", load_playlist);
	dyn_resolver->addDyn("GET", "/cgi-bin/savePlaylist", save_playlist);
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadUserBouquets", load_userBouquets);
	dyn_resolver->addDyn("GET", "/cgi-bin/saveUserBouquets", save_userBouquets);
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadTimerList", load_timerList);
	dyn_resolver->addDyn("GET", "/cgi-bin/saveTimerList", save_timerList);
	dyn_resolver->addDyn("GET", "/cgi-bin/startPlugin", startPlugin);
	dyn_resolver->addDyn("GET", "/cgi-bin/stopPlugin", stopPlugin);
	dyn_resolver->addDyn("GET", "/cgi-bin/osdshot", osdshot);
	dyn_resolver->addDyn("GET", "/cgi-bin/screenshot", osdshot); // for backward compatibility
	dyn_resolver->addDyn("GET", "/cgi-bin/currentService", getCurrentServiceRef);
	dyn_resolver->addDyn("GET", "/cgi-bin/currentTransponderServices", getTransponderServices);
#ifndef DISABLE_FILE
	dyn_resolver->addDyn("GET", "/cgi-bin/setFakeRecordingState", setFakeRecordingState);
#endif
	dyn_resolver->addDyn("GET", "/control/zapto", neutrino_suck_zapto);
	dyn_resolver->addDyn("GET", "/control/getonidsid", neutrino_suck_getonidsid);
	dyn_resolver->addDyn("GET", "/control/channellist", neutrino_suck_getchannellist);
}


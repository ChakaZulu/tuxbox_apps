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
#include <enigma_dyn.h>
#include <enigma_dyn_utils.h>
#include <enigma_dyn_mount.h>
#include <enigma_dyn_wap.h>
#include <enigma_dyn_conf.h>
#include <enigma_dyn_flash.h>
#include <enigma_dyn_rotor.h>
#include <enigma_dyn_xml.h>

using namespace std;

#define WEBIFVERSION "2.4.1"

#define KEYBOARDNORMAL 0
#define KEYBOARDVIDEO 1

int keyboardMode = KEYBOARDNORMAL;

int pdaScreen = 0;
int screenWidth = 1024;
eString lastTransponder;

eString playStatus = "Off";

int currentBouquet = 0;
int currentChannel = -1;

int zapMode = ZAPMODETV;
int zapSubMode = ZAPSUBMODEBOUQUETS;
eString zapSubModes[5] = {"Name", "Category", "Satellites", "Providers", "Bouquets"};

eString zap[5][5] =
{
	{"TV", "0:7:1:0:0:0:0:0:0:0:", /* Satellites */ "1:15:fffffffc:12:0:0:0:0:0:0:", /* Providers */ "1:15:ffffffff:12:ffffffff:0:0:0:0:0:", /* Bouquets */ "4097:7:0:6:0:0:0:0:0:0:"},
	{"Radio", "0:7:2:0:0:0:0:0:0:0:", /* Satellites */ "1:15:fffffffc:4:0:0:0:0:0:0:", /* Providers */ "1:15:ffffffff:4:ffffffff:0:0:0:0:0:", /* Bouquets */ "4097:7:0:4:0:0:0:0:0:0:"},
	{"Data", "0:7:6:0:0:0:0:0:0:0:", /* Satellites */ "1:15:fffffffc:ffffffe9:0:0:0:0:0:0:", /* Providers */ "1:15:ffffffff:ffffffe9:ffffffff:0:0:0:0:0:", /* Bouquets */ ""},
	{"Movies", "4097:7:0:1:0:0:0:0:0:0:", /* Satellites */ "", /* Providers */ "", /* Bouquets */ ""},
	{"Root", "2:47:0:0:0:0:/", /* Satellites */ "", /* Providers */ "", /* Bouquets */ ""}
};

extern bool onSameTP(const eServiceReferenceDVB& ref1, const eServiceReferenceDVB &ref2); // implemented in timer.cpp

eString removeBadChars(eString s)
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
		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000
			|| eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7020)
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

class PluginCollector
{
	std::stringstream& result;
public:
	explicit PluginCollector(std::stringstream &res)
		:result(res)
	{
	}
	bool operator() (ePlugin& plugin)
	{
		result  << "<tr><td width=\"100\">"
			<< button(100, "Start", GREEN, "javascript:startPlugin('" + plugin.cfgname+ "')", "#FFFFFF")
			 << "</td><td>"
			 << plugin.name
			 << "</td><td>"
			 << (plugin.desc ? plugin.desc : "(no description)")
			 << "</td><td>";
		return false; // must return false in order to continue for_each loop
	}
};

static eString getControlPlugins(void)
{
	std::stringstream result;
	result << "<table width=\"100%\" border=\"1\" cellspacing=\"0\" cellpadding=\"0\">";
	eZapPlugins plugins(-1);
	plugins.find();

	if (!plugins.list.getCount())
		result << "<tr><td>No plugins found.</td></tr>";
	else
		plugins.list.forEachEntry(PluginCollector(result));

	result << "</table>";
	result << "<br>";
	result << button(100, "Stop", RED, "javascript:stopPlugin()", "#FFFFFF");

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
	result += "<tr><td>Recording:</td><td>";
#ifndef DISABLE_FILE
		if (eZapMain::getInstance()->isRecording())
			result += "ON";
		else
#endif
			result += "OFF";
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

static bool playService(const eServiceReference &ref)
{
	// ignore locked service
	if (ref.isLocked() && eConfig::getInstance()->pLockActive())
		return false;
	eZapMain::getInstance()->playService(ref, eZapMain::psSetMode|eZapMain::psDontAdd);
	return true;
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

void tuneTransponder(eString transponder)
{
	unsigned int frequency, symbol_rate;
	int polarisation, fec, orbital_position, inversion;
	sscanf(transponder.c_str(), "%d:%d:%d:%d:%d:%d:", &frequency, &symbol_rate, &polarisation, &fec, &orbital_position, &inversion);

	// search for the right transponder...
	for (std::list<tpPacket>::iterator it3(eTransponderList::getInstance()->getNetworks().begin()); it3 != eTransponderList::getInstance()->getNetworks().end(); it3++)
	{
		if (it3->orbital_position == orbital_position)
		{
			// ok, we have the right satellite now...
			for (std::list<eTransponder>::iterator it(it3->possibleTransponders.begin()); it != it3->possibleTransponders.end(); it++)
			{
				if (it->satellite.frequency == frequency && it->satellite.symbol_rate == symbol_rate && it->satellite.polarisation == polarisation && it->satellite.fec == fec && it->satellite.inversion == inversion)
				{
					// and this should be the right transponder...
					it->tune();
					lastTransponder = transponder;
				}
			}
		}
	}
}

static eString admin(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString command = opt["command"];
	eString result =  "Unknown admin command. (valid commands are: shutdown, reboot, restart, standby, wakeup)";
	if (command == "shutdown")
	{
		if (eSystemInfo::getInstance()->canShutdown())
		{
			eZap::getInstance()->quit();
			result = "Shutdown initiated...";
		}
		else
		{
			result = "No shutdown function available for this box.";
		}
	}
	else
	if (command == "reboot")
	{
		eZap::getInstance()->quit(4);
		result = "Reboot initiated...";
	}
	else
	if (command == "restart")
	{
		eZap::getInstance()->quit(2);
		result = "Restart initiated...";
	}
	else
	if (command == "wakeup")
	{
		if (eZapStandby::getInstance())
		{
			eZapStandby::getInstance()->wakeUp(0);
			result = "Enigma is waking up...";
		}
		else
		{
			result = "Enigma doesn't sleep.";
		}
	}
	else
	if (command == "standby")
	{
		if (eZapStandby::getInstance())
		{
			result = "Enigma is already sleeping.";
		}
		else
		{
			eZapMain::getInstance()->gotoStandby();
			result = "Standby initiated...";
		}
	}

	return "<html>" + eString(CHARSETMETA) + "<head><title>" + command + "</title></head><body>" + result + "</body></html>";
}

#ifndef DISABLE_FILE
static eString videocontrol(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString command = opt["command"];
	if (command == "rewind")
	{
		eZapMain::getInstance()->startSkip(eZapMain::skipReverse);
	}
	else
	if (command == "forward")
	{
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

	return closeWindow(content, "", 500);
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
	sscanf(opt["language"].c_str(), "0x%04x", &apid);

	eString channel = opt["channel"];
	eAVSwitch::getInstance()->selectAudioChannel(atoi(channel.c_str()));

	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if (sapi)
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

	return WINDOWCLOSE;
}

eString getAudioChannels(void)
{
	eString result;
	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		std::list<eDVBServiceController::audioStream> &astreams(sapi->audioStreams);
		for (std::list<eDVBServiceController::audioStream>::iterator it(astreams.begin())
			;it != astreams.end(); ++it)
		{
			if (it->pmtentry->elementary_PID == Decoder::current.apid)
				result += eString().sprintf("<option selected value=\"0x%04x\">", it->pmtentry->elementary_PID);
			else
				result += eString().sprintf("<option value=\"0x%04x\">", it->pmtentry->elementary_PID);

			result += removeBadChars(it->text);
			result += "</option>";
		}
	}
	else
		result = "<option>none</option>";

	return result;
}

static eString selectAudio(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	content->local_header["Cache-Control"] = "no-cache";

	eString audioChannels = getAudioChannels();

	eString result = readFile(TEMPLATE_DIR + "audioSelection.tmp");
	result.strReplace("#LANGUAGES#", audioChannels);

	int channel = eAVSwitch::getInstance()->getAudioChannel();
	result.strReplace(eString().sprintf("#%d#", channel).c_str(), eString("checked"));
	result.strReplace("#0#", "");
	result.strReplace("#1#", "");
	result.strReplace("#2#", "");

	return result;
}

static eString audioChannels(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	return getAudioChannels();
}

eString getCurrentSubChannel(eString curServiceRef)
{
	eString subChannel;
	if (curServiceRef)
	{
		eString s1 = curServiceRef; int pos; eString nspace;
		for (int i = 0; i < 7 && s1.find(":") != eString::npos; i++)
		{
			pos = s1.find(":");
			nspace = s1.substr(0, pos);
			s1 = s1.substr(pos + 1);
		}
		EIT *eit = eDVB::getInstance()->getEIT();
		if (eit)
		{
			int p=0;
			for (ePtrList<EITEvent>::iterator i(eit->events); i != eit->events.end(); ++i)
			{
				EITEvent *event=*i;
				if ((event->running_status>=2) || ((!p) && (!event->running_status)))
				{
					for (ePtrList<Descriptor>::iterator d(event->descriptor); d != event->descriptor.end(); ++d)
					{
						if (d->Tag() == DESCR_LINKAGE)
						{
							LinkageDescriptor *ld =(LinkageDescriptor *)*d;
							if (ld->linkage_type == 0xB0) //subchannel
							{
								eString subService((char*)ld->private_data, ld->priv_len);
								eString subServiceRef = "1:0:7:" + eString().sprintf("%x", ld->service_id) + ":" + eString().sprintf("%x", ld->transport_stream_id) + ":" + eString().sprintf("%x", ld->original_network_id) + ":"
									+ eString(nspace) + ":0:0:0:";
								if (subServiceRef == curServiceRef)
									subChannel = removeBadChars(subService);
							}
						}
					}
				}
				++p;
			}
			eit->unlock();
		}
	}
	return subChannel;
}

eString getSubChannels(void)
{
	eString result;
	eString curServiceRef = ref2string(eServiceInterface::getInstance()->service);
	if (curServiceRef)
	{
		eString s1 = curServiceRef; int pos; eString nspace;
		for (int i = 0; i < 7 && s1.find(":") != eString::npos; i++)
		{
			pos = s1.find(":");
			nspace = s1.substr(0, pos);
			s1 = s1.substr(pos + 1);
		}
		EIT *eit = eDVB::getInstance()->getEIT();
		if (eit)
		{
			int p=0;
			for (ePtrList<EITEvent>::iterator i(eit->events); i != eit->events.end(); ++i)
			{
				EITEvent *event=*i;
				if ((event->running_status>=2) || ((!p) && (!event->running_status)))
				{
					for (ePtrList<Descriptor>::iterator d(event->descriptor); d != event->descriptor.end(); ++d)
					{
						if (d->Tag() == DESCR_LINKAGE)
						{
							LinkageDescriptor *ld =(LinkageDescriptor *)*d;
							if (ld->linkage_type == 0xB0) //subchannel
							{
								eString subService((char*)ld->private_data, ld->priv_len);
								eString subServiceRef = "1:0:7:" + eString().sprintf("%x", ld->service_id) + ":" + eString().sprintf("%x", ld->transport_stream_id) + ":" + eString().sprintf("%x", ld->original_network_id) + ":"
									+ eString(nspace) + ":0:0:0:";
								if (subServiceRef == curServiceRef)
									result += "<option selected value=\"" + subServiceRef + "\">";
								else
									result += "<option value=\"" + subServiceRef + "\">";
								result += removeBadChars(subService);
								result += "</option>";
							}
						}
					}
				}
				++p;
			}
			eit->unlock();
		}
	}
	if (!result)
		result = "<option>none</option>";

	return result;
}

static eString selectSubChannel(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	content->local_header["Cache-Control"] = "no-cache";
	eString subChannels = getSubChannels();

	eString result = readFile(TEMPLATE_DIR + "subChannelSelection.tmp");
	result.strReplace("#SUBCHANS#", subChannels);

	return result;
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

eString getCurService(void)
{
	eString result;

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
	eString result;

	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();

	if (sapi && sapi->service)
	{
		result = button(100, "Audio", OCKER, "javascript:selectAudio()");
		result += button(100, "Video", OCKER, "javascript:selectSubChannel()");

		if (getCurService() || getCurrentSubChannel(ref2string(sapi->service)))
		{
			result += button(100, "EPG", GREEN, "javascript:openEPG()");
			if (pdaScreen == 0)
			{
				result += button(100, "Info", PINK, "javascript:openChannelInfo()");
				result += button(100, "Stream Info", YELLOW, "javascript:openSI()");
			}
#ifndef DISABLE_FILE
			if (eZapMain::getInstance()->isRecording())
				result += button(100, "Stop", BLUE, "javascript:DVRrecord('stop')");
			else
				result += button(100, "Record", RED, "javascript:DVRrecord('start')");
#endif
		}
	}

	return result;
}

static eString getLeftNavi(eString mode, bool javascript)
{
	eString result;
	eString pre, post;

	if (javascript)
	{
		pre = "javascript:leftnavi('";
		post = "')";
	}

	if (mode.find("zap") == 0)
	{
		if (pdaScreen == 0)
		{
			result += "<span class=\"zapnavi\">";
			result += button(110, "TV", RED, pre + "?mode=zap&zapmode=" + eString().sprintf("%d", ZAPMODETV) + "&zapsubmode=" + eString().sprintf("%d", ZAPSUBMODEBOUQUETS) + post, "#FFFFFF");
			result += "<br>";
			result += button(110, "Radio", GREEN, pre + "?mode=zap&zapmode=" + eString().sprintf("%d", ZAPMODERADIO) + "&zapsubmode=" + eString().sprintf("%d", ZAPSUBMODEBOUQUETS) + post, "#FFFFFF");
			result += "<br>";
			result += button(110, "Data", BLUE, pre + "?mode=zap&zapmode=" + eString().sprintf("%d", ZAPMODEDATA) + "&zapsubmode=" + eString().sprintf("%d", ZAPSUBMODESATELLITES) + post, "#FFFFFF");
			result += "<br>";
#ifndef DISABLE_FILE
			result += button(110, "Movies", OCKER, pre + "?mode=zap&zapmode=" + eString().sprintf("%d", ZAPMODERECORDINGS) + "&zapsubmode=" + eString().sprintf("%d", ZAPSUBMODECATEGORY) + post, "#FFFFFF");
			result += "<br>";
#endif
			result += button(110, "Root", PINK, pre + "?mode=zap&zapmode=" + eString().sprintf("%d", ZAPMODEROOT) + "&zapsubmode=" + eString().sprintf("%d", ZAPSUBMODECATEGORY) + post, "#FFFFFF");
			result += "</span>";
			result += "<br><br>";
			if (zap[zapMode][ZAPSUBMODESATELLITES])
			{
				result += button(110, "Satellites", LEFTNAVICOLOR, pre + "?mode=zap&zapmode=" + eString().sprintf("%d", zapMode) + "&zapsubmode=" + eString().sprintf("%d", ZAPSUBMODESATELLITES) + post);
				result += "<br>";
			}
			if (zap[zapMode][ZAPSUBMODEPROVIDERS])
			{
				result += button(110, "Providers", LEFTNAVICOLOR, pre + "?mode=zap&zapmode=" + eString().sprintf("%d", zapMode) + "&zapsubmode=" + eString().sprintf("%d", ZAPSUBMODEPROVIDERS) + post);
				result += "<br>";
			}
			if (zap[zapMode][ZAPSUBMODEBOUQUETS])
			{
				result += button(110, "Bouquets", LEFTNAVICOLOR, pre + "?mode=zap&zapmode=" + eString().sprintf("%d", zapMode) + "&zapsubmode=" + eString().sprintf("%d", ZAPSUBMODEBOUQUETS) + post);
			}
		}
		else
		{
			result += button(110, "TV", LEFTNAVICOLOR, "?mode=zap&path=0:7:1:0:0:0:0:0:0:0:");
			result += "<br>";
			result += button(110, "Radio", LEFTNAVICOLOR, "?mode=zap&path=0:7:2:0:0:0:0:0:0:0:");
			result += "<br>";
			result += button(110, "Data", LEFTNAVICOLOR, "?mode=zap&path=0:7:6:0:0:0:0:0:0:0:");
#ifndef DISABLE_FILE
			result += "<br>";
			result += button(110, "Root", LEFTNAVICOLOR, "?mode=zap&path=2:47:0:0:0:0:%2f");
			result += "<br>";
			result += button(110, "Movies", LEFTNAVICOLOR, "?mode=zap&path=4097:7:0:1:0:0:0:0:0:0:");
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
		result += button(110, "OSDshot", LEFTNAVICOLOR, pre + "?mode=controlFBShot" + post);
#ifndef DISABLE_LCD
		result += "<br>";
		result += button(110, "LCDshot", LEFTNAVICOLOR, pre + "?mode=controlLCDShot" + post);
#endif
		if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000
			|| eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7020)
		{
			result += "<br>";
			result += button(110, "Screenshot", LEFTNAVICOLOR, pre + "?mode=controlScreenShot" + post);
		}
		result += "<br>";
		result += button(110, "Message", LEFTNAVICOLOR, "javascript:sendMessage2TV()");
		result += "<br>";
		result += button(110, "Plugins", LEFTNAVICOLOR, pre + "?mode=controlPlugins" + post);
		result += "<br>";
		result += button(110, "Timer", LEFTNAVICOLOR, pre + "?mode=controlTimerList" + post);
#ifndef DISABLE_FILE
		result += "<br>";
		result += button(110, "Recover Movies", LEFTNAVICOLOR, "javascript:recoverMovies()");
#endif
		result += "<br>";
		result += button(110, "Logging", LEFTNAVICOLOR, "javascript:logging()");
		result += "<br>";
		result += button(110, "Satfinder", LEFTNAVICOLOR, pre + "?mode=controlSatFinder" + post);
		if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000
			|| eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7020)
		{
			result += "<br>";
			result += button(110, "Remote Control", LEFTNAVICOLOR, "javascript:remoteControl('dreambox')");
		}
		else
		if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::dbox2Nokia
		 || eSystemInfo::getInstance()->getHwType() == eSystemInfo::dbox2Sagem
		 || eSystemInfo::getInstance()->getHwType() == eSystemInfo::dbox2Philips)
		{
			result += "<br>";
			result += button(110, "Remote Control", LEFTNAVICOLOR, "javascript:remoteControl('dbox2')");
		}
	}
	else

	if (mode.find("config") == 0)
	{
#ifndef DISABLE_FILE
#ifdef ENABLE_DYN_CONF
		result += button(110, "Mount Manager", LEFTNAVICOLOR, pre + "?mode=configMountMgr" + post);
		result += "<br>";
#endif
#endif
#ifdef ENABLE_DYN_FLASH
		result += button(110, "Flash Manager", LEFTNAVICOLOR, pre + "?mode=configFlashMgr" + post);
		result += "<br>";
#endif
#ifndef DISABLE_FILE
#ifdef ENABLE_DYN_CONF
		result += button(110, "Swap File", LEFTNAVICOLOR, pre + "?mode=configSwapFile" + post);
		result += "<br>";
		result += button(110, "Settings", LEFTNAVICOLOR, pre + "?mode=configSettings" + post);
		result += "<br>";
#endif
#endif
#ifdef ENABLE_DYN_ROTOR
		result += button(110, "Rotor", LEFTNAVICOLOR, pre + "?mode=configRotor" + post);
#endif
	}
	else
	if (mode.find("updates") == 0)
	{
		result += button(110, "Internet", LEFTNAVICOLOR, pre + "?mode=updatesInternet" + post);
	}
	else
	if (mode.find("help") == 0)
	{
		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000
			|| eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7020)
		{
			result += button(110, "DMM Sites", LEFTNAVICOLOR, pre + "?mode=helpDMMSites" + post);
			result += "<br>";
			result += button(110, "Other Sites", LEFTNAVICOLOR, pre + "?mode=helpOtherSites" + post);
			result += "<br>";
		}
		result += button(110, "Boards", LEFTNAVICOLOR, pre + "?mode=helpForums" + post);
	}

	result += "&nbsp;";
	return result;
}

static eString getTopNavi(bool javascript)
{
	eString result;
	eString pre, post;

	if (javascript)
	{
		pre = "javascript:topnavi('";
		post = "')";
	}

	result += button(100, "ZAP", TOPNAVICOLOR, pre + "?mode=zap" + post);
	result += button(100, "CONTROL", TOPNAVICOLOR, pre + "?mode=control" + post);
	if (pdaScreen == 0)
	{
#if ENABLE_DYN_MOUNT || ENABLE_DYN_CONF || ENABLE_DYN_FLASH
		result += button(100, "CONFIG", TOPNAVICOLOR, pre + "?mode=config" + post);
#endif
		result += button(100, "UPDATES", TOPNAVICOLOR, pre + "?mode=updates" + post);
	}
	result += button(100, "HELP", TOPNAVICOLOR, pre + "?mode=help" + post);

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

static eString channels_getcurrent(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/plain; charset=utf-8";

	if (eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI())
		if (eServiceDVB *current=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service))
			return current->service_name.c_str();

	return "-1";
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
		eAVSwitch::getInstance()->changeVolume(1, 63 - vol);
	}

	return closeWindow(content, "", 500);
}

static eString setVideo(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	eString video = opt["position"];
	if (video)
	{
		int vid = atoi(video.c_str()); // 1..20

		eServiceHandler *handler=eServiceInterface::getInstance()->getService();
		if (handler)
		{
			int total = handler->getPosition(eServiceHandler::posQueryLength);
			int current = handler->getPosition(eServiceHandler::posQueryCurrent);
			int skipTime = ((total * vid) / 20) - current;

			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSeekBegin));
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSkip, skipTime * 380));
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSeekEnd));
		}
	}

	return closeWindow(content, "", 500);
}

eString getIP()
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

static eString getVolBar()
{
	std::stringstream result;
	int volume = (eAVSwitch::getInstance()->getMute()) ? 0 : 63 - eAVSwitch::getInstance()->getVolume();

	for (int i = 9; i <= 63; i+=6)
	{
		result << "<td width=\"15\" height=\"8\">"
			"<a href=\"javascript:setVol(" << i << ")\">";
		if (i <= volume)
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

	result << "<a href=\"javascript:toggleMute(" << eAVSwitch::getInstance()->getMute() << ")\">";
	if (eAVSwitch::getInstance()->getMute())
		result << "<img src=\"speak_off.gif\" border=0>";
	else
		result << "<img src=\"speak_on.gif\" border=0>";
	result << "</a>";

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
	return closeWindow(content, "Please wait...", 2000);
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

		// sorry.. at moment we dont show any directory.. or locked service in webif
		if (ref.isLocked() && eConfig::getInstance()->pLockActive())
			return;

		++count;
	}
};

class eWebNavigatorSearchService: public Object
{
	eString &result;
	eString searched_service;
	eServiceInterface &iface;
public:
	eWebNavigatorSearchService(eString &result, eString searched_service, eServiceInterface &iface): result(result), searched_service(searched_service), iface(iface)
	{
		eDebug("[eWebNavigatorSearchService] searched_service: %s", searched_service.c_str());
	}

	void addEntry(const eServiceReference &e)
	{
		// sorry.. at moment we dont show any directory.. or locked service in webif
		if (e.isLocked() && eConfig::getInstance()->pLockActive())
			return;

		eService *service = iface.addRef(e);
		if (service)
		{
			eDebug("[eWebNavigatorSearchService] searched_service: %s, service: %s\n", searched_service.c_str(), filter_string(service->service_name).c_str());
			if ((filter_string(service->service_name).upper() == searched_service.upper()) && !result)
			{
				result = ref2string(e);
				eDebug("[eWebNavigatorSearchService] service found: %s\n", searched_service.c_str());
			}
			iface.removeRef(e);
		}
	}
};

class myService
{
public:
	eString serviceRef;
	eString serviceName;
	myService(eString sref, eString sname)
	{
//		eDebug("new Service: %s - %s", sref.c_str(), sname.c_str());
		serviceRef = sref;
		serviceName = sname;
	};
	~myService() {};
	bool operator < (const myService &a) const {return serviceName < a.serviceName;}
};

void sortServices(bool sortList, std::list <myService> &myList, eString &serviceRefList, eString &serviceList)
{
	std::list <myService>::iterator myIt;
	eString serviceRef, serviceName;

	eDebug("[ENIGMA_DYN] start sorting...");

	if (sortList)
		myList.sort();

	serviceRefList = "";
	serviceList = "";

	for (myIt = myList.begin(); myIt != myList.end(); ++myIt)
	{
		serviceRefList += "\"" + myIt->serviceRef + "\", ";
		serviceList += "\"" + myIt->serviceName + "\", ";
//		eDebug("[ENIGMA_DYN] adding: %s - %s", myIt->serviceRef.c_str(), myIt->serviceName.c_str());
	}

	eDebug("[ENIGMA_DYN] sorting done.");
}

class eWebNavigatorListDirectory: public Object
{
	eString &result;
	eString path;
	eServiceInterface &iface;
	int num;
public:
	eWebNavigatorListDirectory(eString &result, eString path, eServiceInterface &iface): result(result), path(path), iface(iface)
	{
		eDebug("path: %s", path.c_str());
		num = 0;
	}
	void addEntry(const eServiceReference &e)
	{
		// sorry.. at moment we dont show any directory.. or locked service in webif
		if (e.isLocked() && eConfig::getInstance()->pLockActive())
			return;
#ifndef DISABLE_FILE
		if (eDVB::getInstance()->recorder && !e.path && !e.flags)
		{
			if (!onSameTP(eDVB::getInstance()->recorder->recRef,(eServiceReferenceDVB&)e))
					 return;
		}
#endif
		result += "<tr bgcolor=\"";
		result += (num & 1) ? LIGHTGREY : DARKGREY;
		result += "\"><td width=30 align=center>";

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
				result += "')\"><img src=\"trash.gif\" height=12 border=0></a>";
			}
			else
				result += "&#160;";
		}
		else
		{
			int count = 0;
			countDVBServices bla(e, count);
			if (count)
				result += button(50, "EPG", GREEN, "javascript:openMultiEPG('" + serviceRef + "')");
			else
				result += "&#160;";
		}
		result += eString("</td><td><a href=\"/")+ "?mode=zap&path=" + serviceRef + "\">";

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
	std::list <myService> &myList;
	eString path;
	eServiceInterface &iface;
	bool addEPG;
public:
	eWebNavigatorListDirectory2(std::list <myService> &myList, eString path, eServiceInterface &iface, bool addEPG): myList(myList), path(path), iface(iface), addEPG(addEPG)
	{
		eDebug("[eWebNavigatorListDirectory2:] path: %s", path.c_str());
	}
	void addEntry(const eServiceReference &e)
	{
		// sorry.. at moment we dont show any directory.. or locked service in webif
		if (e.isLocked() && eConfig::getInstance()->pLockActive())
			return;
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
				for (It = evt->begin(); (It != evt->end() && !short_description); ++It)
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

			eString tmp = filter_string(service->service_name);
			if (short_description && addEPG)
				tmp = tmp + " - " + event_start + " (" + event_duration + ") " + filter_string(short_description);
			tmp.strReplace("\"", "'");

			if (!(e.data[0] == -1 && e.data[2] != (int)0xFFFFFFFF))
				myList.push_back(myService(ref2string(e), tmp));
			iface.removeRef(e);
		}
	}
};

static eString getZapContent(eString mode, eString path)
{
	eString result;

	eServiceReference current_service = string2ref(path);
	eServiceInterface *iface = eServiceInterface::getInstance();

	if (!(current_service.flags&eServiceReference::isDirectory))	// is playable
	{
		playService(current_service);
		result = "";
	}
	else
	{
		eWebNavigatorListDirectory navlist(result, path, *iface);
		Signal1<void, const eServiceReference&> signal;
		signal.connect(slot(navlist, &eWebNavigatorListDirectory::addEntry));
		result += "<table width=\"100%\" cellspacing=\"2\" cellpadding=\"1\" border=\"0\">\n";
		iface->enterDirectory(current_service, signal);
		result += "</table>\n";
		eDebug("entered");
		iface->leaveDirectory(current_service);
		eDebug("exited");
	}

	return result;
}

static eString getZapContent2(eString mode, eString path, int depth, bool addEPG, bool sortList)
{
	std::list <myService> myList;
	eString result, result1, result2;
	eString bouquets, bouquetrefs, channels, channelrefs;
	std::stringstream tmp;

	eServiceReference current_service = string2ref(path);
	eServiceInterface *iface = eServiceInterface::getInstance();

	if (!(current_service.flags&eServiceReference::isDirectory))	// is playable
	{
		playService(current_service);
		result = "";
	}
	else
	{
		// first pass thru is to get all user bouquets
		myList.sort();
		eWebNavigatorListDirectory2 navlist(myList, path, *iface, addEPG);
		Signal1<void, const eServiceReference&> signal;
		signal.connect(slot(navlist, &eWebNavigatorListDirectory2::addEntry));
		iface->enterDirectory(current_service, signal);
		eDebug("entered");
		iface->leaveDirectory(current_service);
		eDebug("exited");

		sortServices(sortList, myList, result1, result2);

		tmp.str(result1.left(result1.length() - 1));
		bouquetrefs = result1.left(result1.length() - 2);
		bouquets = result2.left(result2.length() - 2);
		if (depth > 1)
		{
			// go thru all bouquets to get the channels
			int i = 0;
			while (tmp)
			{
				result1 = ""; result2 =""; path = "";
				tmp >> path;
				if (path)
				{
					path = path.mid(1, path.length() - 3);
					eServiceReference current_service = string2ref(path);

					myList.clear();
					eWebNavigatorListDirectory2 navlist(myList, path, *iface, addEPG);
					Signal1<void, const eServiceReference&> signal;
					signal.connect(slot(navlist, &eWebNavigatorListDirectory2::addEntry));

					channels += "channels[";
					channels += eString().sprintf("%d", i);
					channels += "] = new Array(";
					channelrefs += "channelRefs[";
					channelrefs += eString().sprintf("%d", i);
					channelrefs += "] = new Array(";

					iface->enterDirectory(current_service, signal);
					eDebug("entered");
					iface->leaveDirectory(current_service);
					eDebug("exited");

					sortServices(sortList, myList, result1, result2);

					channels += result2.left(result2.length() - 2);
					channels += ");";
					channelrefs += result1.left(result1.length() - 2);
					channelrefs += ");";

					i++;
				}
			}
		}
		else
		{
			channels = "channels[0] = new Array(" + bouquets + ");";
			channelrefs = "channelRefs[0] = new Array(" + bouquetrefs + ");";
			bouquets = "\"Dummy bouquet\"";
			bouquetrefs = "\"Dummy bouquet ref\"";
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
		result.strReplace("#ZAPMODE#", eString().sprintf("%d", zapMode));
		result.strReplace("#ZAPSUBMODE#", eString().sprintf("%d", zapSubMode));
	}

	return result;
}

eString getEITC(eString);

static eString getZap(eString mode, eString path)
{
	eString result, tmp;
	int selsize = 0;

	if (pdaScreen == 0)
	{
#ifndef DISABLE_FILE
		if (zapMode == ZAPMODERECORDINGS) // recordings
		{
			result = readFile(TEMPLATE_DIR + "movies.tmp");
			result.strReplace("#ZAPDATA#", getZapContent2(mode, path, 1, false, false));
			selsize = (screenWidth > 1024) ? 25 : 10;
			result.strReplace("#BUTTON#", button(100, "Delete", RED, "javascript:deleteMovie()", "#FFFFFF"));
		}
		else
#endif
		if (zapMode == ZAPMODEROOT) // root
		{
			result = readFile(TEMPLATE_DIR + "root.tmp");
			eString tmp = getZapContent2(mode, path, 1, false, false);
			if (tmp)
			{
				result.strReplace("#ZAPDATA#", tmp);
				selsize = (screenWidth > 1024) ? 25 : 10;
				result.strReplace("#BUTTON#", "");
			}
			else
				result = "";
		}
		else
		{
			result = readFile(TEMPLATE_DIR + "zap.tmp");
			selsize = (screenWidth > 1024) ? 30 : 15;
			bool sortList = (zapSubMode ==  ZAPSUBMODESATELLITES || zapSubMode == ZAPSUBMODEPROVIDERS);
			result.strReplace("#ZAPDATA#", getZapContent2(mode, path, 2, true, sortList));
		}
		result.strReplace("#SELSIZE#", eString().sprintf("%d", selsize));
	}
	else
	{
		result = getEITC(readFile(TEMPLATE_DIR + "eit_small.tmp"));
		result.strReplace("#SERVICENAME#", filter_string(getCurService()));
		eString tmp = getZapContent(mode, path);
		if (tmp)
			result += tmp;
		else
			result = "";
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
#endif

static eString msgWindow(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString title = opt["title"];
	eString msg = opt["msg"];
	return getMsgWindow(title, msg);
}

static eString aboutDreambox(void)
{
	std::stringstream result;
	result << "<table border=0 cellspacing=0 cellpadding=0>";

	if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000
		|| eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7020)
		if (pdaScreen == 0)
			result << "<img src=\"dm7000.jpg\" width=\"630\" border=\"0\"><br><br>";
		else
			result << "<img src=\"dm7000.jpg\" width=\"160\" border=\"0\"><br><br>";

	result  << "<tr><td>Model:</td><td>" << eSystemInfo::getInstance()->getModel() << "</td></tr>"
		<< "<tr><td>Manufacturer:</td><td>" << eSystemInfo::getInstance()->getManufacturer() << "</td></tr>"
		<< "<tr><td>Processor:</td><td>" << eSystemInfo::getInstance()->getCPUInfo() << "</td></tr>";

#ifndef DISABLE_FILE
	result << getDiskInfo();
	result << getUSBInfo();
#endif
	result	<< "<tr><td>Linux Kernel:</td><td>" << readFile("/proc/version") << "</td></tr>"
		<< "<tr><td>Firmware:</td><td>" << firmwareLevel(getAttribute("/.version", "version")) << "</td></tr>";
		if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000 ||
			eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7020 )
			result << "<tr><td>FP Firmware:</td><td>" << eString().sprintf(" 1.%02d", eDreamboxFP::getFPVersion()) << "</td></tr>";
		result << "<tr><td>Web Interface:</td><td>" << WEBIFVERSION << "</td></tr>"
		<< "</table>";

	return result.str();
}

#ifndef	DISABLE_FILE
bool rec_movies()
{
	bool result = false;
	FILE *rec = fopen("/hdd/movie/recordings.epl", "w");
	if (rec)
	{
		fprintf(rec, "#NAME Aufgenommene Filme\n");

		struct dirent **namelist;
		int n = scandir("/hdd/movie", &namelist, 0, alphasort);
		if (n > 0)
		{
			for (int i = 0; i < n; i++)
			{
				eString filen = namelist[i]->d_name;
				if ((filen.find(".ts") != eString::npos) && (filen.find(".ts.") == eString::npos))
				{
					fprintf(rec, "#SERVICE: 1:0:1:0:0:0:000000:0:0:0:/hdd/movie/%s\n", filen.c_str());
					fprintf(rec, "#DESCRIPTION: %s\n", getLeft(filen, '.').c_str());
					fprintf(rec, "#TYPE 16385\n");
					fprintf(rec, "/hdd/movie/%s\n", filen.c_str());
				}
				free(namelist[i]);
			}
			free(namelist);
			result = true;
		}
		fclose(rec);
		eZapMain::getInstance()->loadRecordings();
	}
	return result;
}

static eString recoverRecordings(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString result;
	if (rec_movies())
		result = "<html><head><title>Info</title></head><body>Movies recovered successfully.</body></html>";
	else
		result = "<html><head><title>Info</title></head><body>Movies could not be recovered.</body></html>";
	return result;
}
#endif

class myTimerEntry
{
public:
	int start;
	eString timerData;
	myTimerEntry(int pStart, eString pTimerData)
	{
		start = pStart;
		timerData = pTimerData;
	};
	~myTimerEntry() {};
	bool operator < (const myTimerEntry &a) const {return start < a.start;}
};

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
	std::list<myTimerEntry> &myList;
	bool repeating;

	getEntryString(std::list<myTimerEntry> &myList, bool repeating)
		:myList(myList), repeating(repeating)
	{
	}

	void operator()(ePlaylistEntry* se)
	{
		eString tmp = readFile(TEMPLATE_DIR + "timerListEntry.tmp");
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

		tmp.strReplace("#DELETEPARMS#", "ref=" + ref2string(se->service) + "&start=" + eString().sprintf("%d", se->time_begin) + "&type=" + eString().sprintf("%d", se->type) + "&force=no");
		tmp.strReplace("#EDITPARMS#", "ref=" + ref2string(se->service) + "&start=" + eString().sprintf("%d", se->time_begin) + "&duration=" + eString().sprintf("%d", se->duration) + "&channel=" + channel + "&descr=" + description + "&type=" + eString().sprintf("%d", se->type));

		if (se->type & ePlaylistEntry::stateFinished)
			tmp.strReplace("#STATEPIC#", "on.gif");
		else
		if (se->type & ePlaylistEntry::stateError)
			tmp.strReplace("#STATEPIC#", "off.gif");
		else
			tmp.strReplace("#STATEPIC#", "trans.gif");

		if (se->type & ePlaylistEntry::isRepeating)
		{
			eString days;
			if (se->type & ePlaylistEntry::Su)
				days += "Su ";
			if (se->type & ePlaylistEntry::Mo)
				days += "Mo ";
			if (se->type & ePlaylistEntry::Tue)
				days += "Tue ";
			if (se->type & ePlaylistEntry::Wed)
				days += "Wed ";
			if (se->type & ePlaylistEntry::Thu)
				days += "Thu ";
			if (se->type & ePlaylistEntry::Fr)
				days += "Fr ";
			if (se->type & ePlaylistEntry::Sa)
				days += "Sa";

			tmp.strReplace("#DAYS#", days);
			tmp.strReplace("#START#", eString().sprintf("XX.XX. - %02d:%02d", startTime.tm_hour, startTime.tm_min));
			tmp.strReplace("#END#", eString().sprintf("XX.XX. - %02d:%02d", endTime.tm_hour, endTime.tm_min));
		}
		else
		{
			tmp.strReplace("#DAYS#", "&nbsp;");
			tmp.strReplace("#START#", eString().sprintf("%02d.%02d. - %02d:%02d", startTime.tm_mday, startTime.tm_mon + 1, startTime.tm_hour, startTime.tm_min));
			tmp.strReplace("#END#", eString().sprintf("%02d.%02d. - %02d:%02d", endTime.tm_mday, endTime.tm_mon + 1, endTime.tm_hour, endTime.tm_min));
		}
		tmp.strReplace("#CHANNEL#", channel);
		tmp.strReplace("#DESCRIPTION#", description);

		myList.push_back(myTimerEntry(se->time_begin, tmp));
	}
};

static eString getControlTimerList()
{
	eString result = readFile(TEMPLATE_DIR + "timerListBody.tmp");
	std::list<myTimerEntry> myList;
	std::list<myTimerEntry>::iterator myIt;

	// regular timers
	int count = 0;
	eTimerManager::getInstance()->forEachEntry(countTimer(count, false));

	eString tmp;
	if (count)
	{
		eTimerManager::getInstance()->forEachEntry(getEntryString(myList, 0));
		myList.sort();
		for (myIt = myList.begin(); myIt != myList.end(); ++myIt)
			tmp += myIt->timerData;
		result.strReplace("#TIMER_REGULAR#", tmp);
	}
	else
		result.strReplace("#TIMER_REGULAR#", "<tr><td colspan=\"7\">None</td></tr>");

	tmp ="";
	myList.clear();

	// repeating timers
	count = 0;
	eTimerManager::getInstance()->forEachEntry(countTimer(count, true));
	if (count)
	{
		eTimerManager::getInstance()->forEachEntry(getEntryString(myList, 1));
		myList.sort();
		for (myIt = myList.begin(); myIt != myList.end(); ++myIt)
			tmp += myIt->timerData;
		result.strReplace("#TIMER_REPEATED#", tmp);
	}
	else
		result.strReplace("#TIMER_REPEATED#", "<tr><td colspan=\"7\">None</td></tr>");

	// buttons
	result.strReplace("#BUTTONCLEANUP#", button(100, "Cleanup", BLUE, "javascript:cleanupTimerList()", "#FFFFFF"));
	result.strReplace("#BUTTONCLEAR#", button(100, "Clear", RED, "javascript:clearTimerList()", "#FFFFFF"));
	result.strReplace("#BUTTONADD#", button(100, "Add", GREEN, "javascript:showAddTimerEventWindow()", "#FFFFFF"));

	return result;
}

#ifndef DISABLE_FILE
#ifdef ENABLE_DYN_CONF
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
	char *swapfilename;
	if (eConfig::getInstance()->getKey("/extras/swapfilename", swapfilename))
		swapfilename = "";
	result.strReplace("#SWAP#", (swapfile == 1) ? "checked" : "");
	result.strReplace("#SWAPFILE#", eString(swapfilename));
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
	return result;
}
#endif
#endif

eString getSatellites(void)
{
	eString result;
	int num = 0;

	for (std::list<eLNB>::iterator it2(eTransponderList::getInstance()->getLNBs().begin()); it2 != eTransponderList::getInstance()->getLNBs().end(); it2++)
	{
		// go thru all satellites...
		for (ePtrList<eSatellite>::iterator s (it2->getSatelliteList().begin()); s != it2->getSatelliteList().end(); s++)
		{
			result += "<tr bgcolor=\"";
			result += (num & 1) ? LIGHTGREY : DARKGREY;
			result += "\">";
			result += "<td><a href=\'?mode=controlSatFinder&display=transponders&sat=" + eString().sprintf("%d", s->getOrbitalPosition()) + "\'>" + s->getDescription() + "</a></td>";
			result += "</tr>\n";
			num++;
		}
	}
	return result;
}

eString getTransponders(int orbital_position)
{
	eString result;
	int num = 0;

	result += "<h2>Satellite Orbital Position: " + eString().sprintf("%d", orbital_position) + "</h2>\n";
	// go thru all transponders...
	for (std::list<tpPacket>::iterator it3(eTransponderList::getInstance()->getNetworks().begin()); it3 != eTransponderList::getInstance()->getNetworks().end(); it3++)
	{
		if (it3->orbital_position == orbital_position)
		{
			for (std::list<eTransponder>::iterator it(it3->possibleTransponders.begin()); it != it3->possibleTransponders.end(); it++)
			{
				eString transponder = eString().sprintf("%d / %d / %c", it->satellite.frequency / 1000, it->satellite.symbol_rate / 1000, it->satellite.polarisation ? 'V' : 'H');
				result += "<tr bgcolor=\"";
				result += (num & 1) ? LIGHTGREY : DARKGREY;
				result += "\">";
				result += "<td><a href=\'javascript:tuneTransponder(\"" + it->satellite.toString() + "\")\'>" + transponder + "</a></td>";
				result += "</tr>\n";
				num++;
			}
		}
	}
	return result;
}

eString getSatellitesAndTransponders(void)
{
	eTransponder *tp = NULL;
	eString bouquets, bouquetrefs; // satellites
	eString channels, channelrefs; // transponders
	eString chs, chrefs;
	eString transponder;
	int currentSatellite = -1, currentTransponder = -1;
	int j, k;

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (sapi && sapi->transponder)
		tp = sapi->transponder;

	k = 0;
	for (std::list<eLNB>::iterator it2(eTransponderList::getInstance()->getLNBs().begin()); it2 != eTransponderList::getInstance()->getLNBs().end(); it2++)
	{
		// first go thru all satellites...
		for (ePtrList<eSatellite>::iterator s (it2->getSatelliteList().begin()); s != it2->getSatelliteList().end(); s++)
		{
			bouquets += "\"" + s->getDescription() + "\", ";
			bouquetrefs += "\"" + s->getDescription() + "\", ";
			if (tp && s->getOrbitalPosition() == tp->satellite.orbital_position)
			{
				//this is the current satellite...
				currentSatellite = k;
			}
			// enter sat into satellite list result1...
			channels += "channels[";
			channels += eString().sprintf("%d", k);
			channels += "] = new Array(";
			channelrefs += "channelRefs[";
			channelrefs += eString().sprintf("%d", k);
			channelrefs += "] = new Array(";
			chs = "";
			chrefs = "";

			j = 0;

			// then go thru all transponders...
			for (std::list<tpPacket>::iterator it3(eTransponderList::getInstance()->getNetworks().begin()); it3 != eTransponderList::getInstance()->getNetworks().end(); it3++)
			{
				if (it3->orbital_position == s->getOrbitalPosition())
				{
					for (std::list<eTransponder>::iterator it(it3->possibleTransponders.begin()); it != it3->possibleTransponders.end(); it++)
					{
						if (tp && *tp == *it)
						{
							// this is the current transponder
							currentTransponder = j;
						}
						transponder = eString().sprintf("%d / %d / %c", it->satellite.frequency / 1000, it->satellite.symbol_rate / 1000, it->satellite.polarisation ? 'V' : 'H');
						chs += "\"" + transponder + "\", ";
						chrefs += "\"" + it->satellite.toString() + "\", ";
					}
				}
				j++;
			}

			channels += chs.left(chs.length() - 2);
			channels += ");";
			channelrefs += chrefs.left(chrefs.length() - 2);
			channelrefs += ");";
			k++;
		}
	}
	bouquetrefs = bouquetrefs.left(bouquetrefs.length() - 2);
	bouquets = bouquets.left(bouquets.length() - 2);

	eString zapdata = readFile(HTDOCS_DIR + "zapdata.js");
	zapdata.strReplace("#BOUQUETS#", bouquets);
	zapdata.strReplace("#BOUQUETREFS#", bouquetrefs);
	zapdata.strReplace("#CHANNELS#", channels);
	zapdata.strReplace("#CHANNELREFS#", channelrefs);
	zapdata.strReplace("#CURRENTBOUQUET#", eString().sprintf("%d", currentSatellite));
	zapdata.strReplace("#CURRENTCHANNEL#", eString().sprintf("%d", currentTransponder));
	zapdata.strReplace("#AUTOBOUQUETCHANGE#", eString().sprintf("%d", 0)); // not used on client
	zapdata.strReplace("#ZAPMODE#", eString().sprintf("%d", -1));
	zapdata.strReplace("#ZAPSUBMODE#", eString().sprintf("%d", 0)); // not used on client

	eString result = readFile(TEMPLATE_DIR + "sat.tmp");
	result.strReplace("#ZAPDATA#", zapdata);
	if (screenWidth > 1024)
		result.strReplace("#SELSIZE#", "30");
	else
		result.strReplace("#SELSIZE#", "15");

	return result;
}

static eString getControlSatFinder(eString opts)
{
	eString result;
	std::map<eString, eString> opt = getRequestOptions(opts, '&');

	if (pdaScreen == 0)
		result = getSatellitesAndTransponders();
	else
	{
		// pda satfinder
		result += "<table width=\"100%\" border=\"0\" cellpadding=\"0\" cellspacing=\"0\">";
		eString display = opt["display"];
		if (display == "transponders")
			result += getTransponders(atoi(opt["sat"].c_str()));
		else
			result += getSatellites();
		result += "</table>";
	}
	return result;
}

static eString getControlScreenShot(void)
{
	eString result;

	int ret = system("grabpic bmp > /tmp/screenshot.bmp");
	eDebug("ret is %d", ret);
	if (ret >> 8)
		result = "grabpic tool is required but not existing or working";
	else
	{
		FILE *bitstream = 0;
		int xres = 0, yres = 0, yres2 = 0, aspect = 0, winxres = 630, winyres = 0, rh = 0, rv = 0;
		if (pdaScreen == 1)
			winxres = 160;
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

static eString getContent(eString mode, eString path, eString opts)
{
	eString result, tmp;
	lastTransponder = "";

	if (mode == "zap")
	{
		tmp = "ZAP";
		if (pdaScreen == 0)
		{
			if (zapMode >= 0 && zapMode <= 4)
				tmp += ": " + zap[zapMode][ZAPSUBMODENAME];
			if (zapSubMode >= 2 && zapSubMode <= 4)
				tmp += " - " + zapSubModes[zapSubMode];
		}

		result = getTitle(tmp);
		tmp = getZap(mode, path);
		if (tmp)
			result += tmp;
		else
			result = "";
	}
	else
#if ENABLE_DYN_MOUNT || ENABLE_DYN_CONF || ENABLE_DYN_FLASH
	if (mode == "config")
	{
		result = getTitle("CONFIG");
		result += "Select one of the configuration categories on the left";
	}
	else
#endif
#ifdef ENABLE_DYN_FLASH
	if (mode == "configFlashMgr")
	{
		result = getTitle("CONFIG: Flash Manager");
		result += getConfigFlashMgr();
	}
	else
#endif
#ifndef DISABLE_FILE
#ifdef ENABLE_DYN_MOUNT
	if (mode == "configMountMgr")
	{
		result = getTitle("CONFIG: Mount Manager");
		result += getConfigMountMgr();
	}
	else
#endif
#ifdef ENABLE_DYN_CONF
	if (mode == "configSwapFile")
	{
		result = getTitle("CONFIG: Swap File");
		result += getConfigSwapFile();
	}
	else
	if (mode == "configSettings")
	{
		result = getTitle("CONFIG: Settings");
		result += getConfigSettings();
	}
	else
#endif
#endif
#if ENABLE_DYN_ROTOR
	if (mode == "configRotor")
	{
		result = getTitle("CONFIG: Rotor");
		result += getConfigRotor();
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
		{
			result += "<table bgcolor=\"#000000\" cellpadding=\"0\" cellspacing=\"0\">";
			result += "<tr><td>";
			if (pdaScreen == 0)
				result += "<img width=\"630\" src=\"/root/tmp/osdshot.png\" border=0>";
			else
				result += "<img width=\"240\" src=\"/root/tmp/osdshot.png\" border=0>";
			result += "</td></tr>";
			result += "</table>";
		}
	}
	else
#ifndef DISABLE_LCD
	if (mode == "controlLCDShot")
	{
		result = getTitle("CONTROL: LCDShot");
		if (!getOSDShot("lcd"))
			if (pdaScreen == 0)
				result += "<img width=\"630\" src=\"/root/tmp/osdshot.png\" border=0>";
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
	if (mode == "controlSatFinder")
	{
		result = getTitle("CONTROL: Satfinder");
		result += getControlSatFinder(opts);
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

eString getEITC(eString result)
{
	eString now_time, now_duration, now_text, now_longtext,
		next_time, next_duration, next_text, next_longtext;

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
					if (event->start_time)
					{
						now_time.sprintf("%s", ctime(&event->start_time));
						now_time = now_time.mid(11, 5);
					}

					now_duration.sprintf("%d", (int)(event->duration / 60));
				}
				if (p == 1)
				{
					if (event->start_time)
					{
 						next_time.sprintf("%s", ctime(&event->start_time));
						next_time = next_time.mid(11, 5);
						next_duration.sprintf("%d", (int)(event->duration / 60));
					}
				}
				for (ePtrList<Descriptor>::iterator descriptor(event->descriptor); descriptor != event->descriptor.end(); ++descriptor)
				{
					if (descriptor->Tag() == DESCR_SHORT_EVENT)
					{
						ShortEventDescriptor *ss = (ShortEventDescriptor*)*descriptor;
						switch(p)
						{
							case 0:
								now_text = filter_string(ss->event_name);
								break;
							case 1:
								next_text = filter_string(ss->event_name);
								break;
						}
					}
					if (descriptor->Tag() == DESCR_EXTENDED_EVENT)
					{
						ExtendedEventDescriptor *ss = (ExtendedEventDescriptor*)*descriptor;
						switch(p)
						{
							case 0:
								now_longtext += filter_string(ss->text);
								break;
							case 1:
								next_longtext += filter_string(ss->text);
								break;
						}
					}
				}
				p++;
		 	}
		}
		eit->unlock();
	}

	result.strReplace("#NOWT#", now_time);
	if (now_duration)
		now_duration = "(" + now_duration + ")";
	result.strReplace("#NOWD#", now_duration);
	result.strReplace("#NOWST#", now_text);
	result.strReplace("#NOWLT#", now_longtext);
	result.strReplace("#NEXTT#", next_time);
	if (next_duration)
		next_duration = "(" + next_duration + ")";
	result.strReplace("#NEXTD#", next_duration);
	result.strReplace("#NEXTST#", next_text);
	result.strReplace("#NEXTLT#", next_longtext);

	eString curService = getCurService();
	eString curServiceRef;
	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if (sapi)
		curServiceRef = ref2string(sapi->service);
	eString curSubService = getCurrentSubChannel(curServiceRef);
	if (curSubService)
	{
		if (curService)
			curService += ": " + curSubService;
		else
			curService = curSubService;
	}
	result.strReplace("#SERVICENAME#", curService);

	return result;
}

static eString audiom3u(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="audio/mpegfile";
	return "http://" + getIP() + ":31338/" + eString().sprintf("%02x\n", Decoder::current.apid);
}

static eString videopls(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	system("killall -9 streamts");
	eString vpid = eString().sprintf("%04x", Decoder::current.vpid);
	eString apid = eString().sprintf("%04x", Decoder::current.apid);
	eString pmt = eString().sprintf("%04x", Decoder::current.pmtpid);

	content->local_header["Content-Type"]="video/mpegfile";
	content->local_header["Cache-Control"] = "no-cache";
	content->local_header["vpid"] = vpid;
	content->local_header["apid"] = apid;
	content->local_header["pmt"] = pmt;

	return "http://" + getIP() + ":31339/0," + pmt + "," + vpid  + "," + apid;
}

#define CHANNELWIDTH 200

class eMEPG: public Object
{
	int d_min;
	eString multiEPG;
	int hours;
	time_t start;
	time_t end;
	int tableWidth;
	int channelWidth;
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
		// sorry.. at moment we dont show any directory.. or locked service in webif
		if (ref.isLocked() && eConfig::getInstance()->pLockActive())
			return;

		time_t now = time(0) + eDVB::getInstance()->time_difference;

		std::stringstream result;
		result << std::setfill('0');
		eService* current;

		eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
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
					result  << "<tr>"
						<< "<td id=\"channel\" width=" << eString().sprintf("%d", channelWidth) << ">"
						<< "<span class=\"channel\">"
						<< filter_string(current->service_name)
						<< "</span>"
						<< "</td>";
					tablePos += CHANNELWIDTH;

					timeMap::const_iterator It;

					for (It = evt->begin(); It != evt->end(); ++It)
					{
						eString ext_description;
						eString short_description;
						eString genre;
						int genreCategory = 0; //none
						EITEvent event(*It->second);
						for (ePtrList<Descriptor>::iterator d(event.descriptor); d != event.descriptor.end(); ++d)
						{
							Descriptor *descriptor = *d;
							if (descriptor->Tag() == DESCR_SHORT_EVENT)
								short_description = ((ShortEventDescriptor *)descriptor)->event_name;
							else
							if (descriptor->Tag() == DESCR_EXTENDED_EVENT)
								ext_description += ((ExtendedEventDescriptor *)descriptor)->text;
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
											genre = gettext(eChannelInfo::getGenre(genreCategory * 16 + ce->content_nibble_level_2).c_str());
									}
								}
							}
						}

						if (!genre)
							genre = "n/a";

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
							if (colUnits == 1)
							{
								eventStart = tableTime;
								eventDuration += adjust2FifteenMinutes(event.duration);
							}
							else
							{
								result << "<td colspan=" << colUnits << ">&nbsp;</td>";
								tableTime = eventStart;
								tablePos += colUnits * 15 * d_min;
								eventDuration = adjust2FifteenMinutes(event.duration);
							}
						}

						if ((eventDuration > 0) && (eventDuration < 15 * 60))
							eventDuration = 15 * 60;

						if (tableTime + eventDuration > end)
							eventDuration = end - tableTime;

						colUnits = eventDuration / 60 / 15;
						if (colUnits > 0)
						{
							result  << "<td class=\"genre"
								<< eString().sprintf("%02d", genreCategory)
								<< "\" colspan=" << colUnits << "\">";
#ifndef DISABLE_FILE
							result  << "<a href=\"javascript:record('"
								<< "ref=" << ref2string(ref)
								<< "&start=" << event.start_time
								<< "&duration=" << event.duration;
							eString tmp = filter_string(short_description);
							tmp.strReplace("\'", "\\\'");
							tmp.strReplace("\"", "\\\"");
							result  << "&descr=" << tmp
								<< "&channel=" << filter_string(current->service_name)
								<< "')\"><img src=\"timer.gif\" border=0></a>"
								<< "&nbsp;&nbsp;";
#endif
							tm* t = localtime(&event.start_time);
							result  << std::setfill('0')
								<< "<span class=\"time\">"
								<< std::setw(2) << t->tm_mday << '.'
								<< std::setw(2) << t->tm_mon+1 << ". - "
								<< std::setw(2) << t->tm_hour << ':'
								<< std::setw(2) << t->tm_min << ' '
								<< "</span>"
								<< "<span class=\"duration\">"
								<< " (" << event.duration / 60 << " min)"
								<< "</span>"
								<< "<br>";
							if ((eventStart <= now) && (eventEnd >= now))
								result << "<a href=\'javascript:switchChannel(\"" << ref2string(ref) << "\", \"0\", \"-1\")\'>";
							result  << "<span class=\"event\">"
								<< "<b>" << short_description << "</b>"
								<< "</span>";
							if ((eventStart <= now) && (eventEnd >= now))
								result << "</a>";

							result	<< "<br>"
								<< "Genre: " << genre
								<< "<br>";

							if ((eventDuration >= 15 * 60) && (pdaScreen == 0))
							{
								result  << "<span class=\"description\">"
									<< filter_string(ext_description)
									<< "</span>";
							}

							result  << "</td>\n";
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

	eMEPG(int hours, time_t start, const eServiceReference & bouquetRef, int channelWidth)
		:d_min((pdaScreen == 0) ? 5 : 3)  // distance on time scale for 1 minute
		,hours(hours)   // horizontally visible hours
		,start(start)
		,end(start + hours * 3600)
		,tableWidth((end - start) / 60 * d_min + channelWidth)
		,channelWidth((pdaScreen == 0) ? CHANNELWIDTH : CHANNELWIDTH / 2)
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

	eString getTimeScale(int channelWidth)
	{
		std::stringstream result;

		result << "<tr>"
			<< "<th width=" << eString().sprintf("%d", channelWidth) << ">"
			<< "CHANNEL"
			<< "<br>"
			<< "<img src=\"trans.gif\" border=\"0\" height=\"1\" width=\"" << eString().sprintf("%d", channelWidth) << "\">"
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
	int channelWidth = (pdaScreen == 0) ? CHANNELWIDTH : CHANNELWIDTH / 2;

	time_t start = time(0) + eDVB::getInstance()->time_difference;
	start -= ((start % 900) + (60 * 60)); // align to 15 mins & start 1 hour before now
	int hours = 24;
	eConfig::getInstance()->getKey("/elitedvb/multiepg/hours", hours); // horizontally visible hours

	eMEPG mepg(hours, start, bouquetRef, channelWidth);

	eString result = (pdaScreen == 0) ? readFile(TEMPLATE_DIR + "mepg.tmp") : readFile(TEMPLATE_DIR + "mepg_small.tmp");
	result.strReplace("#TIMESCALE#", mepg.getTimeScale(channelWidth));
	result.strReplace("#BODY#", mepg.getMultiEPG());
	return result;
}

static eString getcurepg(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::stringstream result;
	eString description, ext_description, genre;
	int genreCategory = 0;
	result << std::setfill('0');

	eService* current;
	eServiceReference ref;

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString type = opt["type"];

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "No EPG available";

	eString serviceRef = opt["ref"];
	if (serviceRef)
		ref = string2ref(serviceRef);
	else
		ref = sapi->service;

	eDebug("[ENIGMA_DYN] getcurepg: opts = %s, serviceRef = %s", opts.c_str(), serviceRef.c_str());

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
					<< "&start=" << event.start_time
					<< "&duration=" << event.duration;
				eString tmp = filter_string(description);
				tmp.strReplace("\'", "\\\'");
				tmp.strReplace("\"", "\\\"");
				result  << "&descr=" << tmp
					<< "&channel=" << filter_string(current->service_name)
					<< "')\"><img src=\"timer.gif\" border=0></a>"
					<< "</td>";
#endif
				result  << "<td class=\"genre" << eString().sprintf("%02d", genreCategory) << "\">"
					<< "<span class=\"event\">"
					<< filter_string(description)
					<< "</span>"
					<< "<br>"
					<< "Genre: " << genre
					<< "<br>"
					<< "<span class=\"description\">"
					<< filter_string(ext_description)
					<< "</span>"
					<< "</td>"
					<< "</tr>\n";
			}
			else
			{
				result  << eString().sprintf("<!-- ID: %04x -->", event.event_id)
					<< eString().sprintf("<span class=\"epg\">%02d.%02d - %02d:%02d ", t->tm_mday, t->tm_mon+1, t->tm_hour, t->tm_min)
					<< description
					<< "</span><br>\n";
			}
		}
	}
	eEPGCache::getInstance()->Unlock();

	eString tmp;
	if (type == "extended")
		tmp = readFile(TEMPLATE_DIR + "epg.tmp");
	else
		tmp = readFile(TEMPLATE_DIR + "epg_old.tmp");
	tmp.strReplace("#CHANNEL#", filter_string(current->service_name));
	tmp.strReplace("#BODY#", result.str());
	return tmp;
}

static eString getstreaminfo(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::stringstream result;
	eString name, provider, vpid, apid, pcrpid, tpid, vidform("n/a"), tsid, onid, sid, pmt;

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

static eString getchannelinfo(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result = getEITC(readFile(TEMPLATE_DIR + "eit.tmp"));
	result.strReplace("#SERVICENAME#", getCurService());

	return result;
}

eString genBar(int val)
{
	std::stringstream result;
	for (int i = 10; i <= 100; i += 10)
	{
		result << "<td width=\"15\" height=\"8\">";
		if (i <= val)
			result << "<img src=\"led_on.gif\" border=\"0\" width=\"15\" height=\"8\">";
		else
			result << "<img src=\"led_off.gif\" border=\"0\" width=\"15\" height=\"8\">";
		result << "</td>";
	}
	return result.str();
}

static eString satFinder(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	if (opts != lastTransponder)
		tuneTransponder(opts);

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result = readFile(TEMPLATE_DIR + "satFinder.tmp");

	eFrontend *fe = eFrontend::getInstance();
	int snr = fe->SNR();
	int agc = fe->SignalStrength();
	unsigned int ber = fe->BER();
	int status = fe->Status();
	bool lock = status & FE_HAS_LOCK;
	bool sync = status & FE_HAS_SYNC;

	result.strReplace("#SNR#", eString().sprintf("%d", snr * 100 / 65535));
	result.strReplace("#SNRBAR#", genBar(snr * 100 / 65535));
	result.strReplace("#AGC#", eString().sprintf("%d", agc * 100 / 65535));
	result.strReplace("#AGCBAR#", genBar(agc * 100 / 65535));
	result.strReplace("#BER#", eString().sprintf("%d", ber));
	result.strReplace("#BERBAR#", genBar(ber));
	result.strReplace("#LOCK#", (lock) ? "checked" : "");
	result.strReplace("#SYNC#", (sync) ? "checked" : "");

	return result;
}

static eString message(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	std::map<eString, eString> opts = getRequestOptions(opt, '&');
	eString msg = opts["msg"];
	eString result = "-error";
	if (!msg)
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
		result = closeWindow(content, "", 500);

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
		result = closeWindow(content, "", 500);

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
	eString result;
	std::map<eString,eString> opt = getRequestOptions(opts, '&');

	eString mode = opt["mode"];
	eString path = opt["path"];
	eString curBouquet = opt["curBouquet"];
	if (curBouquet)
		currentBouquet = atoi(curBouquet.c_str());
	eString curChannel = opt["curChannel"];
	if (curChannel)
		currentChannel = atoi(curChannel.c_str());

	eServiceReference current_service = string2ref(path);

	if (!(current_service.flags&eServiceReference::isDirectory))	// is playable
	{
		playService(current_service);
		result = closeWindow(content, "Please wait...", 3000);
	}
	return result;
}

static eString getCurrentServiceRef(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if (eServiceInterface::getInstance()->service)
		return eServiceInterface::getInstance()->service.toString();
	else
		return "E:no service running";
}

eString getPDAContent(eString mode, eString path, eString opts)
{
	eString result;

	if (!path)
		path = eServiceStructureHandler::getRoot(eServiceStructureHandler::modeTV).toString();
	if (!mode)
		mode = "zap";

	result = readFile(TEMPLATE_DIR + "index_small.tmp");
	eString tmp = getContent(mode, path, opts);
	if (!tmp)
		result = "";
	result.strReplace("#CONTENT#", tmp);
	result.strReplace("#VOLBAR#", getVolBar());
	result.strReplace("#MUTE#", getMute());
	result.strReplace("#TOPNAVI#", getTopNavi(false));
	result.strReplace("#CHANNAVI#", getChannelNavi());
	result.strReplace("#LEFTNAVI#", getLeftNavi(mode, false));
	if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000
		|| eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7020)
		result.strReplace("#TOPBALK#", "topbalk_small.png");
	else
	if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::dbox2Nokia)
		result.strReplace("#TOPBALK#", "topbalk2_small.png");
	else
	if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::dbox2Sagem)
		result.strReplace("#TOPBALK#", "topbalk3_small.png");
	else
//	if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::dbox2Philips)
		result.strReplace("#TOPBALK#", "topbalk4_small.png");
	if (!result)
		result = "<html><body>Please wait...<script language=\"javascript\">window.close();</script></body></html>";
	return result;
}

static eString pda_root(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;

	pdaScreen = 1;
	screenWidth = 240;
	eConfig::getInstance()->setKey("/ezap/webif/pdaScreen", pdaScreen);

	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	content->local_header["Content-Type"] = "text/html; charset=utf-8";
	content->local_header["Cache-Control"] = "no-cache";

	eString mode = opt["mode"];
	eString path = opt["path"];
	result = getPDAContent(mode, path, opts);

	return result;
}

static eString web_root(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;

	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	content->local_header["Content-Type"]="text/html; charset=utf-8";

	eConfig::getInstance()->getKey("/ezap/webif/pdaScreen", pdaScreen);

	if (opts.find("screenWidth") != eString::npos)
	{
		eString sWidth = opt["screenWidth"];
		screenWidth = atoi(sWidth.c_str());
		pdaScreen = (screenWidth < 800) ? 1 : 0;
		eConfig::getInstance()->setKey("/ezap/webif/pdaScreen", pdaScreen);
	}
	else
	{
		if ((opts.find("mode") == eString::npos) && (opts.find("path") == eString::npos))
			return readFile(TEMPLATE_DIR + "index.tmp");
	}

	if (pdaScreen == 0)
	{
		result = readFile(TEMPLATE_DIR + "index_big.tmp");
		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000
			|| eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7020)
			result.strReplace("#BOX#", "Dreambox");
		else
			result.strReplace("#BOX#", "dBox");
		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000
			|| eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7020)
			result.strReplace("#TOPBALK#", "topbalk.png");
		else
		if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::dbox2Nokia)
			result.strReplace("#TOPBALK#", "topbalk2.png");
		else
		if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::dbox2Sagem)
			result.strReplace("#TOPBALK#", "topbalk3.png");
		else
//		if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::dbox2Philips)
			result.strReplace("#TOPBALK#", "topbalk4.png");
		result.strReplace("#EMPTYCELL#", "&nbsp;");
		result.strReplace("#TOPNAVI#", getTopNavi(true));
	}
	else
	{
		eString mode = opt["mode"];
		eString path = opt["path"];
		result = getPDAContent(mode, path, opts);
		content->local_header["Cache-Control"] = "no-cache";
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

void sendKey(int evd, unsigned int code, unsigned int value)
{
	struct input_event iev;

	iev.type = EV_KEY;
	iev.code = code;
	iev.value = value;
	write (evd, &iev, sizeof(iev));
}

int translateKey(int key)
{
	if (key == 393) // video
	{
		keyboardMode = (keyboardMode) ? 0 : 1;
	}
	else
	if (key == 66) // text
	{
		keyboardMode = KEYBOARDVIDEO;
	}
	else
	{
		if (keyboardMode == KEYBOARDVIDEO)
		{
			switch (key)
			{
				case 385: key = 128; break; // stop
				case 377: key = 167; break; // record
				case 398: key = 168; break; // rewind
				case 399: key = 207; break; // play
				case 400: key = 119; break; // pause
				case 401: key = 208; break; // forward
			}
		}
	}
	return key;
}

static eString remoteControl(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	enum
	{
		KEY_RELEASED = 0,
		KEY_PRESSED,
		KEY_AUTOREPEAT
	};

	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	content->local_header["Content-Type"]="text/html; charset=utf-8";

	eString keysS = opts;
	eString durationS;
	eString reptimeS;

	while (keysS)
	{
		unsigned int pos;
		eString keyS;
		if ((pos = keyS.find(",")) != eString::npos)
		{
			keyS = keysS.left(pos);
			keysS = keysS.right(keysS.length() - pos - 1);
		}
		else
		{
			keyS = keysS;
			keysS = "";
		}

		eString tmp = keyS;
		if ((pos = tmp.find(":")) != eString::npos)
		{
			keyS = tmp.left(pos);
			tmp = tmp.right(tmp.length() - pos - 1);
		}
		else
		{
			keyS = tmp;
			tmp = "";
		}

		if ((pos = tmp.find(":")) != eString::npos)
		{
			durationS = tmp.left(pos);
			reptimeS = tmp.right(tmp.length() - pos - 1);
		}
		else
		{
			durationS = tmp;
			reptimeS = "";
		}

		unsigned long duration = 0;
		if (durationS)
			duration = atol(durationS.c_str());

		unsigned long reptime = 500;
		if (reptimeS)
			atol(reptimeS.c_str());

		unsigned long time = duration * 1000 / reptime;

		int key = atoi(keyS.c_str());
		key = translateKey(key);
		int evd = open("/dev/input/event0", O_RDWR);
		if (evd)
		{
			sendKey(evd, key, KEY_PRESSED);
			while (time--)
			{
				usleep(reptime * 1000);
				sendKey(evd, key, KEY_AUTOREPEAT);
			}
			sendKey(evd, key, KEY_RELEASED);
			close(evd);
		}
	}
	return closeWindow(content, "", 10);
}

static eString showRemoteControl(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;
	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	content->local_header["Content-Type"]="text/html; charset=utf-8";

	if (pdaScreen == 0)
		if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::dbox2Nokia
		 || eSystemInfo::getInstance()->getHwType() == eSystemInfo::dbox2Sagem
		 || eSystemInfo::getInstance()->getHwType() == eSystemInfo::dbox2Philips)
			result = readFile(TEMPLATE_DIR + "remoteControlDbox2.tmp");
		else
			result = readFile(TEMPLATE_DIR + "remoteControl.tmp");
	else
		result = readFile(TEMPLATE_DIR + "pdaRemoteControl.tmp");

	return result;
}

static eString getCurrentVpidApid(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if (opt != "getpids")
		return eString("+ok");
	else
		return eString().sprintf("%u\n%u\n", Decoder::current.vpid, Decoder::current.apid);
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

#if 0
class treeNode
{
public:
	eString serviceNode;
	eString serviceName;
	bool isDirectory;
	eServiceReference serviceReference;
	int nodeID;
	treeNode(int node, bool isdir, eString sname, eString snode, eServiceReference ref)
	{
//		eDebug("new Service: %s - %s", sname.c_str(), snode.c_str());
		serviceName = sname;
		serviceNode = snode;
		isDirectory = isdir;
		serviceReference = ref;
		nodeID = node;
	};
	~treeNode() {};
	bool operator < (const treeNode &a) const {return serviceName < a.serviceName;}
};

eString genNodes(bool sort, std::list <treeNode> &myList)
{
	std::list <treeNode>::iterator myIt;
	eString result;

	eDebug("[ENIGMA_DYN] start sorting...");

	if (sort)
		myList.sort();

	for (myIt = myList.begin(); myIt != myList.end(); ++myIt)
		result += myIt->serviceNode + "\n";

	eDebug("[ENIGMA_DYN] sorting done.");
	return result;
}

struct listChannels: public Object
{
	eString &result;
	std::list <treeNode> myList;
	eServiceInterface *iface;
	int &nodeID;
	int anchor;
	bool sort;
	bool addEPG;

	listChannels(const eServiceReference &service, eString &result, int &nodeID, int anchor, bool sort, bool addEPG)
		:result(result), myList(myList), iface(eServiceInterface::getInstance()), nodeID(nodeID), anchor(anchor), sort(sort), addEPG(addEPG)
	{
		std::list <treeNode>::iterator myIt;
		Signal1<void, const eServiceReference&> cbSignal;
		CONNECT(cbSignal, listChannels::addTreeNode);
		iface->enterDirectory(service, cbSignal);
		iface->leaveDirectory(service);
		result += genNodes(sort, myList);
		for (myIt = myList.begin(); myIt != myList.end(); ++myIt)
			if (myIt->isDirectory)
				listChannels(myIt->serviceReference, result, nodeID, myIt->nodeID, sort, addEPG);
	}

	void addTreeNode(const eServiceReference& ref)
	{
		eString serviceReference, serviceName, serviceDescription, serviceNode, orbitalPosition;
		
		// sorry.. at moment we dont show any directory.. or locked service in webif
		if (ref.isLocked() && eConfig::getInstance()->pLockActive())
			return;

		eService *service = iface ? iface->addRef(ref) : 0;

		serviceReference = ref.toString();
		if (ref.descr) serviceName = filter_string(ref.descr);
		else
		{
			if (service)
			{
				serviceName = filter_string(service->service_name);
				iface->removeRef(ref);
			}
			else
				serviceName = "unnamed service";
		}

		if (ref.type == eServiceReference::idDVB && !(ref.flags & eServiceReference::isDirectory))
		{
			const eServiceReferenceDVB& dvb_ref = (const eServiceReferenceDVB&)ref;
			eTransponder *tp = eTransponderList::getInstance()->searchTS(
				dvb_ref.getDVBNamespace(),
				dvb_ref.getTransportStreamID(),
				dvb_ref.getOriginalNetworkID());
			if (tp && tp->satellite.isValid())
				orbitalPosition = eString().setNum(tp->satellite.orbital_position);
			else
				orbitalPosition = 0;
		}

		eString epg;
		if (addEPG && epg)
			serviceDescription = serviceName + " - " + epg;
		else
			serviceDescription = serviceName;
			
		serviceDescription.strReplace("'", "\\\'");
		serviceNode = "d.add(" + eString().sprintf("%d", ++nodeID) + "," + eString().sprintf("%d", anchor) + ",'" + serviceDescription + "','javascript:switchChannel(\"" +  serviceReference + "\")');";
		myList.push_back(treeNode(nodeID, ref.flags & eServiceReference::isDirectory, serviceName, serviceNode, ref));
	}
};
#endif

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

#if 0
static eString getChannels(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/plain; charset=utf-8";
	std::map<eString,eString> opts=getRequestOptions(opt, '&');

	eString sref;
	if (opts["ref"])
		sref = opts["ref"];
	else
		sref = zap[0][1];

	eString result = "d.add(0,-1,'" + zap[0][0] + "');\n";
	eServiceReference ref(sref);
	int nodeID = 0;
	listChannels t(ref, result, nodeID, 0, false, false);

	if (result)
		return result;

	return "E: error during list channels";
}
#endif

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

static eString cleanupTimerList(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eTimerManager::getInstance()->cleanupEvents();
	eTimerManager::getInstance()->saveTimerList();
	return closeWindow(content, "", 500);
}

static eString clearTimerList(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eTimerManager::getInstance()->clearEvents();
	eTimerManager::getInstance()->saveTimerList();
	return closeWindow(content, "", 500);
}

static eString TVBrowserTimerEvent(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result, result1;

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString command = opt["command"];
	eString sday = opt["sday"];
	eString smonth = opt["smonth"];
	eString shour = opt["shour"];
	eString smin = opt["smin"];
	eString eday = opt["eday"];
	eString emonth = opt["emonth"];
	eString ehour = opt["ehour"];
	eString emin = opt["emin"];
	eString channel = httpUnescape(opt["channel"]);
	eString description = httpUnescape(opt["descr"]);
	if (!description)
		description = "No description available";

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

	if (channel.find("/") != eString::npos)
	{
		eString tmp = channel;
		channel = getLeft(tmp, '/');
		result1 = getRight(tmp, '/');
	}
	else
	{
		// determine service reference
		eServiceInterface *iface = eServiceInterface::getInstance();
		eServiceReference all_services = eServiceReference(eServiceReference::idDVB,
			eServiceReference::flagDirectory|eServiceReference::shouldSort,
			-2, -1, 0xFFFFFFFF);

		eWebNavigatorSearchService navlist(result1, channel, *iface);
		Signal1<void, const eServiceReference&> signal;
		signal.connect(slot(navlist, &eWebNavigatorSearchService::addEntry));
		iface->enterDirectory(all_services, signal);
		eDebug("entered");
		iface->leaveDirectory(all_services);
		eDebug("exited");
	}

	if (result1)
	{
		if (command == "add")
		{
			ePlaylistEntry entry(string2ref(result1), eventStartTime, duration, -1, ePlaylistEntry::stateWaiting | ePlaylistEntry::RecTimerEntry | ePlaylistEntry::recDVR);
			entry.service.descr = channel + "/" + description;

			if (eTimerManager::getInstance()->addEventToTimerList(entry) == -1)
			{
				content->code = 400;
				content->code_descr = "Function failed.";
				result = "Timer event could not be added because time of the event overlaps with an already existing event.";
			}
			else
				result = "Timer event was created successfully.";
			eTimerManager::getInstance()->saveTimerList();
		}
		if (command == "delete")
		{
			ePlaylistEntry e(
				string2ref(result1),
				eventStartTime,
				-1, -1, ePlaylistEntry::stateWaiting | ePlaylistEntry::RecTimerEntry | ePlaylistEntry::recDVR);

			eTimerManager::getInstance()->deleteEventFromTimerList(e, true);
			eTimerManager::getInstance()->saveTimerList();
			result = "Timer event deleted successfully.";
		}
	}
	else
	{
		if (command == "add")
		{
			content->code = 400;
			content->code_descr = "Function failed.";
			result = "TVBrowser and Enigma service name don't match.";
		}
		else
			result = "Service of timer event does not exist, or no longer exists.";
	}

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

	if (ret == -1)  // event currently running...
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
		eTimerManager::getInstance()->saveTimerList();
		result = readFile(TEMPLATE_DIR + "deleteTimerComplete.tmp");
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
	int oldType = atoi(oldEventType.c_str());
	eString oldStartTime = opt["old_stime"];
	eString newEventType = opt["type"];
	if (newEventType == "repeating")
		oldType |= ePlaylistEntry::isRepeating;
	else
		oldType &= ~ePlaylistEntry::isRepeating;

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
	eString mo = opt["mo"];
	eString tu = opt["tu"];
	eString we = opt["we"];
	eString th = opt["th"];
	eString fr = opt["fr"];
	eString sa = opt["sa"];
	eString su = opt["su"];

	time_t now = time(0)+eDVB::getInstance()->time_difference;
	tm start = *localtime(&now);
	if (oldType & ePlaylistEntry::isRepeating)
	{
		start.tm_year = 70;  // 1.1.1970
		start.tm_mon = 0;
		start.tm_mday = 1;
	}
	else
	{
		start.tm_mday = atoi(sday.c_str());
		start.tm_mon = atoi(smonth.c_str()) - 1;
	}
	start.tm_hour = atoi(shour.c_str());
	start.tm_min = atoi(smin.c_str());
	start.tm_sec = 0;

	tm end = *localtime(&now);
	if (oldType & ePlaylistEntry::isRepeating)
	{
		end.tm_year = 70;  // 1.1.1970
		end.tm_mon = 0;
		end.tm_mday = 1;
	}
	else
	{
		end.tm_mday = atoi(eday.c_str());
		end.tm_mon = atoi(emonth.c_str()) -1;
	}
	end.tm_hour = atoi(ehour.c_str());
	end.tm_min = atoi(emin.c_str());
	end.tm_sec = 0;

	time_t eventStartTime = mktime(&start);
	time_t eventEndTime = mktime(&end);
	int duration = eventEndTime - eventStartTime;

	eServiceReference ref = string2ref(serviceRef);

	ePlaylistEntry oldEvent(
		ref,
		atoi(oldStartTime.c_str()),
		-1, -1, oldType);

	if (oldStartTime < now && eventStartTime >= now)
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
	}

	oldType &= ~(ePlaylistEntry::doGoSleep|ePlaylistEntry::doShutdown);
		oldType |= ePlaylistEntry::stateWaiting;
		oldType |= atoi(after_event.c_str());

	if (oldType & ePlaylistEntry::isRepeating)
	{
		if (mo == "on")
			oldType |= ePlaylistEntry::Mo;
		else
			oldType &= ~ePlaylistEntry::Mo;
		if (tu == "on")
			oldType |= ePlaylistEntry::Tue;
		else
			oldType &= ~ePlaylistEntry::Tue;
		if (we == "on")
			oldType |= ePlaylistEntry::Wed;
		else
			oldType &= ~ePlaylistEntry::Wed;
		if (th == "on")
			oldType |= ePlaylistEntry::Thu;
		else
			oldType &= ~ePlaylistEntry::Thu;
		if (fr == "on")
			oldType |= ePlaylistEntry::Fr;
		else
			oldType &= ~ePlaylistEntry::Fr;
		if (sa == "on")
			oldType |= ePlaylistEntry::Sa;
		else
			oldType &= ~ePlaylistEntry::Sa;
		if (su == "on")
			oldType |= ePlaylistEntry::Su;
		else
			oldType &= ~ePlaylistEntry::Su;
	}
	else
	{
		oldType &= ~(	ePlaylistEntry::Mo |
				ePlaylistEntry::Tue |
				ePlaylistEntry::Wed |
				ePlaylistEntry::Thu |
				ePlaylistEntry::Fr |
				ePlaylistEntry::Sa |
				ePlaylistEntry::Su);
	}

	ref.descr = channel + "/" + description;
	ePlaylistEntry newEvent(
		ref,
		eventStartTime,
		duration,
		-1,
		oldType);

	int ret = eTimerManager::getInstance()->modifyEventInTimerList(oldEvent, newEvent, (force == "yes"));

	if (ret == -1)  // event currently running...
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
		eTimerManager::getInstance()->saveTimerList();
	}
	return result;
}

static eString addTimerEvent(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString serviceRef = opt["ref"];
	eString eventStartTimeS = opt["start"];
	eString eventDurationS = opt["duration"];
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
	eString timer = opt["timer"];
	eString mo = opt["mo"];
	eString tu = opt["tu"];
	eString we = opt["we"];
	eString th = opt["th"];
	eString fr = opt["fr"];
	eString sa = opt["sa"];
	eString su = opt["su"];

	time_t now = time(0) + eDVB::getInstance()->time_difference;

	int eventDuration = 0;
	time_t eventStartTime, eventEndTime;

	if (eventStartTimeS && eventDurationS)
	{
		eventStartTime = atoi(eventStartTimeS.c_str());
		eventDuration = atoi(eventDurationS.c_str());
	}
	else
	{
		tm start = *localtime(&now);
		if (timer == "repeating")
		{
			start.tm_year = 70;  // 1.1.1970
			start.tm_mon = 0;
			start.tm_mday = 1;
		}
		else
		{
			start.tm_mday = atoi(sday.c_str());
			start.tm_mon = atoi(smonth.c_str()) - 1;
		}
		start.tm_hour = atoi(shour.c_str());
		start.tm_min = atoi(smin.c_str());
		start.tm_sec = 0;

		tm end = *localtime(&now);
		if (timer == "repeating")
		{
			end.tm_year = 70;  // 1.1.1970
			end.tm_mon = 0;
			end.tm_mday = 1;
		}
		else
		{
			end.tm_mday = atoi(eday.c_str());
			end.tm_mon = atoi(emonth.c_str()) -1;
		}
		end.tm_hour = atoi(ehour.c_str());
		end.tm_min = atoi(emin.c_str());
		end.tm_sec = 0;

		eventStartTime = mktime(&start);
		eventEndTime = mktime(&end);
		eventDuration = eventEndTime - eventStartTime;
	}

	int timeroffset = 0;
	eConfig::getInstance()->getKey("/enigma/timeroffset", timeroffset);

	eventStartTime = eventStartTime - (timeroffset * 60);
	eventDuration = eventDuration + (2 * timeroffset * 60);

	int type = (after_event) ? atoi(after_event.c_str()) : 0;
	type |= ePlaylistEntry::stateWaiting | ePlaylistEntry::RecTimerEntry | ePlaylistEntry::recDVR;

	if (timer == "repeating")
	{
		type |= ePlaylistEntry::isRepeating;
		if (mo == "on")
			type |= ePlaylistEntry::Mo;
		if (tu == "on")
			type |= ePlaylistEntry::Tue;
		if (we == "on")
			type |= ePlaylistEntry::Wed;
		if (th == "on")
			type |= ePlaylistEntry::Thu;
		if (fr == "on")
			type |= ePlaylistEntry::Fr;
		if (sa == "on")
			type |= ePlaylistEntry::Sa;
		if (su == "on")
			type |= ePlaylistEntry::Su;
	}

	ePlaylistEntry entry(string2ref(serviceRef), eventStartTime, eventDuration, -1, type);
	entry.service.descr = channel + "/" + description;

	if (eTimerManager::getInstance()->addEventToTimerList(entry) == -1)
		result += "Timer event could not be added because time of the event overlaps with an already existing event.";
	else
	{
		result += "Timer event was created successfully.";
		eTimerManager::getInstance()->saveTimerList();
	}

	return result;
}

static eString buildAfterEventOpts(int type)
{
	std::stringstream afterOpts;
	if (type & ePlaylistEntry::doGoSleep || type & ePlaylistEntry::doShutdown)
		afterOpts << "<option value=\"0\">";
	else
		afterOpts << "<option selected value=\"0\">";
	afterOpts << "Nothing"
		<< "</option>";
	if (type & ePlaylistEntry::doGoSleep)
		afterOpts << "<option selected value=\"" << ePlaylistEntry::doGoSleep << "\">";
	else
		afterOpts << "<option value=\"" << ePlaylistEntry::doGoSleep << "\">";
	afterOpts << "Standby"
		<< "</option>";
	if (eSystemInfo::getInstance()->canShutdown())
	{
		if (type & ePlaylistEntry::doShutdown)
			afterOpts << "<option selected value=\"" << ePlaylistEntry::doShutdown << "\">";
		else
			afterOpts << "<option value=\"" << ePlaylistEntry::doShutdown << "\">";
		afterOpts << "Shutdown"
		<< "</option>";
	}
	return afterOpts.str();
}

static eString showEditTimerEventWindow(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString serviceRef = opt["ref"];
	eString eventStartTime = opt["start"];
	eString eventDuration = opt["duration"];
	eString description = httpUnescape(opt["descr"]);

	// this is only for renamed services (or subservices)... changing this in the edit dialog has no effect to
	// the recording service
	eString channel = httpUnescape(opt["channel"]);
	eString eventType = opt["type"];

	time_t eventStart = atoi(eventStartTime.c_str());
	time_t eventEnd = eventStart + atoi(eventDuration.c_str());
	tm start = *localtime(&eventStart);
	if (eventEnd % 300 > 0)
		eventEnd = eventEnd / 300 * 300 + 300;
	tm end = *localtime(&eventEnd);
	int evType = atoi(eventType.c_str());

	eString result = readFile(TEMPLATE_DIR + "editTimerEvent.tmp");

	result.strReplace("#CSS#", (pdaScreen == 0) ? "webif.css" : "webif_small.css");

	if (evType & ePlaylistEntry::isRepeating)
	{
		result.strReplace("#REPEATING#", "selected");
		result.strReplace("#REGULAR#", "");
	}
	else
	{
		result.strReplace("#REGULAR#", "selected");
		result.strReplace("#REPEATING#", "");
	}

	result.strReplace("#AFTEROPTS#", buildAfterEventOpts(evType));
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
	result.strReplace("#EMINOPTS#", genOptions(0, 55, 5, end.tm_min));
	result.strReplace("#CHANNEL#", channel);
	result.strReplace("#DESCRIPTION#", description);
	result.strReplace("#MO#", (evType & ePlaylistEntry::Mo) ? "checked" : "");
	result.strReplace("#TU#", (evType & ePlaylistEntry::Tue) ? "checked" : "");
	result.strReplace("#WE#", (evType & ePlaylistEntry::Wed) ? "checked" : "");
	result.strReplace("#TH#", (evType & ePlaylistEntry::Thu) ? "checked" : "");
	result.strReplace("#FR#", (evType & ePlaylistEntry::Fr) ? "checked" : "");
	result.strReplace("#SA#", (evType & ePlaylistEntry::Sa) ? "checked" : "");
	result.strReplace("#SU#", (evType & ePlaylistEntry::Su) ? "checked" : "");
	eTimerManager::getInstance()->saveTimerList();
	return result;
}

static eString showAddTimerEventWindow(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString timer = opt["timer"];

	time_t now = time(0) + eDVB::getInstance()->time_difference;
	tm start = *localtime(&now);
	tm end = *localtime(&now);

	eString result;
	if (timer == "repeating")
		result = readFile(TEMPLATE_DIR + "addRepeatingTimerEvent.tmp");
	else
		result = readFile(TEMPLATE_DIR + "addTimerEvent.tmp");

	result.strReplace("#CSS#", (pdaScreen == 0) ? "webif.css" : "webif_small.css");

	result.strReplace("#AFTEROPTS#", buildAfterEventOpts(0));

	result.strReplace("#SDAYOPTS#", genOptions(1, 31, 1, start.tm_mday));
	result.strReplace("#SMONTHOPTS#", genOptions(1, 12, 1, start.tm_mon + 1));
	result.strReplace("#SHOUROPTS#", genOptions(0, 23, 1, start.tm_hour));
	result.strReplace("#SMINOPTS#", genOptions(0, 55, 5, (start.tm_min / 5) * 5));

	result.strReplace("#EDAYOPTS#", genOptions(1, 31, 1, end.tm_mday));
	result.strReplace("#EMONTHOPTS#", genOptions(1, 12, 1, end.tm_mon + 1));
	result.strReplace("#EHOUROPTS#", genOptions(0, 23, 1, end.tm_hour));
	result.strReplace("#EMINOPTS#", genOptions(0, 55, 5, (end.tm_min / 5) * 5));

	result.strReplace("#ZAPDATA#", getZapContent2("zap", zap[ZAPMODETV][ZAPSUBMODEBOUQUETS], 2, false, false));
	if (pdaScreen == 1)
		result = "<html><head><title>Info</title></head><body>This function is not available for PDAs.</body></html>";

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
	eString description = "No description available";

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
		ext_description = "No detailed description available";

	result = readFile(TEMPLATE_DIR + "epgDetails.tmp");
	result.strReplace("#EVENT#", filter_string(description));
	result.strReplace("#BODY#", filter_string(ext_description));

	return result;
}

static eString blank(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result = readFile(TEMPLATE_DIR + "blank.tmp");
	return result;
}

static eString leftnavi(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eString mode = opt["mode"];
	if (!mode)
		mode = "zap";
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result = readFile(TEMPLATE_DIR + "leftnavi.tmp");

	result.strReplace("#LEFTNAVI#", getLeftNavi(mode, true));
	return result;
}

static eString channavi(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result = readFile(TEMPLATE_DIR + "channavi.tmp");
	result.strReplace("#CHANNAVI#", getChannelNavi());
	return result;
}

#ifndef DISABLE_FILE
extern int freeRecordSpace(void);  // implemented in enigma_main.cpp
#endif

static eString data(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result = readFile(TEMPLATE_DIR + "data.tmp");

	// epg data
	result = getEITC(result);

	// webif update cycle
	int updateCycle = 10000;
	eConfig::getInstance()->getKey("/ezap/webif/updateCycle", updateCycle);
	result.strReplace("#UPDATECYCLE#", eString().sprintf("%d", updateCycle));

	// standby
	result.strReplace("#STANDBY#", (eZapMain::getInstance()->isSleeping()) ? "1" : "0");

	// uptime
	int sec = atoi(readFile("/proc/uptime").c_str());
	result.strReplace("#UPTIME#", eString().sprintf("%d:%02d h up", sec / 3600, (sec % 3600) / 60));

	// IP
	result.strReplace("#IP#", getIP());

	// webif lock
	int lockWebIf = 1;
	eConfig::getInstance()->getKey("/ezap/webif/lockWebIf", lockWebIf);
	result.strReplace("#LOCK#", (lockWebIf == 1) ? "locked" : "unlocked");

	// vpid
	result.strReplace("#VPID#", (Decoder::current.vpid == -1) ? "none" : eString().sprintf("0x%x", Decoder::current.vpid));

	// apid
	result.strReplace("#APID#", (Decoder::current.apid == -1) ? "none" : eString().sprintf("0x%x", Decoder::current.apid));

	// free recording space on disk
#ifndef DISABLE_FILE
	int fds = freeRecordSpace();
#else
	int fds = 0;
#endif
	if (fds != -1)
	{
		if (fds < 1024)
			result.strReplace("#DISKGB#", eString().sprintf("%d MB", fds));
		else
			result.strReplace("#DISKGB#", eString().sprintf("%d.%02d GB", fds/1024, (int)((fds % 1024) / 10.34)));

		int min = fds / 33;
		if (min < 60)
			result.strReplace("#DISKH#", eString().sprintf("~%d min", min));
		else
			result.strReplace("#DISKH#", eString().sprintf("~%d h, %02d min", min/60, min%60));
	}

	// volume
	result.strReplace("#VOLUME#", (eAVSwitch::getInstance()->getMute()) ? "0" : eString().sprintf("%d", 63 - eAVSwitch::getInstance()->getVolume()));

	// mute
	result.strReplace("#MUTE2#", (eAVSwitch::getInstance()->getMute()) ? "1" : "0");

	// channel stats
	result.strReplace("#DOLBY#", (eZapMain::getInstance()->getAC3Logo()) ? "1" : "0");
	result.strReplace("#CRYPT#", (eZapMain::getInstance()->getSmartcardLogo()) ? "1" : "0");
	result.strReplace("#FORMAT#", (eZapMain::getInstance()->get16_9Logo()) ? "1" : "0");

	// recording status
#ifndef DISABLE_FILE
	result.strReplace("#RECORDING2#", (eZapMain::getInstance()->isRecording()) ? "1" : "0");
#else
	result.strReplace("#RECORDING2#", "0");
#endif
	return result;
}

static eString videodata(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result = readFile(TEMPLATE_DIR + "videodata.tmp");

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
		videopos = (current * 20) / total;
	}

	result.strReplace("#VIDEOPOSITION#", eString().sprintf("%d", videopos));
	result.strReplace("#VIDEOTIME#", eString().sprintf("%d:%02d", min, sec));
	result.strReplace("#PLAYSTATUS#", playStatus);

	return result;
}

static eString body(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	content->local_header["Content-Type"]="text/html; charset=utf-8";

	int previousZapMode = zapMode;
	int previousZapSubMode = zapSubMode;

	eString mode = opt["mode"];
	if (!mode)
		mode = "zap";

	eString path = opt["path"];

	if (mode == "zap")
	{
		eString zapModeS = opt["zapmode"];
		if (zapModeS)
			zapMode = atoi(zapModeS.c_str());

		eString zapSubModeS = opt["zapsubmode"];
		if (zapSubModeS)
			zapSubMode = atoi(zapSubModeS.c_str());

		eString curBouquet = opt["curBouquet"];
		if (curBouquet)
			currentBouquet = atoi(curBouquet.c_str());

		eString curChannel = opt["curChannel"];
		if (curChannel)
			currentChannel = atoi(curChannel.c_str());

		if ((zapMode >= 0) && (zapMode <= 4) && (zapSubMode >= 0) && (zapSubMode <= 4))
		{
			if (!path)
				path = zap[zapMode][zapSubMode];
		}
		else
		{
			zapMode = ZAPMODETV;
			zapSubMode = ZAPSUBMODEBOUQUETS;
			path = zap[zapMode][zapSubMode];
		}

		if (zapMode != previousZapMode || zapSubMode != previousZapSubMode)
		{
			currentBouquet = 0;
			currentChannel = -1;
		}

		result = getContent(mode, path, opts);
	}
	else
	{
		result = readFile(TEMPLATE_DIR + "index2.tmp");
		eString tmp = getContent(mode, path, opts);
		if (tmp)
			result.strReplace("#CONTENT#", tmp);
		else
			result = "";

		if (mode == "controlSatFinder")
			result.strReplace("#ONLOAD#", "onLoad=init()");
		else
			result.strReplace("#ONLOAD#", "");
	}

	if (!result)
		result = closeWindow(content, "Please wait...", 3000);

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
	dyn_resolver->addDyn("GET", "/pda", pda_root, lockWeb);

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
	dyn_resolver->addDyn("GET", "/addTimerEvent", addTimerEvent, lockWeb);
	dyn_resolver->addDyn("GET", "/TVBrowserTimerEvent", TVBrowserTimerEvent, lockWeb);
	dyn_resolver->addDyn("GET", "/deleteTimerEvent", deleteTimerEvent, lockWeb);
	dyn_resolver->addDyn("GET", "/showEditTimerEventWindow", showEditTimerEventWindow, lockWeb);
	dyn_resolver->addDyn("GET", "/showAddTimerEventWindow", showAddTimerEventWindow, lockWeb);
	dyn_resolver->addDyn("GET", "/changeTimerEvent", changeTimerEvent, lockWeb);
	dyn_resolver->addDyn("GET", "/cleanupTimerList", cleanupTimerList, lockWeb);
	dyn_resolver->addDyn("GET", "/clearTimerList", clearTimerList, lockWeb);
	dyn_resolver->addDyn("GET", "/EPGDetails", EPGDetails, lockWeb);
	dyn_resolver->addDyn("GET", "/msgWindow", msgWindow, lockWeb);
	dyn_resolver->addDyn("GET", "/tvMessageWindow", tvMessageWindow, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/status", doStatus, true); //always pw protected for dreamtv
	dyn_resolver->addDyn("GET", "/cgi-bin/switchService", switchService, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/zapTo", zapTo, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/admin", admin, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/audio", audio, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/selectAudio", selectAudio, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/setAudio", setAudio, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/selectSubChannel", selectSubChannel, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/getPMT", getPMT, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/getEIT", getEIT, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/message", message, lockWeb);
	dyn_resolver->addDyn("GET", "/control/message", message, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/xmessage", xmessage, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/rc", remoteControl, lockWeb);
	dyn_resolver->addDyn("GET", "/showRemoteControl", showRemoteControl, lockWeb);
	dyn_resolver->addDyn("GET", "/satFinder", satFinder, lockWeb);
	dyn_resolver->addDyn("GET", "/audio.m3u", audiom3u, lockWeb);
	dyn_resolver->addDyn("GET", "/video.pls", videopls, lockWeb);
	dyn_resolver->addDyn("GET", "/version", version, lockWeb);
//	dyn_resolver->addDyn("GET", "/header", header, lockWeb);
	dyn_resolver->addDyn("GET", "/body", body, lockWeb);
	dyn_resolver->addDyn("GET", "/videodata", videodata, lockWeb);
	dyn_resolver->addDyn("GET", "/data", data, lockWeb);
	dyn_resolver->addDyn("GET", "/blank", blank, lockWeb);
	dyn_resolver->addDyn("GET", "/leftnavi", leftnavi, lockWeb);
	dyn_resolver->addDyn("GET", "/channavi", channavi, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/getcurrentepg", getcurepg, lockWeb);
	dyn_resolver->addDyn("GET", "/getcurrentepg", getcurepg, lockWeb);
	dyn_resolver->addDyn("GET", "/getMultiEPG", getMultiEPG, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/streaminfo", getstreaminfo, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/channelinfo", getchannelinfo, lockWeb);
	dyn_resolver->addDyn("GET", "/channels/getcurrent", channels_getcurrent, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadSettings", reload_settings, lockWeb);
#ifndef DISABLE_FILE
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadRecordings", load_recordings, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/saveRecordings", save_recordings, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/recoverRecordings", recoverRecordings, lockWeb);
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
// functions needed by dreamtv
	dyn_resolver->addDyn("GET", "/cgi-bin/audioChannels", audioChannels, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/videoChannels", videoChannels, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/currentTransponderServices", getTransponderServices, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/getServices", getServices, lockWeb);
//	dyn_resolver->addDyn("GET", "/cgi-bin/getChannels", getChannels, lockWeb);
	dyn_resolver->addDyn("GET", "/control/zapto", getCurrentVpidApid, false); // this dont really zap.. only used to return currently used pids;
	dyn_resolver->addDyn("GET", "/control/getonidsid", neutrino_getonidsid, lockWeb);
	dyn_resolver->addDyn("GET", "/control/channellist", neutrino_getchannellist, lockWeb);
	ezapWapInitializeDyn(dyn_resolver, lockWeb);
#ifdef ENABLE_DYN_MOUNT
	ezapMountInitializeDyn(dyn_resolver, lockWeb);
#endif
#ifdef ENABLE_DYN_CONF
	ezapConfInitializeDyn(dyn_resolver, lockWeb);
#endif
#ifdef ENABLE_DYN_FLASH
	ezapFlashInitializeDyn(dyn_resolver, lockWeb);
#endif
#ifdef ENABLE_DYN_ROTOR
	ezapRotorInitializeDyn(dyn_resolver, lockWeb);
#endif
#ifdef ENABLE_DYN_XML
	ezapXMLInitializeDyn(dyn_resolver, lockWeb);
#endif
}


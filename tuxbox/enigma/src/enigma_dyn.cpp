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
#include <lib/movieplayer/movieplayer.h>
#include <enigma_dyn.h>
#include <enigma_dyn_utils.h>
#include <enigma_dyn_mount.h>
#include <enigma_dyn_wap.h>
#include <enigma_dyn_conf.h>
#include <enigma_dyn_flash.h>
#include <enigma_dyn_rotor.h>
#include <enigma_dyn_xml.h>
#include <enigma_dyn_misc.h>
#include <enigma_dyn_epg.h>
#include <enigma_dyn_timer.h>
#include <enigma_dyn_pda.h>
#include <enigma_dyn_movieplayer.h>
#include <enigma_streamer.h>
#include <enigma_processutils.h>
#include <epgwindow.h>
#include <streaminfo.h>
#include <enigma_mount.h>

using namespace std;

#define KEYBOARDTV 0
#define KEYBOARDVIDEO 1

int keyboardMode = KEYBOARDTV;

int pdaScreen = 0;
int screenWidth = 1024;
eString lastTransponder;

int currentBouquet = 0;
int currentChannel = -1;

int zapMode = ZAPMODETV;
int zapSubMode = ZAPSUBMODEBOUQUETS;
eString zapSubModes[6] = {"Name", "Category", "Satellites", "Providers", "Bouquets", "All Services"};

eString zap[6][6] =
{
	{"TV", "0:7:1:0:0:0:0:0:0:0:", /* Satellites */ "1:15:fffffffc:12:0:0:0:0:0:0:", /* Providers */ "1:15:ffffffff:12:ffffffff:0:0:0:0:0:", /* Bouquets */ "4097:7:0:6:0:0:0:0:0:0:", /* All */ "1:15:fffffffe:12:ffffffff:0:0:0:0:0:"},
	{"Radio", "0:7:2:0:0:0:0:0:0:0:", /* Satellites */ "1:15:fffffffc:4:0:0:0:0:0:0:", /* Providers */ "1:15:ffffffff:4:ffffffff:0:0:0:0:0:", /* Bouquets */ "4097:7:0:4:0:0:0:0:0:0:", /* All */ "1:15:fffffffe:4:ffffffff:0:0:0:0:0:"},
	{"Data", "0:7:6:0:0:0:0:0:0:0:", /* Satellites */ "1:15:fffffffc:ffffffe9:0:0:0:0:0:0:", /* Providers */ "1:15:ffffffff:ffffffe9:ffffffff:0:0:0:0:0:", /* Bouquets */ "", /* All */ ""},
	{"Movies", "4097:7:0:1:0:0:0:0:0:0:", /* Satellites */ "", /* Providers */ "", /* Bouquets */ "", /* All */ ""},
	{"Root", "2:47:0:0:0:0:/", /* Satellites */ "", /* Providers */ "", /* Bouquets */ "", /* All */ ""},
	{"Stream", "", /* Satellites */ "", /* Providers */ "", /* Bouquets */ "", /* All */ ""}
};

extern bool onSameTP(const eServiceReferenceDVB& ref1, const eServiceReferenceDVB &ref2); // implemented in timer.cpp
extern bool canPlayService(const eServiceReference & ref); // implemented in timer.cpp

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

static eString tvMessageWindow(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	return readFile(TEMPLATE_DIR + "sendMessage.tmp");
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
	eString display = opt["display"];
	if (display == "")
		display = "yes";

	if (getOSDShot(opt["mode"]) == 0)
	{
		if (display == "yes")
		{
			content->local_header["Location"]="/root/tmp/osdshot.png";
			content->code = 307;
			return "+ok";
		}
		else
			return closeWindow(content, "", 500);
	}
	else
		return "-error";
}

bool playService(const eServiceReference &ref)
{
	// ignore locked service
	if (ref.isLocked() && eConfig::getInstance()->pLockActive())
		return false;
	eZapMain::getInstance()->playService(ref, eZapMain::psSetMode|eZapMain::psDontAdd);
	return true;
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
	eString result;
	if (eSystemInfo::getInstance()->canShutdown())
		result =  "Unknown admin command. (valid commands are: shutdown, reboot, restart, standby, wakeup)";
	else
		result =  "Unknown admin command. (valid commands are: reboot, restart, standby, wakeup)";
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
	eString sReference = opt["sref"];
	eServiceReference sref = string2ref(sReference);
	eString command = opt["command"];
#ifdef ENABLE_DYN_STREAM
	if (eMoviePlayer::getInstance()->getStatus())
	{
		eMoviePlayer::getInstance()->control(command.c_str(), "");
	}
	else
#endif
	{
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
			if (eZapMain::getInstance()->isRecording())
			{
				if (eSystemInfo::getInstance()->getDefaultTimerType() == ePlaylistEntry::RecTimerEntry|ePlaylistEntry::recDVR)
					eZapMain::getInstance()->recordDVR(0,0);
				else
				if (eSystemInfo::getInstance()->getDefaultTimerType() == ePlaylistEntry::RecTimerEntry|ePlaylistEntry::recNgrab)
					eZapMain::getInstance()->stopNGrabRecord();
			}
			else
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
			eString curChannel = opt["curChannel"];
			if (curChannel)
			{
				currentChannel = atoi(curChannel.c_str());
				currentBouquet = 0;
			}
			if (sref)
			{
				if (eServiceInterface::getInstance()->service == sref)
					eZapMain::getInstance()->play();
				else
					playService(sref);
			}
			else
				eZapMain::getInstance()->play();
		}
		else
		if (command == "record")
		{
			if (eSystemInfo::getInstance()->getDefaultTimerType() == ePlaylistEntry::RecTimerEntry|ePlaylistEntry::recDVR)
				eZapMain::getInstance()->recordDVR(1,0);
			else
			if (eSystemInfo::getInstance()->getDefaultTimerType() == ePlaylistEntry::RecTimerEntry|ePlaylistEntry::recNgrab)
				eZapMain::getInstance()->startNGrabRecord();
		}
	}

	return closeWindow(content, "", 500);
}
#endif

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

			result += it->text;
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
									subChannel = subService;
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
			int p = 0;
			for (ePtrList<EITEvent>::iterator i(eit->events); i != eit->events.end(); ++i)
			{
				EITEvent *event = *i;
				if ((event->running_status >= 2) || ((!p) && (!event->running_status)))
				{
					for (ePtrList<Descriptor>::iterator d(event->descriptor); d != event->descriptor.end(); ++d)
					{
						if (d->Tag() == DESCR_LINKAGE)
						{
							LinkageDescriptor *ld = (LinkageDescriptor *)*d;
							if (ld->linkage_type == 0xB0) //subchannel
							{
								eString subService((char *)ld->private_data, ld->priv_len);
								eString subServiceRef = "1:0:7:" + eString().sprintf("%x", ld->service_id) + ":" + eString().sprintf("%x", ld->transport_stream_id) + ":" + eString().sprintf("%x", ld->original_network_id) + ":"
									+ eString(nspace) + ":0:0:0:";
								if (subServiceRef == curServiceRef)
									result += "<option selected value=\"" + subServiceRef + "\">";
								else
									result += "<option value=\"" + subServiceRef + "\">";
								result += subService;
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
	content->local_header["Content-Type"] = "text/html; charset=utf-8";
	content->local_header["Cache-Control"] = "no-cache";
	eString subChannels = getSubChannels();

	eString result = readFile(TEMPLATE_DIR + "subChannelSelection.tmp");
	result.strReplace("#SUBCHANS#", subChannels);

	return result;
}

eString getCurService(void)
{
	eString result;

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		eService *current = eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
		if (current)
			result = current->service_name;
	}
	return filter_string(result);
}

eString getChanNavi()
{
	eString result;
	result += button(100, "Audio", YELLOW, "javascript:selectAudio()", "#FFFFFF");
	result += button(100, "Video", GREEN, "javascript:selectSubChannel()", "#FFFFFF");
	result += button(100, "EPG", RED, "javascript:openEPG(\'\')", "#FFFFFF");
	result += button(100, "Info", BLUE, "javascript:openChannelInfo()", "#FFFFFF");
	result += button(100, "Stream Info", TOPNAVICOLOR, "javascript:openSI()", "#000000");
	result += button(100, "VLC", TOPNAVICOLOR, "javascript:vlc()", "#000000");
	return result;
}

eString getTopNavi()
{
	eString result;
	eString pre, post;

	if (pdaScreen == 0)
	{
		pre = "javascript:topnavi('";
		post = "')";
	}

	result += button(100, "ZAP", TOPNAVICOLOR, pre + "?mode=zap" + post);
	result += button(100, "TIMERS", TOPNAVICOLOR, pre + "?mode=timers" + post);
	result += button(100, "CONTROL", TOPNAVICOLOR, pre + "?mode=control" + post);
	if (pdaScreen == 0)
	{
#if ENABLE_DYN_MOUNT || ENABLE_DYN_CONF || ENABLE_DYN_FLASH
		result += button(100, "CONFIG", TOPNAVICOLOR, pre + "?mode=config" + post);
#endif
	}
	result += button(100, "HELP", TOPNAVICOLOR, pre + "?mode=help" + post);

	return result;
}

eString getLeftNavi(eString mode)
{
	eString result;
	eString pre, post;

	if (pdaScreen == 0)
	{
		pre = "javascript:leftnavi('";
		post = "')";
	}

	if (mode.find("zap") == 0)
	{
		if (pdaScreen == 0)
		{
			result += button(110, "TV", LEFTNAVICOLOR, pre + "?mode=zap&zapmode=" + eString().sprintf("%d", ZAPMODETV) + "&zapsubmode=" + eString().sprintf("%d", ZAPSUBMODEBOUQUETS) + post, "#000000");
			result += "<br>";
			result += button(110, "Radio", LEFTNAVICOLOR, pre + "?mode=zap&zapmode=" + eString().sprintf("%d", ZAPMODERADIO) + "&zapsubmode=" + eString().sprintf("%d", ZAPSUBMODEBOUQUETS) + post, "#000000");
			result += "<br>";
			result += button(110, "Data", LEFTNAVICOLOR, pre + "?mode=zap&zapmode=" + eString().sprintf("%d", ZAPMODEDATA) + "&zapsubmode=" + eString().sprintf("%d", ZAPSUBMODESATELLITES) + post, "#000000");
			result += "<br>";
#ifndef DISABLE_FILE
			result += button(110, "Movies", LEFTNAVICOLOR, pre + "?mode=zap&zapmode=" + eString().sprintf("%d", ZAPMODERECORDINGS) + "&zapsubmode=" + eString().sprintf("%d", ZAPSUBMODECATEGORY) + post, "#000000");
			result += "<br>";
			result += button(110, "Root", LEFTNAVICOLOR, pre + "?mode=zap&zapmode=" + eString().sprintf("%d", ZAPMODEROOT) + "&zapsubmode=" + eString().sprintf("%d", ZAPSUBMODECATEGORY) + post, "#000000");
#endif
#ifdef ENABLE_DYN_STREAM
			result += "<br>";
			result += button(110, "Stream", LEFTNAVICOLOR, pre + "?mode=zap&zapmode=" + eString().sprintf("%d", ZAPMODESTREAMING) + "&zapsubmode=" + eString().sprintf("%d", ZAPSUBMODECATEGORY) + post, "#000000");
#endif
			result += "<br><br>";
			if (zap[zapMode][ZAPSUBMODEALLSERVICES])
			{
				result += button(110, "All Services", RED, pre + "?mode=zap&zapmode=" + eString().sprintf("%d", zapMode) + "&zapsubmode=" + eString().sprintf("%d", ZAPSUBMODEALLSERVICES) + post, "#FFFFFF");
				result += "<br>";
			}
			if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite && zap[zapMode][ZAPSUBMODESATELLITES])
			{
				result += button(110, "Satellites", GREEN, pre + "?mode=zap&zapmode=" + eString().sprintf("%d", zapMode) + "&zapsubmode=" + eString().sprintf("%d", ZAPSUBMODESATELLITES) + post, "#FFFFFF");
				result += "<br>";
			}
			if (zap[zapMode][ZAPSUBMODEPROVIDERS])
			{
				result += button(110, "Providers", YELLOW, pre + "?mode=zap&zapmode=" + eString().sprintf("%d", zapMode) + "&zapsubmode=" + eString().sprintf("%d", ZAPSUBMODEPROVIDERS) + post, "#FFFFFF");
				result += "<br>";
			}
			if (zap[zapMode][ZAPSUBMODEBOUQUETS])
			{
				result += button(110, "Bouquets", BLUE, pre + "?mode=zap&zapmode=" + eString().sprintf("%d", zapMode) + "&zapsubmode=" + eString().sprintf("%d", ZAPSUBMODEBOUQUETS) + post, "#FFFFFF");
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
			result += button(110, "Shutdown", LEFTNAVICOLOR, "javascript:admin(\'shutdown\')");
			result += "<br>";
		}
		result += button(110, "Restart", LEFTNAVICOLOR, "javascript:admin(\'restart\')");
		result += "<br>";
		result += button(110, "Reboot", LEFTNAVICOLOR, "javascript:admin(\'reboot\')");
		result += "<br>";
		result += button(110, "Standby", LEFTNAVICOLOR, "javascript:admin(\'standby\')");
		result += "<br>";
		result += button(110, "Wakeup", LEFTNAVICOLOR, "javascript:admin(\'wakeup\')");
		result += "<br>";
		result += button(110, "OSDshot", LEFTNAVICOLOR, pre + "?mode=controlFBShot" + post);
#ifndef DISABLE_LCD
		result += "<br>";
		result += button(110, "LCDshot", LEFTNAVICOLOR, pre + "?mode=controlLCDShot" + post);
#endif
		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000)
		{
			result += "<br>";
			result += button(110, "Screenshot", LEFTNAVICOLOR, pre + "?mode=controlScreenShot" + post);
		}
		result += "<br>";
		result += button(110, "Message", LEFTNAVICOLOR, "javascript:sendMessage2TV()");
		result += "<br>";
#ifdef DEBUG
		result += button(110, "Logging", LEFTNAVICOLOR, "javascript:logging()");
		result += "<br>";
#endif
		result += button(110, "Satfinder", LEFTNAVICOLOR, pre + "?mode=controlSatFinder" + post);
		switch (eSystemInfo::getInstance()->getHwType())
		{
			case eSystemInfo::dbox2Nokia:
			case eSystemInfo::dbox2Sagem:
			case eSystemInfo::dbox2Philips:
				result += "<br>";
				result += button(110, "Remote Control", LEFTNAVICOLOR, "javascript:remoteControl('dbox2')");
				break;
			default:
				if (eSystemInfo::getInstance()->hasKeyboard())
				    result += "<br>"+button(110, "Remote Control", LEFTNAVICOLOR, "javascript:remoteControl('dreambox')");
				break;
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
	if (mode.find("help") == 0)
	{
		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000)
		{
			result += button(110, "DMM Sites", LEFTNAVICOLOR, pre + "?mode=helpDMMSites" + post);
			result += "<br>";
			result += button(110, "Other Sites", LEFTNAVICOLOR, pre + "?mode=helpOtherSites" + post);
			result += "<br>";
		}
		result += button(110, "Boards", LEFTNAVICOLOR, pre + "?mode=helpForums" + post);
		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000)
		{
			result += "<br><br>";
			result += button(110, "Images", LEFTNAVICOLOR, pre + "?mode=helpUpdatesInternet" + post);
		}
	}

	result += "&nbsp;";
	return result;
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
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSkip, skipTime * 376));
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSeekEnd));
		}
	}

	return closeWindow(content, "", 500);
}

eString getBoxInfo(eString, eString);
static eString getTimers()
{
	return getTimerList("HTML");
}

static eString getHelpUpdatesInternet()
{
        eString versionFile = "/.version";

        if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7020)
                versionFile = "/etc/image-version";

	std::stringstream result;
	eString imageName = "&nbsp;", imageVersion = "&nbsp;", imageURL = "&nbsp;", imageCreator = "&nbsp;", imageMD5 = "&nbsp;";
	eString myCatalogURL = getAttribute(versionFile, "catalog");

	if (!(eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7020) && myCatalogURL.length())
	{
		system(eString("wget -q -O /tmp/catalog.xml " + myCatalogURL).c_str());
		ifstream catalogFile("/tmp/catalog.xml");
		if (catalogFile)
		{
			result  << "<table id=\"epg\" width=\"100%\" border=\"1\" cellpadding=\"5\" cellspacing=\"0\">"
				<< "<thead>"
				<< "<tr>"
				<< "<th colspan=\"2\">Available Images</th>"
				<< "</tr>"
				<< "</thead>"
				<< "<tbody>";
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
			result 	<< "</tbody>"
				<< "</table>";
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

	eString sref = opt["ref"];
	eServiceReference ref = string2ref(sref);
	ePlaylist *recordings = eZapMain::getInstance()->getRecordings();

	if (ref.path.right(3).upper() == ".TS")
	{
		for (std::list<ePlaylistEntry>::iterator it(recordings->getList().begin()); it != recordings->getList().end(); ++it) 
		{
			if (it->service.path == ref.path)
			{
				recordings->getList().erase(it);
				recordings->save();
				break;
			}
		}
		eString filename = ref.path;
		filename.erase(filename.length() - 2, 2);
		filename += "eit";
		eDebug("[ENIGMA_DYN] deleting %s", filename.c_str());
		remove(filename.c_str());
		eDebug("[ENIGMA_DYN] deleting %s", eString(ref.path + ".indexmarks").c_str());
		remove((ref.path + ".indexmarks").c_str());
		
		int slice = 0;
		while (1)
		{
			filename = ref.path;
			if (slice)
				filename += eString().sprintf(".%03d", slice);
			slice++;
			struct stat64 s;
			if (::stat64(filename.c_str(), &s) < 0)
				break;
			eDebug("[ENIGMA_DYN] deleting %s", filename.c_str());
			eBackgroundFileEraser::getInstance()->erase(filename.c_str());
		}
	}
	return closeWindow(content, "Please wait...", 2000);
}
#endif

class myService
{
public:
	eString serviceRef;
	eString serviceName;
	myService(eString sref, eString sname)
	{
		serviceRef = sref;
		serviceName = sname;
	};
	~myService() {};
	bool operator < (const myService &a) const {return serviceName < a.serviceName;}
};

void genHTMLServicesList(std::list <myService> &myList, eString &serviceRefList, eString &serviceList)
{
	std::list <myService>::iterator myIt;
	eString serviceRef, serviceName;

	serviceRefList = "";
	serviceList = "";

	for (myIt = myList.begin(); myIt != myList.end(); ++myIt)
	{
		serviceRefList += "\"" + myIt->serviceRef + "\", ";
		serviceList += "\"" + myIt->serviceName + "\", ";
	}
	
	serviceRefList = serviceRefList.left(serviceRefList.length() - 2);
	serviceList = serviceList.left(serviceList.length() - 2);
}

class eWebNavigatorListDirectory2: public Object
{
	std::list <myService> &myList;
	eString path;
	eServiceInterface &iface;
	bool addEPG;
	bool forceAll;
public:
	eWebNavigatorListDirectory2(std::list <myService> &myList, eString path, eServiceInterface &iface, bool addEPG, bool forceAll): myList(myList), path(path), iface(iface), addEPG(addEPG), forceAll(forceAll)
	{
//		eDebug("[eWebNavigatorListDirectory2:] path: %s", path.c_str());
	}
	void addEntry(const eServiceReference &e)
	{
		// sorry.. at moment we dont show any directory.. or locked service in webif
		if (e.isLocked() && eConfig::getInstance()->pLockActive())
			return;
#ifndef DISABLE_FILE
		if (!forceAll)
		{
			if (eDVB::getInstance()->recorder && !e.path && !e.flags)
			{
				if (!onSameTP(eDVB::getInstance()->recorder->recRef,(eServiceReferenceDVB&)e))
					return;
			}
		}
#endif
		eString short_description, event_start, event_duration;
		
		eEPGCache::getInstance()->Lock();
		eServiceReferenceDVB &ref = (eServiceReferenceDVB&)e;
		const timeMap* evt = eEPGCache::getInstance()->getTimeMap(ref);
		if (evt)
		{
			int tsidonid = (ref.getTransportStreamID().get()<<16)|ref.getOriginalNetworkID().get();
			timeMap::const_iterator It;
			for (It = evt->begin(); (It != evt->end() && !short_description); ++It)
			{
				EITEvent event(*It->second,tsidonid);
				time_t now = time(0) + eDVB::getInstance()->time_difference;
				if ((now >= event.start_time) && (now <= event.start_time + event.duration))
				{
					LocalEventData led;
					led.getLocalData(&event, &short_description);
					tm t = *localtime(&event.start_time);
					event_start = eString().sprintf("%02d:%02d", t.tm_hour, t.tm_min);
					event_duration = eString().sprintf("%d", event.duration / 60);
				}
			}
		}
		eEPGCache::getInstance()->Unlock();
		
		eString tmp;
		if (ref.descr)
			tmp = filter_string(ref.descr);
		else
		{
			eService *service = iface.addRef(e);
			if (service)
			{
				tmp = filter_string(service->service_name);
				iface.removeRef(e);
			}
		}
		
		if (short_description && addEPG)
			tmp = tmp + " - " + event_start + " (" + event_duration + ") " + filter_string(short_description);
		tmp.strReplace("\"", "'");
		tmp.strReplace("\n", "-");
		
		if (zapMode == ZAPMODERECORDINGS)
		{
			eString r = e.toString();
			tmp = "[" + eString().sprintf("%05lld", getMovieSize(r.right(r.length() - r.find("/hdd/movie"))) / 1024 / 1024) + " MB] " + tmp;
		}

		if (!(e.data[0] == -1 && e.data[2] != (int)0xFFFFFFFF) && tmp)
			myList.push_back(myService(ref2string(e), tmp));
	}
};

eString getZapContent(eString path, int depth, bool addEPG, bool sortList, bool forceAll)
{
	std::list <myService> myList, myList2;
	std::list <myService>::iterator myIt;
	eString result, result1, result2;
	eString bouquets, bouquetrefs, channels, channelrefs;

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
		myList.clear();
		eWebNavigatorListDirectory2 navlist(myList, path, *iface, addEPG, forceAll);
		Signal1<void, const eServiceReference&> signal;
		signal.connect(slot(navlist, &eWebNavigatorListDirectory2::addEntry));
		iface->enterDirectory(current_service, signal);
//		eDebug("entered");
		iface->leaveDirectory(current_service);
//		eDebug("exited");

		if (sortList)
			myList.sort();
			
		genHTMLServicesList(myList, result1, result2);
		bouquetrefs = result1;
		bouquets = result2;
		
		if (depth > 1)
		{
			// go thru all bouquets to get the channels
			int i = 0;
			for (myIt = myList.begin(); myIt != myList.end(); myIt++)
			{
				result1 = ""; result2 = "";
				path = myIt->serviceRef;
				if (path)
				{
					eServiceReference current_service = string2ref(path);

					myList2.clear();
					eWebNavigatorListDirectory2 navlist(myList2, path, *iface, addEPG, forceAll);
					Signal1<void, const eServiceReference&> signal;
					signal.connect(slot(navlist, &eWebNavigatorListDirectory2::addEntry));

					iface->enterDirectory(current_service, signal);
//					eDebug("entered");
					iface->leaveDirectory(current_service);
//					eDebug("exited");
	
					if (sortList)
						myList2.sort();
						
					genHTMLServicesList(myList2, result1, result2);

					channels += "channels[";
					channels += eString().sprintf("%d", i);
					channels += "] = new Array(";
					channelrefs += "channelRefs[";
					channelrefs += eString().sprintf("%d", i);
					channelrefs += "] = new Array(";
					
					channels += result2;
					channels += ");";
					channelrefs += result1;
					channelrefs += ");";
					
					channels += "\n";
					channelrefs += "\n";
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

static eString getZap(eString path)
{
	eString result, tmp;
	int selsize = 0;

	if (pdaScreen == 0)
	{
#ifndef DISABLE_FILE
		if (zapMode == ZAPMODERECORDINGS) // recordings
		{
			result = readFile(TEMPLATE_DIR + "movies.tmp");
			result.strReplace("#ZAPDATA#", getZapContent(path, 1, false, false, false));
			selsize = (screenWidth > 1024) ? 25 : 10;
#ifdef ENABLE_DYN_MOUNT
			tmp = readFile(TEMPLATE_DIR + "movieSources.tmp");
			tmp.strReplace("#OPTIONS#", eMountMgr::getInstance()->listMovieSources());
#endif
			result.strReplace("#MOVIESOURCES#", tmp);
			tmp = button(100, "Delete", RED, "javascript:deleteMovie()", "#FFFFFF");
			result.strReplace("#DELETEBUTTON#", tmp);
			tmp = button(100, "Download", GREEN, "javascript:downloadMovie()", "#FFFFFF");
			result.strReplace("#DOWNLOADBUTTON#", tmp);
			tmp = button(100, "VLC", YELLOW, "javascript:streamMovie()", "#FFFFFF");
			result.strReplace("#STREAMBUTTON#", tmp);
			tmp = button(100, "Recover", BLUE, "javascript:recoverMovies()", "#FFFFFF");
			result.strReplace("#RECOVERBUTTON#", tmp);
		}
		else
#endif
		if (zapMode == ZAPMODEROOT) // root
		{
			result = readFile(TEMPLATE_DIR + "root.tmp");
			eString tmp = getZapContent(path, 1, false, false, false);
			if (tmp)
			{
				result.strReplace("#ZAPDATA#", tmp);
				selsize = (screenWidth > 1024) ? 25 : 10;
			}
			else
				result = "";
		}
		else
		if (zapMode == ZAPMODESTREAMING)
		{
			result = getStreamingServer();
		}
		else
		{
			result = readFile(TEMPLATE_DIR + "zap.tmp");
			bool sortList = (zapSubMode ==  ZAPSUBMODESATELLITES || zapSubMode == ZAPSUBMODEPROVIDERS || zapSubMode == ZAPSUBMODEALLSERVICES);
			int columns = (zapSubMode == ZAPSUBMODEALLSERVICES) ? 1 : 2;
			result.strReplace("#ZAPDATA#", getZapContent(path, columns, true, sortList, false));
			selsize = (screenWidth > 1024) ? 30 : 15;
			if (columns == 1)
			{
				result.strReplace("#WIDTH1#", "0");
				result.strReplace("#WIDTH2#", "630");
			}
			else
			{
				result.strReplace("#WIDTH1#", "200");
				result.strReplace("#WIDTH2#", "430");
			}
			tmp = button(100, "EPG-Overview", RED, "javascript:mepg()", "#FFFFFF");
			result.strReplace("#MEPGBUTTON#", tmp);
		}
		result.strReplace("#SELSIZE#", eString().sprintf("%d", selsize));
	}
	else
	{
		eString tmp = getPDAZapContent(path);
		result = (tmp) ? getEITC(readFile(TEMPLATE_DIR + "eit_small.tmp")) + tmp : "";
	}

	return result;
}

#ifndef DISABLE_FILE
eString getDiskInfo(void)
{
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
	}
	return sharddisks;
}

eString getUSBInfo(void)
{
	eString usbStick = "none";
	eString line;
	ifstream infile("/proc/scsi/usb-storage/0");
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
		}
	}
	return usbStick;
}
#endif

eString getBoxInfo(eString skelleton, eString format)
{
        eString versionFile = "/.version";

        if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7020)
                versionFile = "/etc/image-version";

	eString result = readFile(TEMPLATE_DIR + format + skelleton + ".tmp");

	result.strReplace("#VERSION#", getAttribute(versionFile, "version"));
	result.strReplace("#CATALOG#", getAttribute(versionFile, "catalog"));
	result.strReplace("#COMMENT#", getAttribute(versionFile, "comment"));
	result.strReplace("#URL#", getAttribute(versionFile, "url"));

	result.strReplace("#MODEL#", eSystemInfo::getInstance()->getModel());
	result.strReplace("#MANUFACTURER#", eSystemInfo::getInstance()->getManufacturer());
	result.strReplace("#PROCESSOR#", eSystemInfo::getInstance()->getCPUInfo());
#ifndef DISABLE_FILE
	result.strReplace("#DISK#", getDiskInfo());
	result.strReplace("#USBSTICK#", getUSBInfo());
#else
	result.strReplace("#DISK#", "none");
	result.strReplace("#USBSTICK#", "none");
#endif
	result.strReplace("#LINUXKERNEL#", readFile("/proc/version"));
	result.strReplace("#FIRMWARE#", firmwareLevel(getAttribute(versionFile, "version")));
	if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000 ||
		eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7020)
		result.strReplace("#FP#", eString().sprintf(" 1.%02d", eDreamboxFP::getFPVersion()));
	else
		result.strReplace("#FP#", "n/a");
	result.strReplace("#WEBIFVERSION#", WEBIFVERSION);

	return result;
}

#ifndef	DISABLE_FILE
// Recover index with recordings on harddisk in /hdd/movie.
bool rec_movies()
{
	eString filen;
	int i;
	bool result = false;

	eZapMain::getInstance()->loadRecordings();
	ePlaylist *recordings = eZapMain::getInstance()->getRecordings();
	std::list<ePlaylistEntry>& rec_list = recordings->getList();
	struct dirent **namelist;
	int n = scandir("/hdd/movie", &namelist, 0, alphasort);

	if (n > 0)
	{
		// There will be 2 passes through recordings list:
		// 1) Delete entries from list that are not on disk.
		// 2) Add entries to list that do not exist yet.
	
		// Pass 1
		std::list<ePlaylistEntry>::iterator it(rec_list.begin());
		std::list<ePlaylistEntry>::iterator it_next;
		while (it != rec_list.end()) 
		{
			bool valid_file = false;
			// For every file in /hdd/movie
			for (i = 0; i < n; i++)
			{
				filen = namelist[i]->d_name;
				// For every valid file
				if ((filen.length() >= 3) &&
				(filen.substr(filen.length()-3, 3).compare(".ts") == 0) &&
					(it->service.path.length() >= 11) &&
					!it->service.path.substr(11,it->service.path.length()-11).compare(filen))
				{
					valid_file = true;
					break;
				}
			}

			(it_next = it)++;
			if (!valid_file)
				rec_list.erase(it);
			else
			{
				// Trim descr
				if (it->service.descr.find_last_not_of(' ') != eString::npos) 
					it->service.descr = it->service.descr.substr(0, it->service.descr.find_last_not_of(' ') + 1);
			}
			it = it_next;
		}
		
		// Pass 2
		for (i = 0; i < n; i++)
		{
			filen = namelist[i]->d_name;
			// For every .ts file
			if ((filen.length() >= 3) &&
				(filen.substr(filen.length()-3, 3).compare(".ts") == 0))
			{
				// Check if file is in the list.
				bool file_in_list = false;
				for (std::list<ePlaylistEntry>::iterator it(rec_list.begin()); it != rec_list.end(); ++it)
				{
					if ((it->service.path.length() >= 11) &&
						!it->service.path.substr(11,it->service.path.length()-11).compare(filen))
					{
						file_in_list = true;
						break;
					}
				}
				if (!file_in_list)	// Add file to list.
				{
					eServicePath path("1:0:1:0:0:0:000000:0:0:0:/hdd/movie/" + filen);
					rec_list.push_back(path);
					rec_list.back().type = 16385;
					rec_list.back().service.descr = filen.substr(0, filen.find(".ts"));
					rec_list.back().service.path = "/hdd/movie/" + filen;
				}
			}
			free(namelist[i]);
		}
		rec_list.sort();
		result = true;
	}
	
	recordings->save();
	free(namelist);

	return result;
}

static eString recoverRecordings(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString result;
	if (rec_movies())
		result = "<html><head><title>Info</title></head><body onUnload=\"parent.window.opener.location.reload(true)\">Movies recovered successfully.</body></html>";
	else
		result = "<html><head><title>Info</title></head><body onUnload=\"parent.window.opener.location.reload(true)\">Movies could not be recovered.</body></html>";
	return result;
}
#endif

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

#define CLAMP(x)     ((x < 0) ? 0 : ((x > 255) ? 255 : x))

inline unsigned short avg2(unsigned short a, unsigned short b)
{
	return
		(((a & 0xFF) + (b & 0xFF)) >> 1) |
		(((((a>>8) & 0xFF) + ((b>>8) & 0xFF)) >> 1) << 8);
}

struct blasel
{
	int hor, vert;
	char *name;
} subsamplings[]={
	{1, 1, "4:4:4"},
	{2, 1, "4:2:2"},
	{2, 2, "4:2:0"},
	{4, 2, "4:2:0-half"},
	{4, 1, "4:1:1"},
	{4, 4, "4:1:0"}};

int genScreenShot(int index)
{
	unsigned char frame[720 * 576 * 3 + 16]; // max. size

	int fd = open("/dev/video", O_RDONLY);
	if (fd < 0)
	{
		eDebug("could not open /dev/video");
		return 1;
	}
	
	eString filename = "/tmp/screenshot" + ((index > 0) ? eString().sprintf("%d", index) : "") + ".bmp";
	FILE *fd2 = fopen(filename.c_str(), "wr");
	if (fd2 < 0)
	{
		eDebug("could not open %s", filename.c_str());
		return 1;
	}
	
	int genhdr = 1;
	
	int r = read(fd, frame, 720 * 576 * 3 + 16);
	if (r < 16)
	{
		fprintf(stderr, "read failed\n");
		return 1;
	}
	
	int *size = (int*)frame;
	int luma_x = size[0], luma_y = size[1];
	int chroma_x = size[2], chroma_y = size[3];
	
	unsigned char *luma = frame + 16;
	unsigned short *chroma = (unsigned short*)(frame + 16 + luma_x * luma_y);
	
	eDebug("Picture resolution: %dx%d", luma_x, luma_y);
	
	int sub[2] = {luma_x / chroma_x, luma_y / chroma_y};
	
	int ssid;
	char *d = "unknown";
	for (ssid = 0; ssid < (int)(sizeof(subsamplings)/sizeof(*subsamplings)); ++ssid)
		if ((subsamplings[ssid].hor == sub[0]) && (subsamplings[ssid].vert == sub[1]))
		{
			d = subsamplings[ssid].name;
			break;
		}
		
	eDebug("Chroma  subsampling: %s", d);
	
	if (genhdr)
	{
		eDebug("generating bitmap.");
		unsigned char hdr[14 + 40];
		int i = 0;
#define PUT32(x) hdr[i++] = ((x)&0xFF); hdr[i++] = (((x)>>8)&0xFF); hdr[i++] = (((x)>>16)&0xFF); hdr[i++] = (((x)>>24)&0xFF);
#define PUT16(x) hdr[i++] = ((x)&0xFF); hdr[i++] = (((x)>>8)&0xFF);
#define PUT8(x) hdr[i++] = ((x)&0xFF);
		PUT8('B'); PUT8('M');
		PUT32((((luma_x * luma_y) * 3 + 3) &~ 3) + 14 + 40);
		PUT16(0); PUT16(0); PUT32(14 + 40);
		PUT32(40); PUT32(luma_x); PUT32(luma_y);
		PUT16(1);
		PUT16(24);
		PUT32(0); PUT32(0); PUT32(0); PUT32(0); PUT32(0); PUT32(0);
#undef PUT32
#undef PUT16
#undef PUT8
		fwrite(hdr, 1, i, fd2);
	}
	
	int x, y;
	
	for (y=luma_y - 1; y >= 0; --y)
	{
		unsigned char line[luma_x * 3];
		for (x = 0; x < luma_x; ++x)
		{
			int l = luma[y * luma_x + x];
			int c = 0x8080;
			switch (ssid)
			{
			case 0: // 4:4:4
				c = chroma[y * chroma_x + x];
				break;
			case 1: // 4:2:2
				if (!(x & 1))
					c = chroma[y * chroma_x + (x >> 1)];
				else
					c = avg2(chroma[y * chroma_x + (x >> 1)], chroma[y * chroma_x + (x >> 1) + 1]);
				break;
			case 2: // 4:2:0
				if (!((x|y) & 1))
					c = chroma[(y >> 1) * chroma_x + (x >> 1)];
				else if (!(y & 1))
					c = avg2(chroma[(y >> 1) * chroma_x + (x >> 1)], chroma[(y >> 1) * chroma_x + (x >> 1) + 1]);
				else if (!(x & 1))
					c = avg2(chroma[(y >> 1) * chroma_x + (x >> 1)], chroma[((y >> 1) + 1) * chroma_x + (x >> 1)]);
				else
					c = avg2(
						avg2(chroma[(y >> 1) * chroma_x + (x >> 1)], chroma[(y >> 1) * chroma_x + (x >> 1) + 1]),
						avg2(chroma[((y >> 1) + 1) * chroma_x + (x >> 1)], chroma[((y >> 1) + 1) * chroma_x + (x >> 1) + 1]));
				break;
			case 3:	// 4:2:0-half
				if (!(((x >> 1)|y) & 1))
					c = chroma[(y >> 1) * chroma_x + (x >> 2)];
				else if (!(y & 1))
					c = avg2(chroma[(y >> 1) * chroma_x + (x >> 2)], chroma[(y >> 1) * chroma_x + (x >> 2) + 1]);
				else if (!(x & 2))
					c = avg2(chroma[(y >> 1) * chroma_x + (x >> 2)], chroma[((y >> 1) + 1) * chroma_x + (x >> 2)]);
				else
					c = avg2(
						avg2(chroma[(y >> 1) * chroma_x + (x >> 2)], chroma[(y >> 1) * chroma_x + (x >> 2) + 1]),
						avg2(chroma[((y >> 1) + 1) * chroma_x + (x >> 2)], chroma[((y >> 1) + 1) * chroma_x + (x >> 2) + 1]));
				break;
			case 4:	// 4:1:1
				if (!((x >> 1) & 1))
					c = chroma[y * chroma_x + (x >> 2)];
				else
					c = avg2(chroma[y * chroma_x + (x >> 2)], chroma[y * chroma_x + (x >> 2) + 1]);
				break;
			case 5:
				if (!((x >> 1) & 1))
					c = chroma[(y >> 2) * chroma_x + (x >> 2)];
				else
					c = avg2(chroma[(y >> 2) * chroma_x + (x >> 2)], chroma[(y >> 2) * chroma_x + (x >> 2) + 1]);
				break;
			}
			
			signed char cr = (c & 0xFF) - 128;
			signed char cb = (c >> 8) - 128;

			l -= 16;
			
			int r, g, b;
			
			r = 104635 * cr + l * 76310;
			g = -25690 * cb - 53294 * cr + l * 76310;
			b = 132278 * cb + l * 76310;
			
			line[x * 3 + 2] = CLAMP(r >> 16);
			line[x * 3 + 1] = CLAMP(g >> 16);
			line[x * 3 + 0] = CLAMP(b >> 16);
		}
		fwrite(line, 1, luma_x * 3, fd2);
	}
	fclose(fd2);
	return 0;
}

static eString getControlScreenShot(void)
{
	eString result;

	int rc = genScreenShot(0);
	eDebug("rc is %d", rc);
	if (rc != 0)
	{
		eDebug("could not generate /tmp/screenshot.bmp");
	}
	else
	{
		FILE *bitstream = 0;
		int xres = 0, yres = 0, yres2 = 0, aspect = 0, winxres = 630, winyres = 0, rh = 0, rv = 0;
		if (pdaScreen == 1)
			winxres = 160;
		if (Decoder::current.vpid != -1)
			bitstream = fopen("/proc/bus/bitstream", "rt");
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

eString getContent(eString mode, eString path, eString opts)
{
	eString result, tmp;
	lastTransponder = "";

	if (mode == "zap")
	{
		tmp = "ZAP";
		if (pdaScreen == 0)
		{
			if (zapMode >= 0 && zapMode <= 5)
				tmp += ": " + zap[zapMode][ZAPSUBMODENAME];
			if (zapSubMode >= 2 && zapSubMode <= 5)
				tmp += " - " + zapSubModes[zapSubMode];
		}

		result = getTitle(tmp);
		tmp = getZap(path);
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
		if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000
		|| eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7020)
		if (pdaScreen == 0)
			result += "<img src=\"dm7000.jpg\" width=\"630\" border=\"0\"><br><br>";
		else
			result += "<img src=\"dm7000.jpg\" width=\"160\" border=\"0\"><br><br>";
		result += getBoxInfo("BoxInfo", "HTML");
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
	if (mode == "timers")
	{
		result = getTitle("TIMERS");
		result += getTimers();
	}
	else
	if (mode == "helpUpdatesInternet")
	{
		result = getTitle("HELP: Images");
		result += getHelpUpdatesInternet();
	}
	else
	{
		result = getTitle("GENERAL");
		result += mode + " is not available yet";
	}

	return result;
}

static eString audiopls(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="audio/mpegfile";
	
	eString serviceName = "Enigma Audio Stream";
	eService* current;
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		current = eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
		if (current)
			serviceName = filter_string(current->service_name);
	}
	
	eString result = "[playlist]\n";
	result += "File1=";
	result += "http://" + getIP() + ":31343/" + eString().sprintf("%02x\n", Decoder::current.apid);
	result += "Title1=" + serviceName + "\n";
	result += "Length1=-1\n";
	result += "NumberOfEntries=1\n";
	result += "Version=2";
	
	return result;
}

static eString getvideom3u()
{
	eString vpid = eString().sprintf("%04x", Decoder::current.vpid);
	eString apid = eString().sprintf("%04x", Decoder::current.apid);
	eString pmtpid = eString().sprintf("%04x", Decoder::current.pmtpid);
	eString pcrpid = eString().sprintf("%04x", Decoder::current.pcrpid);

	eString apids;	
	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		std::list<eDVBServiceController::audioStream> &astreams(sapi->audioStreams);
		for (std::list<eDVBServiceController::audioStream>::iterator it(astreams.begin())
			;it != astreams.end(); ++it)
		{
			apids += "," + eString().sprintf("%04x", it->pmtentry->elementary_PID);
		}
	}

	return "http://" + getIP() + ":31339/0," + pmtpid + "," + vpid  + apids + "," + pcrpid;
}


static eString videom3u(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eProcessUtils::killProcess("streamts");
	
	content->local_header["Content-Type"]="video/mpegfile";
	content->local_header["Cache-Control"] = "no-cache";

	return getvideom3u();
}


static eString moviem3u(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString,eString> opt = getRequestOptions(opts, '&');

	eString movieRef = httpUnescape(opt["ref"]);
	eString movieFile = movieRef.right(movieRef.length() - movieRef.find("/hdd/movie"));
	
	eProcessUtils::killProcess("streamts");
	
	content->local_header["Content-Type"]="video/mpegfile";
	content->local_header["Cache-Control"] = "no-cache";

	return "http://" + getIP() + ":31342" + movieFile;
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

static eString setStreamingServiceRef(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	eStreamer::getInstance()->setServiceReference(string2ref(opt["sref"]));
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	return closeWindow(content, "", 10);
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
	eString msg = opts["message"];
	if (!msg)
		msg = opt;
	if (!msg)
		msg = "Error: No message text available.";
		
	eZapMain::getInstance()->postMessage(eZapMessage(1, _("External Message"), httpUnescape(msg), 10), 0);
		
	return closeWindow(content, "", 10);
}

static eString zapTo(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString,eString> opt = getRequestOptions(opts, '&');

	eString curBouquet = opt["curBouquet"];
	if (curBouquet)
		currentBouquet = atoi(curBouquet.c_str());
	eString curChannel = opt["curChannel"];
	if (curChannel)
		currentChannel = atoi(curChannel.c_str());

	eServiceReference current_service = string2ref(opt["path"]);

	if (!(current_service.flags&eServiceReference::isDirectory) && current_service)
	{
		eProcessUtils::killProcess("streamts");
		playService(current_service);
	}
	
	return closeWindow(content, "Please wait...", 3000);
}

static eString web_root(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;

	std::map<eString,eString> opt = getRequestOptions(opts, '&');
	content->local_header["Content-Type"]="text/html; charset=utf-8";

	eConfig::getInstance()->getKey("/ezap/webif/screenWidth", screenWidth);
	pdaScreen = (screenWidth < 800) ? 1 : 0;

	if (opts.find("screenWidth") != eString::npos)
	{
		eString sWidth = opt["screenWidth"];
		screenWidth = atoi(sWidth.c_str());
		eConfig::getInstance()->setKey("/ezap/webif/screenWidth", screenWidth);
		pdaScreen = (screenWidth < 800) ? 1 : 0;
	}
	else
	{
		if ((opts.find("mode") == eString::npos) && (opts.find("path") == eString::npos))
			return readFile(TEMPLATE_DIR + "index.tmp");
	}

	if (pdaScreen == 0)
	{
		result = readFile(TEMPLATE_DIR + "index_big.tmp");
		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000)
			result.strReplace("#BOX#", "Dreambox");
		else
			result.strReplace("#BOX#", "dBox");
		if (eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000)
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
		result.strReplace("#CHANNAVI#", getChanNavi());
		result.strReplace("#TOPNAVI#", getTopNavi());
#ifndef DISABLE_FILE
		result.strReplace("#DVRCONTROLS#", readFile(TEMPLATE_DIR + "dvrcontrols.tmp"));
#else
		result.strReplace("#DVRCONTROLS#", "");
#endif
	}
	else
	{
		content->local_header["Cache-Control"] = "no-cache";
		result = getPDAContent(opts);
	}

	return result;
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
		pos = tmp.find(":");
		if (pos != eString::npos)
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
	{
		if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::dbox2Nokia
		 || eSystemInfo::getInstance()->getHwType() == eSystemInfo::dbox2Sagem
		 || eSystemInfo::getInstance()->getHwType() == eSystemInfo::dbox2Philips)
			result = readFile(TEMPLATE_DIR + "remoteControlDbox2.tmp");
		else
		{
			result = readFile(TEMPLATE_DIR + "remoteControl.tmp");
			int osdshotenabled = 1;
			eConfig::getInstance()->getKey("/enigma/osdshotenabled", osdshotenabled);
			result.strReplace("#OSDSHOTENABLED#", eString().sprintf("%d", osdshotenabled));
			if ((access("/tmp/osdshot.png", R_OK) == 0) && (osdshotenabled == 1))
				result.strReplace("#OSDSHOTPNG#", "/root/tmp/osdshot.png");
			else
				result.strReplace("#OSDSHOTPNG#", "trans.gif\" width=\"0\" height=\"0");
		}
	}
	else
	{
		if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::dbox2Nokia
		 || eSystemInfo::getInstance()->getHwType() == eSystemInfo::dbox2Sagem
		 || eSystemInfo::getInstance()->getHwType() == eSystemInfo::dbox2Philips)
			result = readFile(TEMPLATE_DIR + "pdaRemoteControlDbox2.tmp");
		else
			result = readFile(TEMPLATE_DIR + "pdaRemoteControl.tmp");
	}

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

	result.strReplace("#LEFTNAVI#", getLeftNavi(mode));
	return result;
}

static eString webxtv(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result = readFile(TEMPLATE_DIR + "webxtv" + opt["browser"] + ".tmp");
	result.strReplace("#ZAPDATA#", getZapContent(zap[ZAPMODETV][ZAPSUBMODEBOUQUETS], 2, true, false, false));
	return result;
}

#ifndef DISABLE_FILE
extern int freeRecordSpace(void);  // implemented in enigma_main.cpp
#endif

eString getBoxStatus(eString format)
{
	eString result = readFile(TEMPLATE_DIR + format + "Data.tmp");
	
	// mode
	result.strReplace("#MODE#", eString().sprintf("%d", eZapMain::getInstance()->getMode()));
	
	// time
	time_t atime;
	time(&atime);
	result.strReplace("#TIME#", eString(ctime(&atime)));

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
	
	// vlc parameters
	result.strReplace("#VLCPARMS#", getvideom3u());
	
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
	else
	{
		result.strReplace("#DISKGB#", "n/a");
		result.strReplace("#DISKH#", "n/a");
	}

	// volume
	result.strReplace("#VOLUME#", (eAVSwitch::getInstance()->getMute()) ? "0" : eString().sprintf("%d", 63 - eAVSwitch::getInstance()->getVolume()));

	// mute
	result.strReplace("#MUTE#", (eAVSwitch::getInstance()->getMute()) ? "1" : "0");

	// channel stats
	result.strReplace("#DOLBY#", (eZapMain::getInstance()->getAC3Logo()) ? "1" : "0");
	result.strReplace("#CRYPT#", (eZapMain::getInstance()->getSmartcardLogo()) ? "1" : "0");
	result.strReplace("#FORMAT#", (eZapMain::getInstance()->get16_9Logo()) ? "1" : "0");

	// recording status
#ifndef DISABLE_FILE
	result.strReplace("#RECORDING#", (eZapMain::getInstance()->isRecording()) ? "1" : "0");
#else
	result.strReplace("#RECORDING#", "0");
#endif
	// vlc streaming
	result.strReplace("#SERVICEREFERENCE#", (eServiceInterface::getInstance()->service) ? eServiceInterface::getInstance()->service.toString() : "");
	
	// dvr info
	int videopos = 0;
	int min = 0, sec2 = 0;
	int total = 0, current = 0;

#ifndef DISABLE_FILE
	if (eServiceHandler *handler = eServiceInterface::getInstance()->getService())
	{
		total = handler->getPosition(eServiceHandler::posQueryLength);
		current = handler->getPosition(eServiceHandler::posQueryCurrent);
	}

	if ((total > 0) && (current != -1))
	{
		min = total - current;
		sec2 = min % 60;
		min /= 60;
		videopos = (current * 20) / total;
	}
#endif

	result.strReplace("#VIDEOPOSITION#", eString().sprintf("%d", videopos));
	result.strReplace("#VIDEOTIME#", eString().sprintf("%d:%02d", min, sec2));
	
	// stream info
	eFrontend *fe = eFrontend::getInstance();
	result.strReplace("#SNR#", eString().sprintf("%d", fe->SNR() * 100 / 65535));
	result.strReplace("#AGC#", eString().sprintf("%d", fe->SignalStrength() * 100 / 65535));
	result.strReplace("#BER#", eString().sprintf("%u", fe->BER()));
	
#ifdef ENABLE_DYN_STREAM
	// streaming client status
	result.strReplace("#STREAMINGCLIENTSTATUS#", eString().sprintf("%d", eMoviePlayer::getInstance()->getStatus()));
#else
	result.strReplace("#STREAMINGCLIENTSTATUS#", "0");
#endif

	return result;
}


static eString data(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	return getBoxStatus("HTML");
}

static eString body(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;
	std::map<eString, eString> opt = getRequestOptions(opts, '&');
	content->local_header["Content-Type"]="text/html; charset=utf-8";

	eConfig::getInstance()->getKey("/ezap/webif/screenWidth", screenWidth);

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

		if ((zapMode >= 0) && (zapMode <= 5) && (zapSubMode >= 0) && (zapSubMode <= 5))
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
#ifndef DISABLE_FILE
	dyn_resolver->addDyn("GET", "/cgi-bin/videocontrol", videocontrol, lockWeb);
#endif
	dyn_resolver->addDyn("GET", "/setVolume", setVolume, lockWeb);
	dyn_resolver->addDyn("GET", "/setVideo", setVideo, lockWeb);
	dyn_resolver->addDyn("GET", "/tvMessageWindow", tvMessageWindow, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/zapTo", zapTo, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/admin", admin, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/selectAudio", selectAudio, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/setAudio", setAudio, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/selectSubChannel", selectSubChannel, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/message", message, lockWeb);
	dyn_resolver->addDyn("GET", "/control/message", message, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/rc", remoteControl, lockWeb);
	dyn_resolver->addDyn("GET", "/showRemoteControl", showRemoteControl, lockWeb);
	dyn_resolver->addDyn("GET", "/satFinder", satFinder, lockWeb);
	dyn_resolver->addDyn("GET", "/audio.pls", audiopls, lockWeb);
	dyn_resolver->addDyn("GET", "/video.m3u", videom3u, lockWeb);
	dyn_resolver->addDyn("GET", "/movie.m3u", moviem3u, lockWeb);
	dyn_resolver->addDyn("GET", "/mplayer.mply", mPlayer, lockWeb);
	dyn_resolver->addDyn("GET", "/body", body, lockWeb);
	dyn_resolver->addDyn("GET", "/data", data, lockWeb);
	dyn_resolver->addDyn("GET", "/leftnavi", leftnavi, lockWeb);
	dyn_resolver->addDyn("GET", "/webxtv", webxtv, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/setStreamingServiceRef", setStreamingServiceRef, lockWeb);
#ifndef DISABLE_FILE
	dyn_resolver->addDyn("GET", "/cgi-bin/recoverRecordings", recoverRecordings, lockWeb);
	dyn_resolver->addDyn("GET", "/cgi-bin/deleteMovie", deleteMovie, lockWeb);
#endif
	dyn_resolver->addDyn("GET", "/cgi-bin/osdshot", osdshot, lockWeb);
	
	ezapWapInitializeDyn(dyn_resolver, lockWeb);
	ezapXMLInitializeDyn(dyn_resolver, lockWeb);
	ezapEPGInitializeDyn(dyn_resolver, lockWeb);
	ezapMiscInitializeDyn(dyn_resolver, lockWeb);
	ezapTimerInitializeDyn(dyn_resolver, lockWeb);
	ezapPDAInitializeDyn(dyn_resolver, lockWeb);
#ifdef ENABLE_DYN_STREAM
	ezapMoviePlayerInitializeDyn(dyn_resolver, lockWeb);
#endif
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
}


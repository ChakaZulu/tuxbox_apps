#include "enigma_dyn.h"

#include <map>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>

#include <config.h>
#include <apps/enigma/enigma.h>
#include <apps/enigma/enigma_main.h>
#include <apps/enigma/sselect.h>

#include <core/system/http_dyn.h>
#include <core/dvb/dvb.h>
#include <core/dvb/edvb.h>
#include <core/dvb/epgcache.h>
#include <core/system/econfig.h>
#include <core/gdi/fb.cpp>
#include <core/dvb/decoder.h>
#include <core/dvb/dvbservice.h>
#include <core/dvb/service.h>
#include <core/gui/emessage.h>
#include <core/driver/eavswitch.h>


// #include <core/dvr/dvrsocket.h>

#define TEMPLATE_DIR DATADIR+eString("/enigma/templates/")

#define DELETE(WHAT) result.strReplace(#WHAT, "")

static int getHex(int c)
{
	c=toupper(c);
	c-='0';
	if (c<0)
		return -1;
	if (c > 9)
		c-='A'-10;
	if (c > 0xF)
		return -1;
	return c;
}

static eString httpUnescape(const eString &string)
{
	eString ret;
	for (unsigned int i=0; i<string.length(); ++i)
	{
		int c=string[i];
		switch (c)
		{
		case '%':
		{
			int val='%';
			if ((string.length() - i) > 1)
				val=getHex(string[++i]);
			if ((string.length() - i) > 1)
			{
				val<<=4;
				val+=getHex(string[++i]);
			}
			ret+=val;
			break;
		}
		case '+':
			ret+=' ';
			break;
		default:
			ret+=c;
			break;
		}
	}
	return ret;
}

static std::map<eString,eString> getRequestOptions(eString opt)
{
	std::map<eString,eString> result;
	
	if (opt[0]=='?')
		opt=opt.mid(1);

	while (opt.length())
	{
		unsigned int e=opt.find("=");
		if (e==eString::npos)
			e=opt.length();
		unsigned int a=opt.find("&", e);
		if (a==eString::npos)
			a=opt.length();
		eString n=opt.left(e);

		unsigned int b=opt.find("&", e+1);
		if(b==eString::npos)
			b=(unsigned)-1;
		eString r=httpUnescape(opt.mid(e+1, b-e-1));
		result.insert(std::pair<eString, eString>(n, r));
		opt=opt.mid(a+1);
	}
	return result;
}

static eString doStatus(eString request, eString path, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html";
	eString result;
	time_t atime;
	time(&atime);
	atime+=eDVB::getInstance()->time_difference;
	result="<html>\n"
		"<head>\n"
		"  <title>enigma status</title>\n"
		"  <link rel=stylesheet type=\"text/css\" href=\"/index.css\">\n"
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

static eString switchService(eString request, eString path, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html";
	
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "ERROR not available";
	
	int service_id=-1, original_network_id=-1, transport_stream_id=-1, service_type=-1;
	unsigned int optval=opt.find("=");
	if (optval!=eString::npos)
		opt=opt.mid(optval+1);
	if (opt.length())
		sscanf(opt.c_str(), "%x:%x:%x:%x", &service_id, &transport_stream_id, &original_network_id, &service_type);
	eString result="";
	
	if ((service_id!=-1) && (original_network_id!=-1) && (transport_stream_id!=-1) && (service_type!=-1))
	{
			sapi->switchService(
				eServiceReferenceDVB(
					eTransportStreamID(transport_stream_id), 
					eOriginalNetworkID(original_network_id),
					eServiceID(service_id),
					service_type));
		result+="OK\n";
	} else
	{
		result+="ERROR wrong parms\n";
	}
	return result;
}

struct listService: public std::unary_function<std::pair<eServiceReferenceDVB,eService>&,void>
{
	eString &result;
	const eString &search;
	listService(eString &result, const eString &search): result(result), search(search)
	{
	}
	void operator()(eService &service)
	{
		if (search && (service.service_name.find(search)==eString::npos))
			return;
		eString sc;
		sc.sprintf("%x:%x:%x:%x", service.service_id.get(), service.transport_stream_id.get(), service.original_network_id.get(), service.service_type);
		result+="<tr><td><a href=\"/cgi-bin/switchService?service=" + sc + "\">" + service.service_name.c_str() + "</a></td>"
						"<td>" + eString().setNum(service.service_type, 0x10) + "</td></tr>\n";
	}
};

static eString listServices(eString request, eString path, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html";
	eString result;
	std::map<eString,eString> opt=getRequestOptions(opts);
	eString search=opt["search"];
	result="<html>\n"
		"<head>\n"
		"  <title>enigma service list</title>\n"
		"  <link rel=stylesheet type=\"text/css\" href=\"/index.css\">\n"
		"</head>\n"
		"<body>\n"
		"<h1>Enigma channel list</h1>\n"
		"<table>\n";
		
	eDVB::getInstance()->settings->getTransponders()->forEachService(listService(result, search));
	result+="</table>\n"
		"</body>\n"
		"</html>\n";
	return result;
}

static eString admin(eString request, eString path, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html";
	std::map<eString,eString> opt=getRequestOptions(opts);
	eString command=opt["command"];
	if (command && command=="shutdown")
	{
		eZap::getInstance()->quit();
		return "<html><head><title>Shutdown</title></head><body>Shutdown initiated.</body></html>\n";
	} else
		return "<html><head><title>Error</title></head><body>Unknown admin command.</body></html>\n";
}

static eString audio(eString request, eString path, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html";
	std::map<eString,eString> opt=getRequestOptions(opts);
	eString result="";
	eString volume=opt["volume"];
	if (volume)
	{
		int vol=atoi(volume.c_str());
		eAVSwitch::getInstance()->changeVolume(1, vol);
		result+="Volume set.<br>\n";
	}
	eString mute=opt["mute"];
	if (mute)
	{
		eAVSwitch::getInstance()->toggleMute();
		result+="mute set<br>\n";
	}
	result+=eString().sprintf("volume: %d<br>\nmute: %d<br>\n", eAVSwitch::getInstance()->getVolume(), eAVSwitch::getInstance()->getMute());
	return result;
}

static eString getPMT(eString request, eString path, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="x-application/PMT";
	PMT *pmt=eDVB::getInstance()->getPMT();
	if (!pmt)
		return "result=ERROR\n";
	eString res="result=OK\n";
	res+="PMT"+eString().sprintf("(%04x)\n", pmt->pid);
	res+="program_number="+eString().sprintf("%04x\n", pmt->program_number);
	res+="PCR_PID="+eString().sprintf("%04x\n", pmt->PCR_PID);
	res+="program_info\n";
	for (ePtrList<Descriptor>::iterator d(pmt->program_info); d != pmt->program_info.end(); ++d)
		res+=d->toString();
	for (ePtrList<PMTEntry>::iterator s(pmt->streams); s != pmt->streams.end(); ++s)
	{
		res+="PMTEntry\n";
		res+="stream_type="+eString().sprintf("%02x\n", s->stream_type);
		res+="elementary_PID="+eString().sprintf("%04x\n", s->elementary_PID);
		res+="ES_info\n";
		for (ePtrList<Descriptor>::iterator d(s->ES_info); d != s->ES_info.end(); ++d)
			res+=d->toString();
	}
	pmt->unlock();
	return res;
}

static eString version(eString request, eString path, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/plain";
	eString result;
	result.sprintf("EliteDVB Version : %s\r\n, eZap Version : doof\r\n",eDVB::getInstance()->getVersion().c_str());
	return result;
}

static eString channels_getcurrent(eString request, eString path, eString opt, eHTTPConnection *content)
{
	eString result="";
	content->local_header["Content-Type"]="text/plain";

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "-1";
		
	eService *service=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
	
	if (service)
		result+=eString().sprintf("%d", service->service_number);
	else
		result+="-1";
	return result+"\r\n";
}

static eString getVolume()
{
	return eString().setNum((63-eAVSwitch::getInstance()->getVolume())*100/63, 10);
}

static eString setVolume(eString request, eString path, eString opts, eHTTPConnection *content)
{
	std::map<eString,eString> opt=getRequestOptions(opts);
	eString mute="0";
	eString volume;
	eString result="";
	int mut=0, vol=0;

	content->local_header["Content-Type"]="text/html";

	result+="<script language=\"javascript\">window.close();</script>";
	mute=opt["mute"];
	volume=opt["volume"];

	if(!mute) {
		mut=0;
	} else {
		eAVSwitch::getInstance()->toggleMute();
		result+="[mute OK]";
		return result;
	}

	if(volume) {
		vol=atoi(volume.c_str());
	} else {
		result+="[no params]";
		return result;
	}
	if(vol>10) vol=10;
	if(vol<0) vol=0;

	float temp=(float)vol;
	temp=temp*6.3;
	vol=(int)temp;

	eAVSwitch::getInstance()->changeVolume(1, 63-vol);
	result+="[volume OK]";

	return result;
}

static eString read_file(eString filename)
{
#define BLOCKSIZE 8192
	int fd;
	fd=open(filename.c_str(), O_RDONLY);
	if(!fd)
		return eString("file: "+filename+" not found\n");
	
	int size=0;

	char tempbuf[BLOCKSIZE+200];
	eString result("");

	while((size=read(fd, tempbuf, BLOCKSIZE))>0)
	{
		tempbuf[size]=0;
		result+=eString(tempbuf);
	}
	return result;
}

static eString getIP()
{
	eString tmp;
	int sd;
	struct ifreq ifr;
	sd=socket(AF_INET, SOCK_DGRAM, 0);
	if(sd<0)
	{
		return "?.?.?.?-socket-error";
	}
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_addr.sa_family=AF_INET; // fixes problems with some linux vers.
	strncpy(ifr.ifr_name, "eth0", sizeof(ifr.ifr_name));
	if(ioctl(sd, SIOCGIFADDR, &ifr) < 0)
	{
		return "?.?.?.?-ioctl-error";
	}
	close(sd);
	
	tmp.sprintf("%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr) );
	return tmp;
}


static eString filter_string(eString string)
{
	return string.removeChars('\x86').removeChars('\x87').removeChars('\x05');
}


eBouquet *getBouquet(int bouquet_id)
{
	ePtrList<eBouquet>* b;
	b=eDVB::getInstance()->settings->getBouquets();

	for (ePtrList<eBouquet>::iterator It(*b); It != b->end(); It++)
		if (It->bouquet_id == bouquet_id)
			return *It;
	
	return 0;
}


static eString getVolBar()
{
	eString result="";
	int volume=atoi(getVolume().c_str());

	result+="<table cellspacing=\"0\" cellpadding=\"0\" border=\"0\">";
	result+="<tr>";

	for(int i=1;i<=(volume/10);i++)
	{
		result+="<td><a class=\"volgreen\" href=\"javascript:setVol(";
		result+=eString().setNum(i, 10);
		result+=")\">||</a></span></td>";
	}
	for(int i=(volume/10)+1;i<=(100/10);i++)
	{
		result+="<td><a class=\"volnot\" href=\"javascript:setVol(";
		result+=eString().setNum(i, 10);
		result+=")\">||</a></span></td>";
	}

	result+="<td>";

	if(eAVSwitch::getInstance()->getMute()==1) {
		result+="<a class=\"mute\" href=\"javascript:unMute()\">";
		result+="unmute";
	} else {
		result+="<a class=\"mute\" href=\"javascript:Mute()\">";
		result+="mute";
	}
	result+="</a></td>";
	result+="</tr>";
	result+="</table>";
	return result;
}

static eString getWatchContent(eString mode, int bouquetid)
{
	ePtrList<eBouquet>* bouquets;
	std::list<eServiceReferenceDVB> esref;
	eService *es;
	eString result("");
	eString tmp("");

	bouquets=eDVB::getInstance()->settings->getBouquets();

	if(mode=="tv")
	{
		result+="<form action=\"/?mode=tv\" method=\"get\" name=\"bouquetsel\">";
		result+="<select name=\"bouquetid\" size=\"1\" onChange=\"javascript:getNewPageTV(this.form.bouquetid.options[this.form.bouquetid.options.selectedIndex].value)\">";
		for(ePtrList<eBouquet>::iterator i(*bouquets); i != bouquets->end(); ++i)
		{
			for(std::list<eServiceReferenceDVB>::iterator s = i->list.begin(); s != i->list.end(); s++)
			{
				if(s->getServiceType()==1 || s->getServiceType()==4)
				{
					tmp=eString(filter_string(i->bouquet_name.c_str()));
					result+="<option value=\"" + eString().setNum(i->bouquet_id, 10) + "\"";
					if(i->bouquet_id==bouquetid)
					{
						result+=" selected";
					}
					result+=">" + tmp + "</option>";
					break;
				}
			}
		}
		result+="</select>";
		result+="<select name=\"channel\" size=\"1\" onChange=\"javascript:switchtoChannel(this.form.channel.options[this.form.channel.options.selectedIndex].value)\"><option>-----</option>";
		eBouquet *act;

		act=getBouquet(bouquetid);
		if(!act)
			return eString("no bouquets");
		esref=act->list;
		for(std::list<eServiceReferenceDVB>::iterator j = esref.begin(); j != esref.end() ; j++)
		{
			es=eDVB::getInstance()->settings->getTransponders()->searchService(*j);
			if (es)
			{
				if(es->service_type==1||es->service_type==4)
				{
					result+="<option value=\"";
					tmp.sprintf("%x:%x:%x:%x", j->getServiceID().get(), j->getTransportStreamID().get(), j->getOriginalNetworkID().get(), j->getServiceType());
					result+=tmp;
					result+="\">";
					result+=filter_string(es->service_name.c_str());
					result+="</option>";
				}
			}
		}
		result+="</select>";
		result+="</form>";
	}

	if(mode=="radio")
	{
		result+="<form action=\"/?mode=radio\" method=\"get\" name=\"bouquetsel\">";
		result+="<select name=\"bouquetid\" size=\"1\" onChange=\"javascript:getNewPageRadio(this.form.bouquetid.options[this.form.bouquetid.options.selectedIndex].value)\">";
		for(ePtrList<eBouquet>::iterator i(*bouquets); i != bouquets->end(); ++i)
		{
			for(std::list<eServiceReferenceDVB>::iterator s = i->list.begin(); s != i->list.end(); s++)
			{
				if(s->getServiceType()==2)
				{
					tmp=eString(filter_string(i->bouquet_name.c_str()));
					result+="<option value=\"" + eString().setNum(i->bouquet_id, 10) + "\"";
					if(i->bouquet_id==bouquetid)
					{
						result+=" selected";
					}
					result+=">" + tmp + "</option>";
					break;
				}
			}
		}
		result+="</select>";
		result+="<select name=\"channel\" size=\"1\" onChange=\"javascript:switchtoChannel(this.form.channel.options[this.form.channel.options.selectedIndex].value)\"><option>-----</option>";
		eBouquet *act;
		act=getBouquet(bouquetid);
		esref=act->list;
		for(std::list<eServiceReferenceDVB>::iterator j = esref.begin(); j != esref.end() ; j++)
		{
			es=eDVB::getInstance()->settings->getTransponders()->searchService(*j);
			if (es)
			{
				if(es->service_type==2)
				{
					result+="<option value=\"";
					tmp.sprintf("%x:%x:%x:%x", j->getServiceID().get(), j->getTransportStreamID().get(), j->getOriginalNetworkID().get(), j->getServiceType());
					result+=tmp;
					result+="\">";
					result+=filter_string(es->service_name.c_str());
					result+="</option>";
				}
			}
		}
		result+="</select>";
		result+="</form>";
	}

	return result;
}

static eString getContent(eString mode, int bouquetid)
{
	eString result("");

	if(mode=="tv"||mode=="radio")
	{
		if(!bouquetid)
		{
			int imode=eZap::TV;
			if(mode=="radio")
				imode=eZap::Radio;
			eZap::getInstance()->setMode(imode);
			bouquetid=eZap::getInstance()->getServiceSelector()->getCurrentBouquet();
		}
		else
		{
/*
	buggy stuff... :(
			if(eZap::getInstance()->getServiceSelector()->getCurrentBouquet()!=bouquetid)
			{
				eZap::getInstance()->getServiceSelector()->useBouquet(getBouquet(bouquetid));
			}		
*/
		}

		result=getWatchContent(mode, bouquetid);
	}

	if(result.length()<3)
		result="not ready yet";

	return result;
}

static eString getCurService()
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "not available";
	
	eService *current=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
	if(current)
		return current->service_name.c_str();
	else
		return "no channel selected";
}

static eString getEITC()
{
	eString result;

	EIT *eit=eDVB::getInstance()->getEIT();
	
	if(eit)
	{
		eString now_time="", now_duration="", now_text="", now_longtext="";
		eString next_time="", next_duration="", next_text="", next_longtext="";

		int p=0;

		for(ePtrList<EITEvent>::iterator event(eit->events); event != eit->events.end(); ++event)
		{
			if(*event)
			{
				if(p==0)
				{
					if(event->start_time!=0) {
						now_time.sprintf("%s", ctime(&event->start_time));
						now_time=now_time.mid(10, 6);
					} else {
						now_time="";
					}
					now_duration.sprintf("%d", (int)(event->duration/60));
				}
				if(p==1)
				{
					if(event->start_time!=0) {
 						next_time.sprintf("%s", ctime(&event->start_time));
						next_time=next_time.mid(10,6);
						next_duration.sprintf("%d", (int)(event->duration/60));
					} else {
						now_time="";
					}
				}
				for(ePtrList<Descriptor>::iterator descriptor(event->descriptor); descriptor != event->descriptor.end(); ++descriptor)
				{
					if(descriptor->Tag()==DESCR_SHORT_EVENT)
					{
						ShortEventDescriptor *ss=(ShortEventDescriptor*)*descriptor;
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
					if(descriptor->Tag()==DESCR_EXTENDED_EVENT)
					{
						ExtendedEventDescriptor *ss=(ExtendedEventDescriptor*)*descriptor;
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

		if(now_time!="") {
			result=read_file(TEMPLATE_DIR+"eit.tmp");
			result.strReplace("#NOWT#", now_time);
			result.strReplace("#NOWD#", now_duration);
			result.strReplace("#NOWST#", now_text);
			result.strReplace("#NOWLT#", filter_string(now_longtext));
			result.strReplace("#NEXTT#", next_time);
			result.strReplace("#NEXTD#", next_duration);
			result.strReplace("#NEXTST#", next_text);
			result.strReplace("#NEXTLT#", filter_string(next_longtext));
		} else {
			result="eit undefined";
		}	
		eit->unlock();
	}
	else
	{
		result="no eit";
	}

	return result;
}


static eString getStats()
{
	eString result;
	eString apid, vpid;
	eString tmp;
	int iapid=0, ivpid=0, bootcount=0;


	int sec=atoi(read_file("/proc/uptime").c_str());

	result="<span class=\"white\">";
	result+=eString().sprintf("%d:%02dm up", sec/3600, (sec%3600)/60);
	result+="</span> | ";

	tmp=read_file("/proc/mounts");
	if(tmp.find("cramfs")!=eString::npos)
	{
		result+="<span class=\"white\">running from flash</span>";
	}
	else
	{
		result+="<span class=\"white\">running via net</span>";
	}
	result+=" | ";

	eConfig::getInstance()->getKey("/elitedvb/system/bootCount", bootcount);

	result+="<span class=\"white\">"+getIP()+"</span>";

	result+=" | ";
	tmp.sprintf("<span class=\"white\">booted enigma %d times</span><br>", bootcount);
	result+=tmp;

	ivpid=Decoder::parms.vpid;
	iapid=Decoder::parms.apid;
	if(ivpid==-1)
		vpid="none";
	else
		vpid.sprintf("0x%x", ivpid);
	if(iapid==-1)
		apid="none";
	else
		apid.sprintf("0x%x", iapid);
	tmp.sprintf("<span class=\"white\">vpid: %s</span> | <a class=\"audio\" href=\"/audio.m3u\">apid: %s</a>", vpid.c_str(), apid.c_str());
	result+=tmp;

	return result;
}

static eString getNavi(eString mode)
{
	eString radioc, tvc, aboutc, linksc, updatesc;
	eString result;

	tvc="normal";
	radioc="normal";
	aboutc="normal";
	linksc="normal";
	updatesc="normal";

	if(mode=="tv")
		tvc="white";
	if(mode=="radio")
		radioc="white";
	if(mode=="about")
		aboutc="white";
	if(mode=="links")
		linksc="white";
	if(mode=="updates")
		updatesc="white";


	result="<a class=\"";
	result+=tvc;
	result+="\" href=\"/?mode=tv\">tv</a> | <a class=\"";
	result+=radioc;
	result+="\" href=\"/?mode=radio\">radio</a> | <a class=\"";
	result+=aboutc;
	result+="\" href=\"/?mode=about\">about</a> | <a class=\"";
	result+=linksc;
	result+="\" href=\"/?mode=links\">links</a> | <a class=\"";
	result+=updatesc;
	result+="\" href=\"/?mode=updates\">updates</a>";

	return result;
}

static eString getMode(eString mode)
{
	eString result;
	mode.upper();
	result="<span class=\"titel\">"+mode+"</span>";
	return result;
}

static eString web_root(eString request, eString path, eString opts, eHTTPConnection *content)
{
	eString result;
	std::map<eString,eString> opt=getRequestOptions(opts);
	content->local_header["Content-Type"]="text/html";

	eString mode=opt["mode"];
	eString bid="0";

	if(opt["bouquetid"])
		bid=opt["bouquetid"];

	int bouquetid=atoi(bid.c_str());

	if(!mode)
	{
		switch(eZap::getInstance()->getMode())
		{
			case 0:
				mode="tv";
				break;
			case 1:
				mode="radio";
				break;
			default:
				mode="tv";
				break;
		}
	}

	result=read_file(TEMPLATE_DIR+"index.tmp");

	result.strReplace("#STATS#", getStats());
	result.strReplace("#NAVI#", getNavi(mode));
	result.strReplace("#MODE#", getMode(mode));
	result.strReplace("#COP#", getContent(mode, bouquetid));

	if((mode=="tv")||
           (mode=="radio"))
	{
		result.strReplace("#EIT#", getEITC() );
		result.strReplace("#SERVICENAME#", filter_string(getCurService()));
		
		eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();

		if(sapi && sapi->service)
		{
			result.strReplace("#EPG#", "<u><a href=\"javascript:openEPG()\" class=\"small\">epg</a></u>");
			result.strReplace("#SI#", "<u><a href=\"javascript:openSI()\" class=\"small\">si</a></u>");
		}
		else
		{
			DELETE(#EPG#);
			DELETE(#SI#);
		}
	}
	else
	{
		DELETE(#SERVICENAME#);
		DELETE(#EPG#);
		DELETE(#SI#);
		DELETE(#EIT#);
	}
	result.strReplace("#VOLBAR#", getVolBar());
	return result;
}

static eString switchServiceWeb(eString request, eString path, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html";

	int service_id=-1, original_network_id=-1, transport_stream_id=-1, service_type=-1;
	unsigned int optval=opt.find("=");
	if (optval!=eString::npos)
		opt=opt.mid(optval+1);
	if(opt)
		sscanf(opt.c_str(), "%x:%x:%x:%x", &service_id, &transport_stream_id, &original_network_id, &service_type);
	eString result="";
	
	if ((service_id!=-1) && (original_network_id!=-1) && (transport_stream_id!=-1) && (service_type!=-1))
	{
		eZap::getInstance()->getServiceSelector()->actualize();
		if(eDVB::getInstance()->settings->getTransponders())
		{
			const eServiceReferenceDVB *ref=eDVB::getInstance()->settings->getTransponders()->searchService(eOriginalNetworkID(original_network_id), eServiceID(service_id));
			if(ref)
				eServiceInterface::getInstance()->play(*ref);
		}
		result+="<script language=\"javascript\">window.close();</script>";
	} else
	{
		result+="<script language=\"javascript\">alert(\"ERROR wrong parms\")</script>";
	}
	return result;
}

static eString audiom3u(eString request, eString path, eString opt, eHTTPConnection *content)
{
	eString result;
	eString tmp;

	content->local_header["Content-Type"]="audio/mpegfile";
	result="http://"+getIP()+":31338/";
        tmp.sprintf("%02x\n", Decoder::parms.apid);
 	result+=tmp;
	return result;
}


static eString getbouq(eString request, eString path, eString opt, eHTTPConnection *content)
{
 	eString result;
	eString tmp;

	ePtrList<eBouquet>* bouquets;
	std::list<eServiceReferenceDVB> esref;

	content->local_header["Content-Type"]="text/html";

	bouquets=eDVB::getInstance()->settings->getBouquets();
	result=eString("");

	for(ePtrList<eBouquet>::iterator i(*bouquets); i != bouquets->end(); ++i)
	{
		tmp=eString(i->bouquet_name.c_str());
		result+=eString(i->bouquet_name.c_str());
	}
	
	return result;
}

static eString getcurepg(eString request, eString path, eString opt, eHTTPConnection *content)
{
	eString result("");
	eString tmp;
	eService* current;

	content->local_header["Content-Type"]="text/html";

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "not available";

	current=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
	if(!current)
		return eString("epg not ready yet");

	result+=eString("<html><head><title>epgview</title><link rel=\"stylesheet\" type=\"text/css\" href=\"/epgview.css\"></head><body bgcolor=#000000>");
	result+=eString("<span class=\"title\">");
	result+=eString(current->service_name);
	result+=eString("</span>");
	result+=eString("<br>\n");

	const eventMap* evt=eEPGCache::getInstance()->getEventMap(sapi->service);
	if(!evt)
		return eString("epg not ready yet");

	eventMap::const_iterator It;

	for(It=evt->begin(); It!= evt->end(); It++)
	{
		EITEvent event(*It->second);
		for(ePtrList<Descriptor>::iterator d(event.descriptor); d != event.descriptor.end(); ++d)
		{
			Descriptor *descriptor=*d;
			if(descriptor->Tag()==DESCR_SHORT_EVENT)
			{
				tm* t = localtime(&event.start_time);
				tmp.sprintf("<span class=\"epg\">%02d.%02d - %02d:%02d ", t->tm_mday, t->tm_mon+1, t->tm_hour, t->tm_min);
				result+=tmp;
				result+=((ShortEventDescriptor*)descriptor)->event_name;
				result+=eString("</span><br>\n");
			}
		}

	}
	result+="</body></html>";
	return result;
}

static eString getsi(eString request, eString path, eString opt, eHTTPConnection *content)
{
	eString result("");
	eString name("");
	eString provider("");
	eString vpid("");
	eString apid("");
	eString pcrpid("");
	eString tpid("");
	eString vidform("n/a");
	eString tsid("");
	eString onid("");
	eString sid("");

	content->local_header["Content-Type"]="text/html";

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "not available";

	eService *service=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
	if (service)
	{
		name=service->service_name.c_str();
		provider=service->service_provider.c_str();
	}
	vpid=eString().sprintf("%04xh (%dd)", Decoder::parms.vpid, Decoder::parms.vpid);
	apid=eString().sprintf("%04xh (%dd)", Decoder::parms.apid, Decoder::parms.apid);
	pcrpid=eString().sprintf("%04xh (%dd)", Decoder::parms.pcrpid, Decoder::parms.pcrpid);
	tpid=eString().sprintf("%04xh (%dd)", Decoder::parms.tpid, Decoder::parms.tpid);
	tsid=eString().sprintf("%04xh", sapi->service.getTransportStreamID().get());
	onid=eString().sprintf("%04xh", sapi->service.getOriginalNetworkID().get());
	sid=eString().sprintf("%04xh", sapi->service.getServiceID().get());

	FILE *bitstream=0;
	
	if (Decoder::parms.vpid!=-1)
		bitstream=fopen("/proc/bus/bitstream", "rt");
	if (bitstream)
	{
		char buffer[100];
		int xres=0, yres=0, aspect=0;
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
			vidform+="square"; break;
		case 2:
			vidform+="4:3"; break;
		case 3:
			vidform+="16:9"; break;
		case 4:
			vidform+="20:9"; break;
		}
	}


	result+=eString("<html><head><title>streaminfo</title><link rel=\"stylesheet\" type=\"text/css\" href=\"/si.css\"></head><body bgcolor=#000000>");
	result+=eString("<table cellspacing=0 cellpadding=0 border=0>");
	result+=eString("<tr><td>name:</td><td>"+name+"</td></tr>");
	result+=eString("<tr><td>provider:</td><td>"+provider+"</td></tr>");
	result+=eString("<tr><td>vpid:</td><td>"+vpid+"</td></tr>");
	result+=eString("<tr><td>apid:</td><td>"+apid+"</td></tr>");
	result+=eString("<tr><td>pcrpid:</td><td>"+pcrpid+"</td></tr>");
	result+=eString("<tr><td>tpid:</td><td>"+tpid+"</td></tr>");
	result+=eString("<tr><td>tsid:</td><td>"+tsid+"</td></tr>");
	result+=eString("<tr><td>onid:</td><td>"+onid+"</td></tr>");
	result+=eString("<tr><td>sid:</td><td>"+sid+"</td></tr>");
	result+=eString("<tr><td>vidformat:<td>"+vidform+"</td></tr>");
	result+=eString("</table>");
	result+=eString("</body></html>");
	return result;
}

static eString neutrino_suck_zapto(eString request, eString path, eString opt, eHTTPConnection *content)
{
	if(opt!="getpids")
		return(eString("ok\n"));
	else
		return(eString().sprintf("%u\n%u\n", Decoder::parms.vpid, Decoder::parms.apid));
}

static eString message(eString request, eString path, eString opt, eHTTPConnection *content)
{
	if (opt.length())
	{
		opt.strReplace("%20", " ");
		eZapMain::getInstance()->postMessage(eZapMessage(1, "external message", opt, 10), 0);
		return eString("ok\n");
	} else
		return eString("error\n");
}

static eString xmessage(eString request, eString path, eString opt, eHTTPConnection *content)
{
	std::map<eString,eString> opts=getRequestOptions(opt);
	
	if (opts.find("timeout") == opts.end())
		return "E: no timeout set";
	
	if (opts.find("caption") == opts.end())
		return "E: no caption set";

	if (opts.find("body") == opts.end())
		return "E: no body set";

	int type=-1;
	if (opts.find("type") != opts.end())
		type=atoi(opts["type"].c_str());
	
	int timeout=atoi(opts["timeout"].c_str());
	
	eZapMain::getInstance()->postMessage(eZapMessage(1, opts["caption"], opts["body"], timeout), type != -1);

	return eString("OK\n");
}

/*
static eString record_off(eString request, eString path, eString opt, eHTTPConnection *content)
{
	DVRSocket *dvr;
	dvr=new DVRSocket(eString("10.0.0.2"), 3000, NGRAB);
	dvr->stopRecording();
	delete dvr;
	return "ok";
}

static eString record_on(eString request, eString path, eString opt, eHTTPConnection *content)
{
	DVRSocket *dvr;
	dvr=new DVRSocket(eString("10.0.0.2"), 3000, NGRAB);;
	dvr->startRecording();
	return "ok";
}
*/

void ezapInitializeDyn(eHTTPDynPathResolver *dyn_resolver)
{
	dyn_resolver->addDyn("GET", "/", web_root);
	dyn_resolver->addDyn("GET", "/switchTo", switchServiceWeb);
	dyn_resolver->addDyn("GET", "/setVolume", setVolume);
/*
	dyn_resolver->addDyn("GET", "/record/on", record_on);
	dyn_resolver->addDyn("GET", "/record/off", record_off);
*/
	dyn_resolver->addDyn("GET", "/cgi-bin/status", doStatus);
	dyn_resolver->addDyn("GET", "/cgi-bin/switchService", switchService);
	dyn_resolver->addDyn("GET", "/cgi-bin/listServices", listServices);
	dyn_resolver->addDyn("GET", "/cgi-bin/admin", admin);
	dyn_resolver->addDyn("GET", "/cgi-bin/audio", audio);
	dyn_resolver->addDyn("GET", "/cgi-bin/getPMT", getPMT);
	dyn_resolver->addDyn("GET", "/cgi-bin/message", message);
	dyn_resolver->addDyn("GET", "/cgi-bin/xmessage", xmessage);

	dyn_resolver->addDyn("GET", "/audio.m3u", audiom3u);
	dyn_resolver->addDyn("GET", "/version", version);
	dyn_resolver->addDyn("GET", "/cgi-bin/getbouquets", getbouq);
	dyn_resolver->addDyn("GET", "/cgi-bin/getcurrentepg", getcurepg);
	dyn_resolver->addDyn("GET", "/cgi-bin/streaminfo", getsi);
	dyn_resolver->addDyn("GET", "/channels/getcurrent", channels_getcurrent);


	dyn_resolver->addDyn("GET", "/control/zapto", neutrino_suck_zapto);

/*	dyn_resolver->addDyn("GET", "/channels/numberchannels", channels_numberchannels);
	dyn_resolver->addDyn("GET", "/channels/gethtmlchannels", channels_gethtmlchannels);
	dyn_resolver->addDyn("GET", "/channels/getchannels", channels_getgetchannels);
	dyn_resolver->addDyn("GET", "/epg/now", epg_now);
	dyn_resolver->addDyn("GET", "/epg/next", epg_next); */
}

#include <time.h>
#include <qmap.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "enigma.h"
#include "decoder.h"
#include "enigma_dyn.h"
#include "http_dyn.h"
#include "dvb.h"
#include "edvb.h"

#include <config.h>
#include <core/system/econfig.h>

#define TEMPLATE_DIR DATADIR+eString("/enigma/templates/")


static QMap<eString,eString> getRequestOptions(eString opt)
{
	QMap<eString,eString> result;
	
	if (opt[0]=='?')
		opt=opt.mid(1);
	while (opt.length())
	{
		int e=opt.find("=");
		if (e==-1)
			e=opt.length();
		int a=opt.find("&", e);
		if (a==-1)
			a=opt.length();
		eString n=opt.left(e);
		eString r=opt.mid(e+1, opt.find("&", e+1)-e-1);
		result.insert(n, r);
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
		"  <title>elitedvb status</title>\n"
		"  <link rel=stylesheet type=\"text/css\" href=\"/index.css\">\n"
		"</head>\n"
		"<body>\n"
		"<h1>EliteDVB status</h1>\n"
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
	int service_id=-1, original_network_id=-1, transport_stream_id=-1, service_type=-1;
	if (opt.find("="))
		opt=opt.mid(opt.find("=")+1);
	if (opt.length())
		sscanf(opt.c_str(), "%x:%x:%x:%x", &service_id, &transport_stream_id, &original_network_id, &service_type);
	eString result="";
	
	if ((service_id!=-1) && (original_network_id!=-1) && (transport_stream_id!=-1) && (service_type!=-1))
	{
		eService *meta=0;
		meta=eDVB::getInstance()->getTransponders()->searchService(original_network_id, service_id);
		if (meta)
			eDVB::getInstance()->switchService(meta);
		else
			eDVB::getInstance()->switchService(service_id, original_network_id, transport_stream_id, service_type);
		result+="OK\n";
	} else
	{
		result+="ERROR wrong parms\n";
	}
	return result;
}

struct listService: public std::unary_function<std::pair<sref,eService>&,void>
{
	eString &result;
	const eString &search;
	listService(eString &result, const eString &search): result(result), search(search)
	{
	}
	void operator()(eService &service)
	{
		if (search && (service.service_name.find(search)==-1))
			return;
		eString sc;
		sc.sprintf("%x:%x:%x:%x", service.service_id, service.transport_stream_id, service.original_network_id, service.service_type);
		result+="<tr><td><a href=\"/cgi-bin/switchService?service=" + sc + "\">" + service.service_name.c_str() + "</a></td>"
						"<td>" + eString().setNum(service.service_type, 0x10) + "</td></tr>\n";
	}
};

static eString listServices(eString request, eString path, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html";
	eString result;
	QMap<eString,eString> opt=getRequestOptions(opts);
	eString search=opt["search"];
	result="<html>\n"
		"<head>\n"
		"  <title>elitedvb service list</title>\n"
		"  <link rel=stylesheet type=\"text/css\" href=\"/index.css\">\n"
		"</head>\n"
		"<body>\n"
		"<h1>EliteDVB channel list</h1>\n"
		"<table>\n";
		
	eDVB::getInstance()->getTransponders()->forEachService(listService(result, search));
	result+="</table>\n"
		"</body>\n"
		"</html>\n";
	return result;
}

static eString admin(eString request, eString path, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html";
	QMap<eString,eString> opt=getRequestOptions(opts);
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
	QMap<eString,eString> opt=getRequestOptions(opts);
	eString result="";
	eString volume=opt["volume"];
	if (volume)
	{
		int vol=atoi(volume.c_str());
		eDVB::getInstance()->changeVolume(1, vol);
		result+="Volume set.<br>\n";
	}
	eString mute=opt["mute"];
	if (mute)
	{
		int m=atoi(mute.c_str());
		eDVB::getInstance()->changeVolume(3, m);
		result+="mute set<br>\n";
	}
	result+=eString().sprintf("volume: %d<br>\nmute: %d<br>\n", eDVB::getInstance()->volume, eDVB::getInstance()->mute);
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
	if (eDVB::getInstance()->service)
		result+=eString().sprintf("%d", eDVB::getInstance()->service->service_number);
	else
		result+="-1";
	return result+"\r\n";
}


static void unpack(__u32 l, int *t)
{
	for (int i=0; i<4; i++)
		*t++=(l>>((3-i)*8))&0xFF;
}

static eString getVolume()
{
	return eString().setNum((63-eDVB::getInstance()->volume)*100/63, 10);
}

static eString setVolume(eString request, eString path, eString opts, eHTTPConnection *content)
{
	QMap<eString,eString> opt=getRequestOptions(opts);
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
		eDVB::getInstance()->changeVolume(2,1);
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

	eDVB::getInstance()->changeVolume(1, 63-vol);
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
	int ip[4];
	memset(&ip, 0, sizeof(ip));
	tmp=read_file("/proc/net/tcp");
	
	if(sscanf(tmp.c_str(), "%02x%02x%02x%02x:0050", &ip[0], &ip[1], &ip[2], &ip[3])==4) {
		return eString().sprintf("%d.%d.%d.%d", ip);
	}
	return "?.?.?.?";
}


static eString filter_string(eString string)
{
	string=string.removeChars('\x86');
	string=string.removeChars('\x87');
	string=string.removeChars('\x05');
	return string;
}


eBouquet *getBouquet(int bouquet_id)
{
	ePtrList<eBouquet>* b;
	b=eDVB::getInstance()->getBouquets();

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

	if(eDVB::getInstance()->mute==1) {
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


static eString getContent(eString mode, int bouquetid)
{
	eString result="";
	eString tmp="";
	ePtrList<eBouquet>* bouquets;
	std::list<eServiceReference> esref;
	eService *es;

	bouquets=eDVB::getInstance()->getBouquets();

	bouquets->sort();

	if(mode=="tv")
	{
		result+="<form action=\"/?mode=tv\" method=\"get\" name=\"bouquetsel\">";
		result+="<select name=\"bouquetid\" size=\"1\" onChange=\"javascript:getNewPageTV(this.form.bouquetid.options[this.form.bouquetid.options.selectedIndex].value)\">";
		for(ePtrList<eBouquet>::iterator i(*bouquets); i != bouquets->end(); ++i)
		{
			tmp=filter_string(i->bouquet_name.c_str());
			if(tmp.find("[TV]")>-1)
			{
				result+="<option value=\"" + eString().setNum(i->bouquet_id, 10) + "\"";
				if(i->bouquet_id==bouquetid)
				{
					result+=" selected";
				}
				result+=">" + eString(i->bouquet_name.c_str()) + "</option>";
			}
		}
		result+="</select>";
		result+="<select name=\"channel\" size=\"1\" onChange=\"javascript:switchtoChannel(this.form.channel.options[this.form.channel.options.selectedIndex].value)\"><option>-----</option>";
		eBouquet *act;
		act=getBouquet(bouquetid);
		esref=act->list;
		for(std::list<eServiceReference>::iterator j = esref.begin(); j != esref.end() ; j++)
		{
			es=j->service;
			result+="<option value=\"";
			tmp.sprintf("%x:%x:%x:%x", es->service_id, es->transport_stream_id, es->original_network_id, es->service_type);
			result+=tmp;
			result+="\">";
			result+=filter_string(es->service_name.c_str());
			result+="</option>";
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
			tmp=filter_string(i->bouquet_name.c_str());
			if(tmp.find("[RADIO]")>-1)
			{
				result+="<option value=\"" + eString().setNum(i->bouquet_id, 10) + "\"";
				if(i->bouquet_id==bouquetid)
				{
					result+=" selected";
				}
				result+=">" + eString(i->bouquet_name.c_str()) + "</option>";
			}
			result+="</select>";
			result+="<select name=\"channel\" size=\"1\" onChange=\"javascript:switchtoChannel(this.form.channel.options[this.form.channel.options.selectedIndex].value)\"><option>-----</option>";
			eBouquet *act;
			act=getBouquet(bouquetid);
			esref=act->list;
			for(std::list<eServiceReference>::iterator j = esref.begin(); j != esref.end() ; j++)
			{
				es=j->service;
				result+="<option value=\"";
				tmp.sprintf("%x:%x:%x:%x", es->service_id, es->transport_stream_id, es->original_network_id, es->service_type);
				result+=tmp;
				result+="\">";
				result+=filter_string(es->service_name.c_str());
				result+="</option>";
			}
			result+="</select>";
			result+="</form>";
		}
	}
	if(result.length()<3)
		result="not ready yet";

	return result;
}

static eString getCurService()
{
	eService *current;
	current=eDVB::getInstance()->service;
	if(current)
		return current->service_name.c_str();
	else
		return "no channel selected";
}

static eString web_root(eString request, eString path, eString opts, eHTTPConnection *content)
{
	eString result="";
	QMap<eString,eString> opt=getRequestOptions(opts);
	content->local_header["Content-Type"]="text/html";

	eString mode=opt["mode"];
	eString bid="0";

	if(opt["bouquetid"])
		bid=opt["bouquetid"];

	int bouquetid=atoi(bid.c_str());

	result+=read_file(TEMPLATE_DIR+"index.tmp");

	eString radioc, tvc, aboutc, linksc, updatesc;
	eString cop;
	eString navi;
	eString stats;
	eString tmp;
	__u32 myipp=0;
	int myip[4];
	int bootcount=0;
	
	stats+="<span class=\"white\">";
	int sec=atoi(read_file("/proc/uptime").c_str());
	stats+=eString().sprintf("%d:%02dm up", sec/3600, (sec%3600)/60);
	stats+="</span> | ";

	tmp=read_file("/proc/mounts");
	if(!tmp.find("cramfs"))
	{
		stats+="<span class=\"white\">running from flash</span>";
	}
	else
	{
		stats+="<span class=\"white\">running via net</span>";
	}
	stats+=" | ";

	eConfig::getInstance()->getKey("/elitedvb/system/bootCount", bootcount);

	stats+="<span class=\"white\">"+getIP()+"</span>";

	stats+=" | ";
	tmp.sprintf("<span class=\"white\">booted enigma %d times</span><br>", bootcount);
	stats+=tmp;

	tmp.sprintf("<span class=\"white\">vpid: 0x%x</span> | <a class=\"audio\" href=\"/audio.pls\">apid: 0x%x</a>", Decoder::parms.vpid, Decoder::parms.apid);
	stats+=tmp;
	tvc="normal";
	radioc="normal";
	aboutc="normal";
	linksc="normal";
	updatesc="normal";

	if(!mode)
		mode="tv";
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

	navi ="<a class=\"";
	navi+=tvc;
	navi+="\" href=\"/?mode=tv\">tv</a> | <a class=\"";
	navi+=radioc;
	navi+="\" href=\"/?mode=radio\">radio</a> | <a class=\"";
	navi+=aboutc;
	navi+="\" href=\"/?mode=about\">about</a> | <a class=\"";
	navi+=linksc;
	navi+="\" href=\"/?mode=links\">links</a> | <a class=\"";
	navi+=updatesc;
	navi+="\" href=\"/?mode=updates\">updates</a>";

	tmp="<span class=\"titel\">"+mode+"</span>";
	tmp=tmp.upper();

	cop=getContent(mode, bouquetid);

	eString eitc;

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
			eitc+=read_file(TEMPLATE_DIR+"eit.tmp");
			eitc.strReplace("#NOWT#", now_time);
			eitc.strReplace("#NOWD#", now_duration);
			eitc.strReplace("#NOWST#", now_text);
			eitc.strReplace("#NOWLT#", filter_string(now_longtext));
			eitc.strReplace("#NEXTT#", next_time);
			eitc.strReplace("#NEXTD#", next_duration);
			eitc.strReplace("#NEXTST#", next_text);
			eitc.strReplace("#NEXTLT#", filter_string(next_longtext));
		} else {
			eitc+"eit undefined";
		}	
		eit->unlock();
	}
	else
	{
		eitc+="no eit";
	}

	result.strReplace("#STATS#", stats);
	result.strReplace("#NAVI#", navi);
	result.strReplace("#MODE#", tmp);
	result.strReplace("#COP#", cop);
	if((mode=="tv")||
           (mode=="radio"))
		result.strReplace("#SERVICENAME#", filter_string(getCurService()));
	else
		result.strReplace("#SERVICENAME#", "");
	result.strReplace("#VOLBAR#", getVolBar());
	if(mode=="tv"||mode=="radio")
		result.strReplace("#EIT#", eitc);
	else
		result.strReplace("#EIT#", "");
	return result;
}

static eString switchServiceWeb(eString request, eString path, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html";
	int service_id=-1, original_network_id=-1, transport_stream_id=-1, service_type=-1;
	if (opt.find("="))
		opt=opt.mid(opt.find("=")+1);
	if(opt)
		sscanf(opt.c_str(), "%x:%x:%x:%x", &service_id, &transport_stream_id, &original_network_id, &service_type);
	eString result="";
	
	if ((service_id!=-1) && (original_network_id!=-1) && (transport_stream_id!=-1) && (service_type!=-1))
	{
		eService *meta=0;

		meta=eDVB::getInstance()->getTransponders()->searchService(original_network_id, service_id);

		if (meta)
			eDVB::getInstance()->switchService(meta);
		else
			eDVB::getInstance()->switchService(service_id, original_network_id, transport_stream_id, service_type);
		result+="<script language=\"javascript\">window.close();</script>";
	} else
	{
		result+="<script language=\"javascript\">alert(\"ERROR wrong parms\")</script>";
	}
	return result;
}

static eString audiopls(eString request, eString path, eString opt, eHTTPConnection *content)
{
	eString result;
	eString tmp;

	content->local_header["Content-Type"]="audio/x-scpls";
	result="[playlist]\n";
	result+="NumberOfEntries=1\n";
	result+="File1=http://"+getIP()+":31338/";
        tmp.sprintf("%02x\n", Decoder::parms.apid);
 	result+=tmp;
	return result;
}

void ezapInitializeDyn(eHTTPDynPathResolver *dyn_resolver)
{
	dyn_resolver->addDyn("GET", "/", web_root);
	dyn_resolver->addDyn("GET", "/switchTo", switchServiceWeb);
	dyn_resolver->addDyn("GET", "/setVolume", setVolume);

	dyn_resolver->addDyn("GET", "/cgi-bin/status", doStatus);
	dyn_resolver->addDyn("GET", "/cgi-bin/switchService", switchService);
	dyn_resolver->addDyn("GET", "/cgi-bin/listServices", listServices);
	dyn_resolver->addDyn("GET", "/cgi-bin/admin", admin);
	dyn_resolver->addDyn("GET", "/cgi-bin/audio", audio);
	dyn_resolver->addDyn("GET", "/cgi-bin/getPMT", getPMT);

	dyn_resolver->addDyn("GET", "/audio.pls", audiopls);
	dyn_resolver->addDyn("GET", "/version", version);
	dyn_resolver->addDyn("GET", "/channels/getcurrent", channels_getcurrent);
/*	dyn_resolver->addDyn("GET", "/channels/numberchannels", channels_numberchannels);
	dyn_resolver->addDyn("GET", "/channels/gethtmlchannels", channels_gethtmlchannels);
	dyn_resolver->addDyn("GET", "/channels/getchannels", channels_getgetchannels);
	dyn_resolver->addDyn("GET", "/epg/now", epg_now);
	dyn_resolver->addDyn("GET", "/epg/next", epg_next); */
}

#include <time.h>
#include <qmap.h>
#include <qfile.h>
#include <qtextstream.h>
#include "enigma.h"
#include "decoder.h"
#include "enigma_dyn.h"
#include "http_dyn.h"
#include "dvb.h"
#include "edvb.h"

#include <config.h>

#define TEMPLATE_DIR DATADIR+QString("/enigma/templates/")


static QMap<QString,QString> getRequestOptions(QString opt)
{
	QMap<QString,QString> result;
	
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
		QString n=opt.left(e);
		QString r=opt.mid(e+1, opt.find("&", e+1)-e-1);
		result.insert(n, r);
		opt=opt.mid(a+1);
	}
	return result;
}

static QString doStatus(QString request, QString path, QString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html";
	QString result;
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
		"<tr><td>current time:</td><td>" + QString(ctime(&atime)) + "</td></tr>\n"
		"</table>\n"
		"</body>\n"
		"</html>\n";
	return result;
}

static QString switchService(QString request, QString path, QString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html";
	int service_id=-1, original_network_id=-1, transport_stream_id=-1, service_type=-1;
	if (opt.find("="))
		opt=opt.mid(opt.find("=")+1);
	sscanf(opt, "%x:%x:%x:%x", &service_id, &transport_stream_id, &original_network_id, &service_type);
	QString result="";
	
	if ((service_id!=-1) && (original_network_id!=-1) && (transport_stream_id!=-1) && (service_type!=-1))
	{
		eService *meta=0;
		for (QListIterator<eService> i(*eDVB::getInstance()->getServices()); i.current(); ++i)
		{
			if ((i.current()->service_id==service_id) &&
					(i.current()->original_network_id==original_network_id) &&
					(i.current()->transport_stream_id==transport_stream_id) &&
					(i.current()->service_type==service_type))
				meta=i.current();
		}
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

static QString listServices(QString request, QString path, QString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html";
	QString result;
	QMap<QString,QString> opt=getRequestOptions(opts);
	QString search=opt["search"];
	result="<html>\n"
		"<head>\n"
		"  <title>elitedvb service list</title>\n"
		"  <link rel=stylesheet type=\"text/css\" href=\"/index.css\">\n"
		"</head>\n"
		"<body>\n"
		"<h1>EliteDVB channel list</h1>\n"
		"<table>\n";
	for (QListIterator<eService> i(*eDVB::getInstance()->getServices()); i.current(); ++i)
	{
		eService *service=i.current();
		if (search && (service->service_name.find(search)==-1))
			continue;
		QString sc;
		sc.sprintf("%x:%x:%x:%x", service->service_id, service->transport_stream_id, service->original_network_id, service->service_type);
		result+="<tr><td><a href=\"/cgi-bin/switchService?service=" + sc + "\">" + service->service_name + "</a></td>"
						"<td>" + QString().setNum(service->service_type, 0x10) + "</td></tr>\n";
	}
	result+="</table>\n"
		"</body>\n"
		"</html>\n";
	return result;
}

static QString admin(QString request, QString path, QString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html";
	QMap<QString,QString> opt=getRequestOptions(opts);
	QString command=opt["command"];
	if (command && command=="shutdown")
	{
		eZap::getInstance()->quit();
		return "<html><head><title>Shutdown</title></head><body>Shutdown initiated.</body></html>\n";
	} else
		return "<html><head><title>Error</title></head><body>Unknown admin command.</body></html>\n";
}

static QString audio(QString request, QString path, QString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html";
	QMap<QString,QString> opt=getRequestOptions(opts);
	QString result="";
	QString volume=opt["volume"];
	if (volume)
	{
		int vol=atoi(volume);
		eDVB::getInstance()->changeVolume(1, vol);
		result+="Volume set.<br>\n";
	}
	QString mute=opt["mute"];
	if (mute)
	{
		int m=atoi(mute);
		eDVB::getInstance()->changeVolume(3, m);
		result+="mute set<br>\n";
	}
	result+=QString().sprintf("volume: %d<br>\nmute: %d<br>\n", eDVB::getInstance()->volume, eDVB::getInstance()->mute);
	return result;
}

static QString getPMT(QString request, QString path, QString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="x-application/PMT";
	PMT *pmt=eDVB::getInstance()->getPMT();
	if (!pmt)
		return "result=ERROR\n";
	QString res="result=OK\n";
	res+="PMT"+QString().sprintf("(%04x)\n", pmt->pid);
	res+="program_number="+QString().sprintf("%04x\n", pmt->program_number);
	res+="PCR_PID="+QString().sprintf("%04x\n", pmt->PCR_PID);
	res+="program_info\n";
	for (QListIterator<Descriptor> d(pmt->program_info); d.current(); ++d)
		res+=d.current()->toString();
	for (QListIterator<PMTEntry> s(pmt->streams); s.current(); ++s)
	{
		PMTEntry *e=s.current();
		res+="PMTEntry\n";
		res+="stream_type="+QString().sprintf("%02x\n", e->stream_type);
		res+="elementary_PID="+QString().sprintf("%04x\n", e->elementary_PID);
		res+="ES_info\n";
		for (QListIterator<Descriptor> d(e->ES_info); d.current(); ++d)
			res+=d.current()->toString();
	}
	pmt->unlock();
	return res;
}

static QString version(QString request, QString path, QString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/plain";
	QString result="";
		"EliteDVB Version " + eDVB::getInstance()->getVersion() + "\r\n"
		"eZap Version " + eZap::getInstance()->getVersion() + "\r\n";
	return result;
}

static QString channels_getcurrent(QString request, QString path, QString opt, eHTTPConnection *content)
{
	QString result="";
	content->local_header["Content-Type"]="text/plain";
	if (eDVB::getInstance()->service)
		result+=QString().sprintf("%d", eDVB::getInstance()->service->service_number);
	else
		result+="-1";
	return result+"\r\n";
}


static void unpack(__u32 l, int *t)
{
	for (int i=0; i<4; i++)
		*t++=(l>>((3-i)*8))&0xFF;
}

static QString getVolume()
{
	return QString().setNum((63-eDVB::getInstance()->volume)*100/63, 10);
}

static QString setVolume(QString request, QString path, QString opts, eHTTPConnection *content)
{
	QMap<QString,QString> opt=getRequestOptions(opts);
	QString mute="0";
	QString volume;
	QString result="";
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
		vol=atoi(volume);
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


static QString read_file(QString filename)
{
	QFile f(filename);

	if(f.open(IO_ReadOnly))
	{
		char buffer[f.size()+1];
		buffer[f.readBlock(buffer, f.size())]=0;
		return QString(buffer);
	} else
		return "file: "+filename+" not found\n";
}

static QString getIP()
{
#if 0
	QString tmp;
	int ip[4];
	ip[0]=0;
	ip[1]=0;
	ip[2]=0;
	ip[3]=0;

	system("cat /proc/net/tcp | grep -v \": 00000000\"> /tmp/ip_temp");
	system("cat /tmp/ip_temp | grep \"0050\" > /tmp/ip");
	tmp=read_file("/tmp/ip");
	tmp=tmp.mid(5, 9);
	
	sscanf(tmp, "%02x%02x%02x%02x", &ip[0], &ip[1], &ip[2], &ip[3]);
	tmp.sprintf("%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	return tmp;
#endif
	return "?.?.?.?";
}


static QString filter_string(QString string)
{
	string=string.replace(QRegExp("\x86"), "");
	string=string.replace(QRegExp("\x87"), "");
	string=string.replace(QRegExp("\x05"), "");
	return string;
}


eBouquet *getBouquet(int bouquet_id)
{
	QList<eBouquet> bouquets;
	
	bouquets=*eDVB::getInstance()->getBouquets();

        for (QListIterator<eBouquet> i(bouquets); i.current(); ++i)
                if (i.current()->bouquet_id==bouquet_id)
                        return i.current();
        return 0;
}


static QString getVolBar()
{
	QString result="";
	int volume=atoi(getVolume());

	result+="<table cellspacing=\"0\" cellpadding=\"0\" border=\"0\">";
	result+="<tr>";

	for(int i=1;i<=(volume/10);i++)
	{
		result+="<td><a class=\"volgreen\" href=\"javascript:setVol(";
		result+=QString().setNum(i, 10);
		result+=")\">||</a></span></td>";  
	}
	for(int i=(volume/10)+1;i<=(100/10);i++)
	{
		result+="<td><a class=\"volnot\" href=\"javascript:setVol(";
		result+=QString().setNum(i, 10);
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


static QString getContent(QString mode, int bouquetid)
{
	QString result="";
	QString tmp="";
	QList<eBouquet> bouquets;
	QList<eServiceReference> esref;
	eService *es;

	bouquets=*eDVB::getInstance()->getBouquets();
 
	bouquets.sort();

	if(mode=="tv")
	{
		result+="<form action=\"/?mode=tv\" method=\"get\" name=\"bouquetsel\">";
		result+="<select name=\"bouquetid\" size=\"1\" onChange=\"javascript:getNewPageTV(this.form.bouquetid.options[this.form.bouquetid.options.selectedIndex].value)\">";
		for(QListIterator<eBouquet> i(bouquets); i.current(); ++i)
		{
			tmp=filter_string(i.current()->bouquet_name);
			if(tmp.find("[TV]")>-1)
			{
				result+="<option value=\"" + QString().setNum(i.current()->bouquet_id, 10) + "\"";
				if(i.current()->bouquet_id==bouquetid) 
				{
					result+=" selected";
				}
				result+=">" + i.current()->bouquet_name + "</option>";
		}
	}
	result+="</select>";
	result+="<select name=\"channel\" size=\"1\" onChange=\"javascript:switchtoChannel(this.form.channel.options[this.form.channel.options.selectedIndex].value)\"><option>-----</option>";
	eBouquet *act;
	act=getBouquet(bouquetid);
	esref=act->list;
	for(QListIterator<eServiceReference> j(esref); j.current(); ++j)
	{
		es=j.current()->service;
		result+="<option value=\"";
		tmp.sprintf("%x:%x:%x:%x", es->service_id, es->transport_stream_id, es->original_network_id, es->service_type);
		result+=tmp;
		result+="\">";
		result+=filter_string(es->service_name);
		result+="</option>";
	}
	result+="</select>";
	result+="</form>";
	}

	if(mode=="radio")
	{
		result+="<form action=\"/?mode=radio\" method=\"get\" name=\"bouquetsel\">";
		result+="<select name=\"bouquetid\" size=\"1\" onChange=\"javascript:getNewPageRadio(this.form.bouquetid.options[this.form.bouquetid.options.selectedIndex].value)\">";
		for(QListIterator<eBouquet> i(bouquets); i.current(); ++i)
		{
			tmp=filter_string(i.current()->bouquet_name);
			if(tmp.find("[Radio]")>-1)
			{
				result+="<option value=\"" + QString().setNum(i.current()->bouquet_id, 10) + "\"";
				if(i.current()->bouquet_id==bouquetid) 
				{
					result+=" selected";
				}
				result+=">" + i.current()->bouquet_name + "</option>";
			}
		}
		result+="</select>";
		result+="<select name=\"channel\" size=\"1\" onChange=\"javascript:switchtoChannel(this.form.channel.options[this.form.channel.options.selectedIndex].value)\"><option>-----</option>";
		eBouquet *act;
		act=getBouquet(bouquetid);
		esref=act->list;
		for(QListIterator<eServiceReference> j(esref); j.current(); ++j)
		{
			es=j.current()->service;
			result+="<option value=\"";
			tmp.sprintf("%x:%x:%x:%x", es->service_id, es->transport_stream_id, es->original_network_id, es->service_type);
			result+=tmp;
			result+="\">";
			result+=filter_string(es->service_name);
			result+="</option>";
		}
		result+="</select>";
		result+="</form>";
	}



	 if(result.length()<3)
		result="not ready yet";

	return result;
}

static QString getCurService()
{
	eService *current;
	current=eDVB::getInstance()->service;
	if(current)
		return current->service_name;
	else
		return "no channel selected";
}

static QString web_root(QString request, QString path, QString opts, eHTTPConnection *content)
{
	QString result="";
	QMap<QString,QString> opt=getRequestOptions(opts);
	content->local_header["Content-Type"]="text/html";

	QString mode=opt["mode"];
	QString bid="0";

	if(opt["bouquetid"])
		bid=opt["bouquetid"];

	int bouquetid=atoi(bid);

	result+=read_file(TEMPLATE_DIR+"index.tmp");
 
	QString radioc, tvc, aboutc, linksc, updatesc;
	QString cop;
	QString navi;
	QString stats;
	QString tmp;
	__u32 myipp=0;
	int myip[4];
	int bootcount=0;
	
	stats+="<span class=\"white\">";
	int sec=atoi(read_file("/proc/uptime"));
	stats+=QString().sprintf("%d:%02dm up", sec/3600, (sec%3600)/60);
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

	eDVB::getInstance()->config.getKey("/elitedvb/system/bootCount", bootcount);

	stats+="<span class=\"white\">"+getIP()+"</span>";

	stats+=" | ";
	tmp.sprintf("<span class=\"white\">booted enigma %d times</span><br>", bootcount);
	stats+=tmp;

	tmp.sprintf("<span class=\"white\">vpid: 0x%x</span> | <a class=\"audio\" href=\"http://"+getIP()+"/audio.pls\">apid: 0x%x</a>", Decoder::parms.vpid, Decoder::parms.apid);
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

	QString eitc;

	EIT *eit=eDVB::getInstance()->getEIT();
	
	if(eit)
	{
		QString now_time="", now_duration="", now_text="", now_longtext="";
		QString next_time="", next_duration="", next_text="", next_longtext="";
 
		int p=0;

		for(QListIterator<EITEvent> i(eit->events); i.current(); ++i)
		{
			EITEvent *event=i.current();
			if(event)
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
				for(QListIterator<Descriptor> d(event->descriptor); d.current(); ++d)
			        {
					Descriptor *descriptor=d.current();
					if(descriptor->Tag()==DESCR_SHORT_EVENT)
					{
						ShortEventDescriptor *ss=(ShortEventDescriptor*)descriptor;
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
						ExtendedEventDescriptor *ss=(ExtendedEventDescriptor*)descriptor;
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
			eitc.replace(QRegExp("#NOWT#"), now_time);
			eitc.replace(QRegExp("#NOWD#"), now_duration);
			eitc.replace(QRegExp("#NOWST#"), now_text);
			eitc.replace(QRegExp("#NOWLT#"), filter_string(now_longtext));
			eitc.replace(QRegExp("#NEXTT#"), next_time);
			eitc.replace(QRegExp("#NEXTD#"), next_duration);
			eitc.replace(QRegExp("#NEXTST#"), next_text);
			eitc.replace(QRegExp("#NEXTLT#"), filter_string(next_longtext));
		} else {
			eitc+"eit undefined";
		}	
		eit->unlock();
	}
	else
	{
		eitc+="no eit";
	}

	result.replace(QRegExp("#STATS#"), stats);
	result.replace(QRegExp("#NAVI#"), navi);
	result.replace(QRegExp("#MODE#"), tmp);
	result.replace(QRegExp("#COP#"), cop);
	if((mode=="tv")||
           (mode=="radio")) 
		result.replace(QRegExp("#SERVICENAME#"), filter_string(getCurService()));
	else
		result.replace(QRegExp("#SERVICENAME#"), "");
	result.replace(QRegExp("#VOLBAR#"), getVolBar());
	if(mode=="tv"||mode=="radio")
		result.replace(QRegExp("#EIT#"), eitc);
	else
		result.replace(QRegExp("#EIT#"), QString(""));
	return result;
}

static QString switchServiceWeb(QString request, QString path, QString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html";
	int service_id=-1, original_network_id=-1, transport_stream_id=-1, service_type=-1;
	if (opt.find("="))
		opt=opt.mid(opt.find("=")+1);
        if(opt)
 	 sscanf(opt, "%x:%x:%x:%x", &service_id, &transport_stream_id, &original_network_id, &service_type);
	QString result="";
	
	if ((service_id!=-1) && (original_network_id!=-1) && (transport_stream_id!=-1) && (service_type!=-1))
	{
		eService *meta=0;
		for (QListIterator<eService> i(*eDVB::getInstance()->getServices()); i.current(); ++i)
		{
			if ((i.current()->service_id==service_id) &&
					(i.current()->original_network_id==original_network_id) &&
					(i.current()->transport_stream_id==transport_stream_id) &&
					(i.current()->service_type==service_type))
				meta=i.current();
		}
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

static QString audiopls(QString request, QString path, QString opt, eHTTPConnection *content)
{
	QString result;
	QString tmp;

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

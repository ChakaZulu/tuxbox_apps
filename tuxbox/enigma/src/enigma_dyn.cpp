#include <time.h>
#include <qmap.h>
#include "ezap.h"
#include "ezap_dyn.h"
#include "http_dyn.h"
#include "dvb.h"
#include "edvb.h"

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

static QString doStatus(QString request, QString path, QString opt, const eHTTPConnection *content)
{
	QString result;
	time_t atime;
	time(&atime);
	atime+=eDVB::getInstance()->time_difference;
	result ="Content-Type: text/html\r\n";
	result+="\r\n";
	result+="<html>\n"
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

static QString switchService(QString request, QString path, QString opt, const eHTTPConnection *content)
{
	int service_id=-1, original_network_id=-1, transport_stream_id=-1, service_type=-1;
	if (opt.find("="))
		opt=opt.mid(opt.find("=")+1);
	sscanf(opt, "%x:%x:%x:%x", &service_id, &transport_stream_id, &original_network_id, &service_type);
	QString result;
	
	result="Content-Type: text/html\r\n";
	result+="\r\n";
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

static QString listServices(QString request, QString path, QString opts, const eHTTPConnection *content)
{
	QString result;
	QMap<QString,QString> opt=getRequestOptions(opts);
	QString search=opt["search"];
	result ="Content-Type: text/html\r\n";
	result+="\r\n";
	result+="<html>\n"
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

static QString admin(QString request, QString path, QString opts, const eHTTPConnection *content)
{
	QMap<QString,QString> opt=getRequestOptions(opts);
	QString command=opt["command"];
	if (command && command=="shutdown")
	{
		eZap::getInstance()->quit();
		return "Content-Type: text/html\r\n\r\n<html><head><title>Shutdown</title></head><body>Shutdown initiated.</body></html>\n";
	} else
		return "Content-Type: text/html\r\n\r\n<html><head><title>Error</title></head><body>Unknown admin command.</body></html>\n";
}

static QString audio(QString request, QString path, QString opts, const eHTTPConnection *content)
{
	QMap<QString,QString> opt=getRequestOptions(opts);
	QString result="Content-Type: text/html\r\n\r\n";
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

static QString getPMT(QString request, QString path, QString opt, const eHTTPConnection *content)
{
	PMT *pmt=eDVB::getInstance()->getPMT();
	if (!pmt)
		return "Content-Type: x-application/PMT\r\nresult=ERROR\n";
	QString res="Content-Type: x-application/PMT\r\nresult=OK\n";
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

static QString version(QString request, QString path, QString opt, const eHTTPConnection *content)
{
	QString result="Content-Type: text/plain\r\n\r\n"
		"EliteDVB Version " + eDVB::getInstance()->getVersion() + "\r\n"
		"eZap Version " + eZap::getInstance()->getVersion() + "\r\n";
	return result;
}

static QString channels_getcurrent(QString request, QString path, QString opt, const eHTTPConnection *content)
{
	QString result="Content-Type: text/plain\r\n\r\n";
	if (eDVB::getInstance()->service)
		result+=QString().sprintf("%d", eDVB::getInstance()->service->service_number);
	else
		result+="-1";
	return result+"\r\n";
}

void ezapInitializeDyn(eHTTPDynPathResolver *dyn_resolver)
{
	dyn_resolver->addDyn("GET", "/cgi-bin/status", doStatus);
	dyn_resolver->addDyn("GET", "/cgi-bin/switchService", switchService);
	dyn_resolver->addDyn("GET", "/cgi-bin/listServices", listServices);
	dyn_resolver->addDyn("GET", "/cgi-bin/admin", admin);
	dyn_resolver->addDyn("GET", "/cgi-bin/audio", audio);
	dyn_resolver->addDyn("GET", "/cgi-bin/getPMT", getPMT);

	dyn_resolver->addDyn("GET", "/version", version);
	dyn_resolver->addDyn("GET", "/channels/getcurrent", channels_getcurrent);
/*	dyn_resolver->addDyn("GET", "/channels/numberchannels", channels_numberchannels);
	dyn_resolver->addDyn("GET", "/channels/gethtmlchannels", channels_gethtmlchannels);
	dyn_resolver->addDyn("GET", "/channels/getchannels", channels_getgetchannels);
	dyn_resolver->addDyn("GET", "/epg/now", epg_now);
	dyn_resolver->addDyn("GET", "/epg/next", epg_next); */
}

#include <lib/system/http_dyn.h>
#include <lib/dvb/service.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/servicemp3.h>

extern eString httpUnescape(const eString &string);
extern eString httpEscape(const eString &string);
extern std::map<eString,eString> getRequestOptions(eString opt);
extern eString ref2string(const eServiceReference &r);
extern eServiceReference string2ref(const eString &service);

eString xmlEscape(const eString &string)
{
	eString ret="";
	for (unsigned int i=0; i<string.length(); ++i)
	{
		int c=string[i];
		
		if (c == '&')
			ret+="&amp;";
		else
			ret+=c;
	}
	return ret;
}


static const eString xmlversion="<?xml version=\"1.0\"?>\n";
static inline eString xmlstylesheet(const eString &ss)
{
	return eString("<?xml-stylesheet type=\"text/xsl\" href=\"/stylesheets/") + ss + ".xsl\"?>\n";
}

static eString web_root(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString ret;
	content->local_header["Content-Type"]="text/xml; charset=utf-8";
	
	ret=xmlversion;
	ret+=xmlstylesheet("services");
	ret+="<services>\n";
	
	for (int i=0; i<10; ++i)
		ret+="<service><name>" + eString().sprintf("Service #%d", i) + "</name></service>\n";
	
	ret+="</services>\n";
	return ret;
}

class eServiceToXml: public Object
{
	eString &result;
	eServiceInterface &iface;
public:
	eServiceToXml(eString &result, eServiceInterface &iface): result(result), iface(iface)
	{
	}
	void addEntry(const eServiceReference &e)
	{
		result+="<service>\n";
		result+="<reference>" + ref2string(e) + "</reference>\n";
		eService *service=iface.addRef(e);
		if (service)
		{
			result+="<name>" + xmlEscape(service->service_name) + "</name>\n";
			if (service->dvb)
			{
				result+="<dvb><namespace>";
				result+=eString().setNum(service->dvb->dvb_namespace.get(), 0x10);
				result+="</namespace><tsid>";
				result+=eString().setNum(service->dvb->transport_stream_id.get(), 0x10);
				result+="</tsid><onid>";
				result+=eString().setNum(service->dvb->original_network_id.get(), 0x10);
				result+="</onid><sid>";
				result+=eString().setNum(service->dvb->service_id.get(), 0x10);
				result+="</sid><type>";
				result+=eString().setNum(service->dvb->service_type, 0x10);
				result+="</type><provider>";
				result+=xmlEscape(service->dvb->service_provider);
				result+="</provider><number>";
				result+=eString().setNum(service->dvb->service_number, 10);
				result+="</number></dvb>\n";
			}
#ifndef DISABLE_FILE
			if (service->id3)
			{
				std::map<eString, eString> & tags = service->id3->getID3Tags();
				result+="<id3>";
				for (std::map<eString, eString>::iterator i(tags.begin()); i != tags.end(); ++i)
					result+="<tag id=\"" + i->first + "\"><" + i->second + "<tag/>\n";
				result+="</id3>";
			}
#endif
		}
		iface.removeRef(e);
		result+="</service>\n";
	}
};

static eString xml_services(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	std::map<eString,eString> opts=getRequestOptions(opt);
	eString spath=opts["path"];
	
	eServiceInterface *iface=eServiceInterface::getInstance();

	if (!iface)
		return "n/a\n";

	eString current;

	unsigned int pos;
	if ((pos=spath.rfind(';')) != eString::npos)
	{
		current=spath.mid(pos+1);
		spath=spath.left(pos);
	} else
	{
		current=spath;
		spath="";
	}
	
	eServiceReference current_service=string2ref(current);
	
	if (!opts["path"])
		current_service=eServiceReference(eServiceReference::idStructure,
			eServiceReference::isDirectory, 0);
	
	eDebug("current_service: %s", current_service.path.c_str());

	eString res;
	
	eServiceToXml conv(res, *iface);

	Signal1<void,const eServiceReference&> signal;
	signal.connect(slot(conv, &eServiceToXml::addEntry));

	res=xmlversion;
	res+=xmlstylesheet("services");
	res+="<services>\n";

	iface->enterDirectory(current_service, signal);
	iface->leaveDirectory(current_service);
	
	res+="</services>\n";

	return res;
}

class eHTTPLog: public eHTTPDataSource, public Object
{
	int mask, format;
	int ok, last;
	void recvMessage(int lvl, const eString &str);
	eString toWrite;
public:
	eHTTPLog(eHTTPConnection *c, int mask, int format);
	~eHTTPLog();
	
	int doWrite(int);
};

eHTTPLog::eHTTPLog(eHTTPConnection *c, int mask, int format):
	eHTTPDataSource(c), mask(mask), format(format), ok(0)
{
	if (format == 0)
		connection->local_header["Content-Type"]="text/plain";
	else if (format == 1)
		connection->local_header["Content-Type"]="text/html";
	connection->code=200;
	connection->code_descr="OK";
	CONNECT(logOutput, eHTTPLog::recvMessage);
	last = -1;
	if (format == 1)
	{
		toWrite="<html><head>" 
		"<link type=\"text/css\" rel=\"stylesheet\" href=\"/stylesheets/log.css\">"
		"<title>Enigma Event Log</title>"
		"</head><body><pre>\n";
	}
}

int eHTTPLog::doWrite(int hm)
{
	// we don't have YET data to send (but there's much to come)
	ok=1;
	if (toWrite.size())
	{
		connection->writeBlock(toWrite.c_str(), toWrite.size());
		toWrite="";
	}
	return 0;
}

void eHTTPLog::recvMessage(int lvl, const eString &msg)
{
	eString res;
	if (lvl & mask)
	{
		if (format == 0) // text/plain
		{
			res=msg;
			res.strReplace("\n", "\r\n");
		} else
		{
			if (last != lvl)
			{
				eString cl="unknown";
				if (lvl == lvlWarning)
					cl="warning";
				else if (lvl == lvlFatal)
					cl="fatal";
				else if (lvl == lvlDebug)
					cl="debug";
				
				if (last != -1)
					res+="</div>";
				res+="<div class=\"" + cl + "\">";
				last=lvl;
			}
			res+=msg;
			// res.strReplace("\n", "<br>\n");  <-- we are <pre>, so no need
		}
		if (ok)
			connection->writeBlock(res.c_str(), res.size());
		else
			toWrite+=res;
	}
}

eHTTPLog::~eHTTPLog()
{
}

class eHTTPLogResolver: public eHTTPPathResolver
{
public:
	eHTTPLogResolver();
	eHTTPDataSource *getDataSource(eString request, eString path, eHTTPConnection *conn);
};

eHTTPLogResolver::eHTTPLogResolver()
{
}

eHTTPDataSource *eHTTPLogResolver::getDataSource(eString request, eString path, eHTTPConnection *conn)
{
	if ((path=="/log/debug") && (request=="GET"))
		return new eHTTPLog(conn, -1, 0);
	if ((path=="/log/warn") && (request=="GET"))
		return new eHTTPLog(conn, 3, 0);
	if ((path=="/log/crit") && (request=="GET"))
		return new eHTTPLog(conn, 1, 0);

	if ((path=="/log/debug.html") && (request=="GET"))
		return new eHTTPLog(conn, -1, 1);
	if ((path=="/log/warn.html") && (request=="GET"))
		return new eHTTPLog(conn, 3, 1);
	if ((path=="/log/crit.html") && (request=="GET"))
		return new eHTTPLog(conn, 1, 1);
	return 0;
}

void ezapInitializeWeb(eHTTPD *httpd, eHTTPDynPathResolver *dyn_resolver)
{
	dyn_resolver->addDyn("GET", "/dyn2/", web_root);
	dyn_resolver->addDyn("GET", "/dyn2/services", xml_services);
	httpd->addResolver(new eHTTPLogResolver);
}


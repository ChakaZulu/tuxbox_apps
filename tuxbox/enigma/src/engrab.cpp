#include <engrab.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <lib/gui/ebutton.h>
#include <lib/gui/emessage.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/service.h>
#include <lib/dvb/dvb.h>
/*#include <lib/dvb/dvbservice.h>*/
#include <lib/dvb/epgcache.h>
//#include <lib/base/estring.h>
//#define TMP_NgrabXML "/var/tmp/e-ngrab.xml"
#include <lib/gui/enumber.h>
#include <lib/gui/statusbar.h>

static eString getServiceName()
{
	eService *current=eServiceInterface::getInstance()->addRef( eServiceInterface::getInstance()->service );
	if(!current)
		return "no channel selected";

	eString sname = current->service_name;

	eServiceInterface::getInstance()->removeRef( eServiceInterface::getInstance()->service );

	return sname;
}

static eString getEPGTitle()
{
	eServiceReference &ref = eServiceInterface::getInstance()->service;

	eString evtName("not available");

	EITEvent *event=eEPGCache::getInstance()->lookupEvent((eServiceReferenceDVB&)ref);

	if(!event)
		return evtName;

	for(ePtrList<Descriptor>::iterator d(event->descriptor); d!= event->descriptor.end(); ++d)
		if(d->Tag()==DESCR_SHORT_EVENT)
		{
			evtName=((ShortEventDescriptor*)*d)->event_name;
			delete event;
			break;
		}

	return evtName;
}


ENgrab::ENgrab()
	:sd(0), timeout(eApp)
{
}

ENgrab::~ENgrab()
{
}
 
eString ENgrab::startxml()
{
	eString xmlstart;

	xmlstart+="<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n";
	xmlstart+=" <neutrino commandversion=\"1\">\n";
	xmlstart+="   <record command=\"record\">\n";
	xmlstart+="    <channelname>"+getServiceName()+"</channelname>\n";//übernommen von trh
	xmlstart+="    <epgtitle>"+getEPGTitle()+"</epgtitle>\n"; //übernommen von trh
	xmlstart+="    <onidsid>123456</onidsid>\n"; // keine ahnung aber wies aussieht wird die sid und die onid nicht gebraucht von ngrab
	xmlstart+="    <epgid>123456</epgid>\n"; // und die epgid auch nicht
	xmlstart+="    <videopid>"+eString().sprintf("%d", Decoder::parms.vpid)+"</videopid>\n";
	xmlstart+="    <audiopids selected=\""+eString().sprintf("%d", Decoder::parms.apid)+"\">\n";
	xmlstart+="       <audio pid=\""+eString().sprintf("%d", Decoder::parms.apid)+"\" name=\"standard\"/>\n";
	xmlstart+="    </audiopids>\n";
	xmlstart+="  </record>\n";
	xmlstart+=" </neutrino>\n";


#ifdef TMP_NgrabXML
	std::fstream xmlstartf;
	char* Name0 =TMP_NgrabXML;
	xmlstartf.open(Name0,ios::out);
	xmlstartf<<xmlstart<<endl;
	xmlstartf.close();
#endif

	return xmlstart;
}

eString ENgrab::stopxml()
{
	eString xmlstop;

	xmlstop+="<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n";
	xmlstop+=" <neutrino commandversion=\"2\">\n";
	xmlstop+="   <record command=\"stop\">\n";
	xmlstop+="    <channelname></channelname>\n";
	xmlstop+="    <epgtitle></epgtitle>\n";
	xmlstop+="    <onidsid></onidsid>\n";
	xmlstop+="    <epgid></epgid>\n";
	xmlstop+="    <videopid></videopid>\n";
	xmlstop+="    <audiopids selected=\"\">\n";
	xmlstop+="       <audio pid=\"\" name=\"\"/>\n";
	xmlstop+="    </audiopids>\n";
	xmlstop+="  </record>\n";
	xmlstop+=" </neutrino>\n";

#ifdef TMP_NgrabXML   
	std::fstream xmlstopf;
	char* Name0 =TMP_NgrabXML;
	xmlstopf.open(Name0,ios::out);
	xmlstopf<<xmlstop<<endl;
	xmlstopf.close();
#endif
	return xmlstop;
}

void ENgrab::sendstart()
{
	eDebug("ngrab sendstart requested");
	sendStr = startxml();
	sending();
}

void ENgrab::sendstop()
{
	eDebug("ngrab sendstop requested");
	sendStr = stopxml();
	sending();
}

void ENgrab::connected()
{
	eDebug("connection to ngrab established");
	sd->writeBlock(sendStr.c_str(), sendStr.length());	
}

void ENgrab::dataWritten( int bytes )
{
	eDebug("all data to ngrab written...close connection");
	if ( !sd->bytesToWrite() )
		sd->close();
}

void ENgrab::connectionTimeouted()
{
	eDebug("could not connect to ngrab server...connection timeout");
	connectionClosed();
}

void ENgrab::connectionClosed()
{
	eDebug("ngrab connection closed");
	delete sd;
	delete this;
}

void ENgrab::sending()
{
	struct in_addr sinet_address;
	eString hostname;
	int port;
	int de[4];

	eConfig::getInstance()->getKey("/elitedvb/network/nserver", sinet_address.s_addr);
	eConfig::getInstance()->getKey("/elitedvb/network/nservport", port);
	eNumber::unpack(sinet_address, de);
	hostname=eString().sprintf("%d.%d.%d.%d", de[0], de[1], de[2], de[3]);

	if (!sd)
	{
		timeout.start(120000,true); // connection timeout
		CONNECT( timeout.timeout, ENgrab::connectionTimeouted );
		sd=new eSocket(eApp);
		CONNECT( sd->connected_, ENgrab::connected );
		CONNECT( sd->connectionClosed_, ENgrab::connectionClosed );
		CONNECT( sd->bytesWritten_, ENgrab::dataWritten );
		eDebug("connect to ngrab server at %s:%d", hostname.c_str(), port);
		sd->connectToHost(hostname, port);
	}
}

void ENgrabWnd::onBackSelected()
{
	close(0);
}

ENgrabWnd::ENgrabWnd()
	:eWindow(1), lb(this)
{
	move( ePoint(100,100));  // Fenster verschieben
	cresize( eSize(400, 200 )); // Grösse festlegen
	lb.cmove( ePoint(0,0));
	lb.cresize( eSize( 400,200 ));
	setText(_("ngrab recording"));

	CONNECT((new eListBoxEntryMenu(&lb, _("[close]"), _("close Window") ))->selected, ENgrabWnd::onBackSelected);
	CONNECT((new eListBoxEntryMenu(&lb, _("start recording"), _("start manual recording") ))->selected, ENgrabWnd::manualStart);
	CONNECT((new eListBoxEntryMenu(&lb, _("stopp recording"), _("stop manual recording") ))->selected, ENgrabWnd::manualStop);
	lb.moveSelection(eListBox<eListBoxEntryMenu>::dirFirst);
}

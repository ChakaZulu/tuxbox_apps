#include <errno.h>
#include <time.h>
#include <malloc.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include <config.h>

#include <core/base/i18n.h>
#include <core/driver/rc.h>
#include <core/dvb/service.h>
#include <core/dvb/dvb.h>
#include <core/dvb/edvb.h>
#include <core/gdi/gfbdc.h>
#include <core/gdi/glcddc.h>
#include <core/gui/emessage.h>
#include <core/gui/actions.h>
#include <core/system/econfig.h>
#include <core/system/httpd.h>
#include <core/system/http_file.h>
#include <core/system/http_dyn.h>
#include <core/system/init.h>
#include <core/gui/ebutton.h>
#include <core/gui/actions.h>
#include <core/driver/rc.h>
#include <core/dvb/dvbservice.h>
#include <core/dvb/decoder.h>

#include <core/system/xmlrpc.h>
#include <apps/enigma/enigma.h>
#include <apps/enigma/enigma_dyn.h>
#include <apps/enigma/enigma_xmlrpc.h>
#include <apps/enigma/enigma_main.h>
#include <apps/enigma/sselect.h>

eZap *eZap::instance;

static char copyright[]="enigma, Copyright (C) dbox-Project\n"
"enigma comes with ABSOLUTELY NO WARRANTY\n"
"This is free software, and you are welcome\n"
"to redistribute it under certain conditions.\n"
"It is licensed under the GNU General Public License,\n"
"Version 2\n";

eZap *eZap::getInstance()
{
	return instance;
}

void eZap::keyEvent(const eRCKey &key)
{
	if (focus)
		focus->event(eWidgetEvent(eWidgetEvent::evtKey, key));
	else if (main)
		main->event(eWidgetEvent(eWidgetEvent::evtKey, key));
}

void eZap::status()
{
}

bool eZap::setMode(int m)
{
	if ( (m != TV && m != Radio) || m == mode )
		return false;

	mode=m;	

	__u32 channel;

	if (mode == TV)
		channel = lastTvChannel;
	else
		channel = lastRadioChannel;

	serviceSelector->actualize();

	if ( eDVB::getInstance()->settings->getTransponders() )
	{
		const eServiceReferenceDVB *ref=eDVB::getInstance()->settings->getTransponders()->searchService(eOriginalNetworkID(channel>>16), eServiceID(channel&0xFFFF));

		if (eDVB::getInstance()->getServiceAPI())
		{
			if (mode == TV)	
				lastRadioChannel = (__u32) ( (eDVB::getInstance()->getServiceAPI()->service.getOriginalNetworkID().get() << 16) | eDVB::getInstance()->getServiceAPI()->service.getServiceID().get() );
			else
				lastTvChannel = (__u32) ( (eDVB::getInstance()->getServiceAPI()->service.getOriginalNetworkID().get() << 16) | eDVB::getInstance()->getServiceAPI()->service.getServiceID().get() );

			if (ref)			
				eServiceInterface::getInstance()->play(*ref);
			else
				Decoder::Flush();

		 	return true;
		}
	}
	return false;
}

eZap::eZap(int argc, char **argv): eApplication(/*argc, argv, 0*/)
{
	int bootcount;

	const eServiceReferenceDVB* currentService=0;

	eZapLCD *pLCD;
	eHTTPD *httpd;
	eHTTPDynPathResolver *dyn_resolver;
	eHTTPFilePathResolver *fileresolver;

	instance = this;

	init = new eInit();
	init->setRunlevel(6);
	
#if 0
	if(0)
	{
		gDC &dc=*gFBDC::getInstance();

		gPainter p(dc);
		
		p.clear();
		p.flush();
		p.setForegroundColor(gColor(0x13));
		p.fill(eRect(0, 0, 720, 576));
		
		eRect x(10, 10, 100, 50);
		p.setFont(gFont("NimbusSansL-Regular Sans L Regular", 30));
		for (int i=0; i<100; i++)
		{
			x.moveBy(5, 5);
//			gPainter p(dc);
			p.setForegroundColor(gColor(0x13^i));
			p.renderText(x, "Hello world dies ist ein ganz langer text der auf den screen gepinselt wird du lieber mensch bla keine ahnung hallo was soll das");
		}
	}
#endif

	focus = 0;
	CONNECT(eRCInput::getInstance()->keyEvent, eZap::keyEvent);

	desktop_fb=new eWidget();
	desktop_fb->setName("desktop_fb");
	desktop_fb->move(ePoint(0, 0));
	desktop_fb->resize(eSize(720, 576));
	desktop_fb->setTarget(gFBDC::getInstance());
	desktop_fb->makeRoot();
	desktop_fb->setBackgroundColor(gColor(0));
	desktop_fb->show();
	
	desktop_lcd=new eWidget();
	desktop_lcd->setName("desktop_lcd");
	desktop_lcd->move(ePoint(0, 0));
	desktop_lcd->resize(eSize(128, 64));
	desktop_lcd->setTarget(gLCDDC::getInstance());
	desktop_lcd->setBackgroundColor(gColor(0));
	desktop_lcd->show();

	eDebug("[ENIGMA] loading default keymaps...");
	
	eActionMapList::getInstance()->loadXML( DATADIR "/enigma/resources/rcdreambox.xml");
	eActionMapList::getInstance()->loadXML( DATADIR "/enigma/resources/rcdreambox2.xml");
	eActionMapList::getInstance()->loadXML( DATADIR "/enigma/resources/rcdboxold.xml");
	eActionMapList::getInstance()->loadXML( DATADIR "/enigma/resources/rcdboxnew.xml");
	eActionMapList::getInstance()->loadXML( DATADIR "/enigma/resources/rcdboxbuttons.xml");

	eDVB::getInstance()->configureNetwork();
	eDebug("<-- network");

	__u32 tmp;

//	eConfig::getInstance()->setKey("/ezap/ui/lastchannel", tmp);

	if (!eConfig::getInstance()->getKey("/ezap/ui/lastchannel", tmp))
	{
			eDebug("Erase old config Files");
			eConfig::getInstance()->delKey("/ezap/ui/lastchannel");
			eConfig::getInstance()->delKey("/ezap/ui/skin");
			
			unlink(CONFIGDIR"/enigma/services");
			unlink(CONFIGDIR"/enigma/bouquets");
	}

	main = new eZapMain();
	eDebug("<-- eZapMain");

	pLCD = eZapLCD::getInstance();
	eDebug("<-- pLCD");

	if (eConfig::getInstance()->getKey("/ezap/ui/lastTvChannel", lastTvChannel))
		lastTvChannel=0;

	if (eConfig::getInstance()->getKey("/ezap/ui/lastRadioChannel", lastRadioChannel))
		lastRadioChannel=0;

	if (eConfig::getInstance()->getKey("/ezap/ui/mode", mode))
		mode=0;  // default TV mode

	if ( (mode && lastRadioChannel) || (!mode && lastTvChannel) )
	{
		__u32 channel;

		if (mode == TV)
			channel = lastTvChannel;
		else
			channel = lastRadioChannel;		

		currentService=eDVB::getInstance()->settings->getTransponders()->searchService(eOriginalNetworkID(channel>>16), eServiceID(channel&0xFFFF));

		if (currentService && eDVB::getInstance()->getServiceAPI())
				eServiceInterface::getInstance()->play(*currentService);
	}

	serviceSelector = new eServiceSelector();
	eDebug("<-- service selector");

	serviceSelector->setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
	eDebug("..");

	dyn_resolver = new eHTTPDynPathResolver();
	ezapInitializeDyn(dyn_resolver);

	fileresolver = new eHTTPFilePathResolver();
	fileresolver->addTranslation("/var/tuxbox/htdocs", "/www"); /* TODO: make user configurable */
	fileresolver->addTranslation("/", "/h4x0r");  // *REMOVE* before commit!
	fileresolver->addTranslation(DATADIR "/enigma/htdocs", "/");

	eDebug("[ENIGMA] starting httpd");
	httpd = new eHTTPD(80);
	ezapInitializeXMLRPC(httpd);
	httpd->addResolver(dyn_resolver);
	httpd->addResolver(fileresolver);


	eDebug("[ENIGMA] ok, beginning mainloop");

/*
	{
		eMessageBox msg("Warning:\nThis version of enigma contains incomplete code.\n"
			"Not working are:\n - Satconfig\n - Cablescan (will be back soon)\n"
			" - some other stuff", "Enigma *pre*");
		msg.show();
		msg.exec();
		msg.hide();
	}*/

	if (eConfig::getInstance()->getKey("/elitedvb/system/bootCount", bootcount))
	{
		bootcount = 1;
		eMessageBox msg("Willkommen zu enigma.\n\nBitte führen sie zunächst eine Kanalsuche durch, indem sie die d-Box-Taste drücken um in das "
			"Hauptmenü zu gelangen. Dort gibt es den Unterpunkt \"Transponder Scan\", der genau das macht, was sie glauben.\n", "enigma - erster Start");
		msg.show();
		msg.exec();
		msg.hide();
	}
	else
		bootcount++;

	eConfig::getInstance()->setKey("/elitedvb/system/bootCount", bootcount);

	init->setRunlevel(10);
}

eZap::~eZap()
{
	if (eDVB::getInstance()->getServiceAPI() &&
			eDVB::getInstance()->getServiceAPI()->service != eServiceReferenceDVB())
	{
		__u32 channel = (__u32) ( (eDVB::getInstance()->getServiceAPI()->service.getOriginalNetworkID().get() << 16 ) | eDVB::getInstance()->getServiceAPI()->service.getServiceID().get() );

		eConfig::getInstance()->setKey("/ezap/ui/mode", mode);

		eDebug("Save lastTvChannel = %u, lastRadioChannel = %u, channel = %u", lastTvChannel, lastRadioChannel, channel);

		if (mode == TV)
		{
			eDebug("Save channel as lastTvChannel");
			eConfig::getInstance()->setKey("/ezap/ui/lastRadioChannel", lastRadioChannel);
			eConfig::getInstance()->setKey("/ezap/ui/lastTvChannel", channel);
		}
		else
		{
			eConfig::getInstance()->setKey("/ezap/ui/lastTvChannel", lastTvChannel);
			eConfig::getInstance()->setKey("/ezap/ui/lastRadioChannel", channel);
		}
	}

	eDebug("[ENIGMA] beginning clean shutdown");
	eDebug("[ENIGMA] main");
	delete main;
	eDebug("[ENIGMA] serviceSelector");
	delete serviceSelector;
	eDebug("[ENIGMA] fertig");
	init->setRunlevel(-1);
	delete init;
	instance = 0;
}

int main(int argc, char **argv)
{
	time_t t=0;
	stime(&t);
	eDebug("%s", copyright);

	setlocale (LC_ALL, "");
	bindtextdomain ("tuxbox-enigma", "/share/locale");
	textdomain ("tuxbox-enigma");

	{
		eZap ezap(argc, argv);
		ezap.exec();
	}
	// system("/sbin/halt &");
}

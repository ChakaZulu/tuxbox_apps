#include <errno.h>
#include <time.h>
#include <malloc.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include <config.h>

#include <core/base/i18n.h>
#include <core/dvb/dvb.h>
#include <core/dvb/edvb.h>
#include <core/system/httpd.h>
#include <core/system/http_file.h>
#include <core/system/http_dyn.h>
#include <core/system/xmlrpc.h>
#include <core/system/econfig.h>
#include <core/system/init.h>
#include <core/gui/emessage.h>
#include <core/gui/actions.h>
#include <core/driver/rc.h>

#include "enigma.h"
#include "enigma_dyn.h"
#include "enigma_xmlrpc.h"
#include "enigma_main.h"
#include "sselect.h"

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

void eZap::keyDown(int code)
{
	if (focus)
		focus->event(eWidgetEvent(eWidgetEvent::keyDown, code));
	else if (main)
		main->event(eWidgetEvent(eWidgetEvent::keyDown, code));
}

void eZap::keyUp(int code)
{
	if (focus)
		focus->event(eWidgetEvent(eWidgetEvent::keyUp, code));
	else if (main)
		main->event(eWidgetEvent(eWidgetEvent::keyUp, code));
}

struct enigmaActions
{
	eActionMap map;
	eAction up, down, left, right;
	enigmaActions(): 
		map("global", "Global"),
		up(map, "selection_up", _("up")),
		down(map, "selection_down", _("down")),
		left(map, "selection_left", _("left")),
		right(map, "selection_right", _("right"))
	{
	}
};

eAutoInitP0<enigmaActions> enigmaActions(5, "enigma Actions");

void eZap::keyEvent(const eRCKey &key)
{
	int c = key.producer->getKeyCompatibleCode(key);
	
	const eAction *action=enigmaActions->map.findAction(key);
	if (action == &enigmaActions->up)
		eDebug("action: UP !!!");

	if (c != -1)
	{
		if (key.flags & eRCKey::flagBreak)
		{
			keyUp(c);
		}
		else
		{
			keyDown(c);
		}
	}
}

void eZap::status()
{
}

#include "gfbdc.h"

eZap::eZap(int argc, char **argv): eApplication(/*argc, argv, 0*/)
{
	int bootcount;
	int e;
	__u32 lastchannel;

	eZapLCD *pLCD;
	eHTTPD *httpd;
	eHTTPDynPathResolver *dyn_resolver;
	eHTTPFilePathResolver *fileresolver;

	instance = this;

	init = new eInit();
	init->setRunlevel(5);
	
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

//	connect(eRCInput::getInstance(), SIGNAL(keyEvent(const eRCKey&)), SLOT(keyEvent(const eRCKey&)));
	CONNECT(eRCInput::getInstance()->keyEvent, eZap::keyEvent);

	eDVB::getInstance()->configureNetwork();
	eDebug("<-- network");

	main = new eZapMain();

	pLCD = eZapLCD::getInstance();
	serviceSelector = new eServiceSelector();
	serviceSelector->setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
	eDebug("<-- service selector");

	dyn_resolver = new eHTTPDynPathResolver();
	ezapInitializeDyn(dyn_resolver);

	fileresolver = new eHTTPFilePathResolver();
	fileresolver->addTranslation(DATADIR "/enigma/htdocs", "/");
	fileresolver->addTranslation("/var/tuxbox/htdocs", "/www"); /* TODO: make user configurable */

	eDebug("[ENIGMA] starting httpd");
	httpd = new eHTTPD(80);
	ezapInitializeXMLRPC(httpd);
	httpd->addResolver(dyn_resolver);
	httpd->addResolver(fileresolver);

	qDebug("[ENIGMA] loading default keymaps...");
	
	eActionMapList::getInstance()->loadXML( DATADIR "/enigma/resources/rcdreambox.xml");

	eDebug("[ENIGMA] ok, beginning mainloop");

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

	if ((!(e = eConfig::getInstance()->getKey("/ezap/ui/lastChannel", lastchannel))) && (eDVB::getInstance()->getTransponders()))
	{
		eService *t = eDVB::getInstance()->getTransponders()->searchService(lastchannel >> 16, lastchannel & 0xFFFF);
		if (t)
		{
			eDVB::getInstance()->switchService(t);
		}
	}
	init->setRunlevel(10);
}

eZap::~eZap()
{
	if (eDVB::getInstance()->service)
	{
		eConfig::getInstance()->setKey("/ezap/ui/lastChannel", (__u32)((eDVB::getInstance()->original_network_id << 16) | eDVB::getInstance()->service_id));
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

#include <errno.h>
#include <time.h>
#include <malloc.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "qrect.h"
#include "enigma.h"
#include "elbwindow.h"
#include "elistbox.h"
#include "elabel.h"
#include "scan.h"
#include "sselect.h"
#include "dvb.h"
#include "edvb.h"
#include "eprogress.h"
#include "streaminfo.h"
#include "httpd.h"
#include "http_file.h"
#include "http_dyn.h"
#include "enigma_dyn.h"
#include "showbnversion.h"
#include "decoder.h"
#include "enigma_xmlrpc.h"
#include "enigma_main.h"
#include "emessage.h"
#include "epng.h"
#include "eavswitch.h"
#include "actions.h"
#include "rc.h"

#include "init.h"

#include <config.h>

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

static eActionMap map("global", "Global");

static eAction up(map, "hoch", "selection_up");

void eZap::keyEvent(const eRCKey &key)
{
	int c=key.getCompatibleCode();
	if (c!=-1)
	{
		if (key.getFlags()&eRCKey::flagBreak)
			keyUp(c);
		else
			keyDown(c);
	}
}

void eZap::status()
{
}

QString eZap::getVersion()
{
	return "enigma 0.1, compiled " __DATE__;
}

eZap::eZap(int argc, char **argv): QApplication(argc, argv, 0)
{
	instance=this;
	
	init=new eInit();
	
	init->setRunlevel(5);

	focus=0;
	main=0;

	connect(eRCInput::getInstance(), SIGNAL(keyEvent(const eRCKey&)), SLOT(keyEvent(const eRCKey&)));

	serviceSelector=new eServiceSelector;
	
	eDVB::getInstance()->configureNetwork();
	
	main=0;
	qDebug("<-- network");

	main=new eZapMain();

	eZapLCD* pLCD = eZapLCD::getInstance();
	serviceSelector=new eServiceSelector();
	serviceSelector->setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
	qDebug("<-- service selector");
	
	qDebug("[ENIGMA] starting httpd");
	eHTTPD *httpd=new eHTTPD(80);
	eHTTPDynPathResolver *dyn_resolver=new eHTTPDynPathResolver();
	ezapInitializeDyn(dyn_resolver);
	ezapInitializeXMLRPC(httpd);
	httpd->addResolver(dyn_resolver);

	eHTTPFilePathResolver *fileresolver=new eHTTPFilePathResolver();
	fileresolver->addTranslation(DATADIR "/enigma/htdocs", "/");
	fileresolver->addTranslation("/var/tuxbox/htdocs", "/www"); /* TODO: make user configurable */
	httpd->addResolver(fileresolver);

	qDebug("[ENIGMA] ok, beginning mainloop");
	__u32 lastchannel;

	int bootcount;
	if (eDVB::getInstance()->config.getKey("/elitedvb/system/bootCount", bootcount))
		bootcount=0;

	if (!bootcount)
	{
		eMessageBox msg("Willkommen zu enigma.\n\nBitte führen sie zunächst eine Kanalsuche durch, indem sie die d-Box-Taste drücken um in das "
			"Hauptmenü zu gelangen. Dort gibt es den Unterpunkt \"Transponder Scan\", der genau das macht, was sie glauben.\n", "enigma - erster Start");
		msg.show();
		msg.exec();
		msg.hide();
	}
	
	bootcount++;
	eDVB::getInstance()->config.setKey("/elitedvb/system/bootCount", bootcount);

	int e;
	if ((!(e=eDVB::getInstance()->config.getKey("/ezap/ui/lastChannel", lastchannel))) && (eDVB::getInstance()->getTransponders()))
	{
		eService *t=eDVB::getInstance()->getTransponders()->searchService(lastchannel>>16, lastchannel&0xFFFF);
		if (t)
			eDVB::getInstance()->switchService(t);
	}
}

eZap::~eZap()
{
	if (eDVB::getInstance()->service)
		eDVB::getInstance()->config.setKey("/ezap/ui/lastChannel", (__u32)((eDVB::getInstance()->original_network_id<<16)|eDVB::getInstance()->service_id));
	qDebug("[ENIGMA] beginning clean shutdown");
	qDebug("[ENIGMA] main");
	delete main;
	qDebug("[ENIGMA] serviceSelector");
	delete serviceSelector;
	qDebug("[ENIGMA] fertig");
	init->setRunlevel(-1);
	delete init;
	instance=0;
}

int main(int argc, char **argv)
{
	time_t t=0;
	stime(&t);
	fprintf(stderr, "%s", copyright);
	{
		eZap ezap(argc, argv);
		ezap.exec();
	}
	// system("/sbin/halt &");
}

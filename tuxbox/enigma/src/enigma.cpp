#include <errno.h>
#include <time.h>
#include <malloc.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <signal.h>
#include <dirent.h>

#include <lib/base/i18n.h>
#include <lib/driver/rc.h>
#include <lib/dvb/service.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/edvb.h>
#include <lib/gdi/gfbdc.h>
#include <lib/gdi/glcddc.h>
#include <lib/gui/emessage.h>
#include <lib/gui/actions.h>
#include <lib/system/econfig.h>
#include <lib/system/httpd.h>
#include <lib/system/http_file.h>
#include <lib/system/http_dyn.h>
#include <lib/system/init.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/actions.h>
#include <lib/driver/rc.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/decoder.h>

#include <lib/system/xmlrpc.h>
#include <enigma.h>
#include <enigma_dyn.h>
#include <enigma_xmlrpc.h>
#include <enigma_main.h>

// #include <mcheck.h>

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

eZap::eZap(int argc, char **argv)
	: eApplication(/*argc, argv, 0*/)
{
	int bootcount;

	eZapLCD *pLCD;
	eHTTPDynPathResolver *dyn_resolver;
	eHTTPFilePathResolver *fileresolver;

	instance = this;

	init = new eInit();
	init->setRunlevel(8);
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
	
	struct dirent **namelist;
	int n;

	n = scandir(DATADIR "/enigma/resources/", &namelist, 0, alphasort);
	if (n < 0)
		perror("scandir");
	else
	{
		while(n--)
		{
			std::string tmp_name(DATADIR "/enigma/resources/");
			tmp_name.append(namelist[n]->d_name);
			if (!strcmp(namelist[n]->d_name, ".."))
				break;
			eActionMapList::getInstance()->loadXML( tmp_name.c_str());
			free(namelist[n]);
		}
		free(namelist);
	}
	
	for (std::map<eString,eRCDevice*>::iterator i(eRCInput::getInstance()->getDevices().begin());
			i != eRCInput::getInstance()->getDevices().end(); ++i)
		eActionMapList::getInstance()->loadDevice(i->second);

	char *language=0;
	if (eConfig::getInstance()->getKey("/elitedvb/language", language))
		language=strdup("");
	setlocale(LC_ALL, language);
	free(language);

	eDVB::getInstance()->configureNetwork();
	eDebug("<-- network");

	// build Service Selector
	serviceSelector = new eServiceSelector();
	eDebug("<-- service selector");	

	main = new eZapMain();
	eDebug("<-- eZapMain");

	pLCD = eZapLCD::getInstance();
	eDebug("<-- pLCD");

	serviceSelector->setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
	eDebug("..");

	dyn_resolver = new eHTTPDynPathResolver();
	ezapInitializeDyn(dyn_resolver);

	fileresolver = new eHTTPFilePathResolver();
  fileresolver->addTranslation("/var/tuxbox/htdocs", "/www", 2); /* TODO: make user configurable */
	fileresolver->addTranslation(CONFIGDIR , "/config", 3);
	fileresolver->addTranslation("/", "/root", 3);
	fileresolver->addTranslation(DATADIR "/enigma/htdocs", "/", 2);

	eDebug("[ENIGMA] starting httpd");
	httpd = new eHTTPD(80, eApp);

	serialhttpd=0;
#if 0
  if ( atoi(eDVB::getInstance()->getInfo("mID").c_str()) > 4 )
  {
  	eDebug("[ENIGMA] starting httpd on serial port...");
    int fd=::open("/dev/tts/0", O_RDWR);
		if (fd < 0)
			eDebug("[ENIGMA] serial port error (%m)");
		else
		{
			struct termios tio;
			bzero(&tio, sizeof(tio));
			tio.c_cflag = B115200 /*| CRTSCTS*/ | CS8 | CLOCAL | CREAD;
			tio.c_iflag = IGNPAR;
			tio.c_oflag = 0;
			tio.c_lflag = 0;
			tio.c_cc[VTIME] = 0;
			tio.c_cc[VMIN] = 1;
			tcflush(fd, TCIFLUSH);
			tcsetattr(fd, TCSANOW, &tio); 

			char *banner="Welcome to the enigma serial access.\r\n"
					"you may start a HTTP session now.\r\n";
			write(fd, banner, strlen(banner));
			serialhttpd = new eHTTPConnection(fd, 0, httpd, 1);
		}
	}
#endif

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
		eMessageBox msg(_("Welcome to enigma.\n\n"
											"Please do a transponderscan first.\n(mainmenu > setup > channels > transponder scan)"),
										_("First start of enigma"),
										eMessageBox::btOK|eMessageBox::iconInfo, eMessageBox::btOK );
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
	eDebug("[ENIGMA] beginning clean shutdown");
	eDebug("[ENIGMA] main");
	delete main;
	eDebug("[ENIGMA] serviceSelector");
	delete serviceSelector;
	eDebug("[ENIGMA] fertig");
	init->setRunlevel(-1);

  if (serialhttpd)
    delete serialhttpd;
    
	delete httpd;
	delete init;
	instance = 0;
}

void fault(int x)
{
	printf(" ----- segfault :/\n");
	exit(2);
}

extern "C" void __mp_initsection();

int main(int argc, char **argv)
{
	time_t t=0;
	int res;
//	signal(SIGSEGV, fault);
//	printf("(secret data: %x)\n", __mp_initsection);

	stime(&t);
	eDebug("%s", copyright);

	setlocale (LC_ALL, "");
	bindtextdomain ("tuxbox-enigma", "/share/locale");
	bind_textdomain_codeset("tuxbox-enigma", "UTF8");
	textdomain ("tuxbox-enigma");
	
//	mtrace();
//	mcheck(0);
	
	{
		eZap ezap(argc, argv);
		res=ezap.exec();
	}
	exit(res);
//	mcheck_check_all();
//	muntrace();
	// system("/sbin/halt &");
}

extern "C" void mkstemps();
void mkstemps()
{
}

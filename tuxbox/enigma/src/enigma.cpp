#include <config.h>
#include <errno.h>
#include <time.h>
#include <malloc.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <signal.h>
#include <sys/klog.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dlfcn.h>

#include <lib/base/i18n.h>
#include <lib/driver/rc.h>
#include <lib/driver/eavswitch.h>
#include <lib/dvb/service.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/edvb.h>
#include <lib/gdi/gfbdc.h>
#include <lib/gdi/glcddc.h>
#include <lib/gui/emessage.h>
#include <lib/gui/actions.h>
#include <lib/system/nconfig.h>
#include <lib/system/httpd.h>
#include <lib/system/http_file.h>
#include <lib/system/http_dyn.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/system/info.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/actions.h>
#include <lib/driver/rc.h>
#include <lib/driver/eavswitch.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/serviceexternal.h>

#include <lib/system/xmlrpc.h>
#include <enigma.h>
#include <enigma_dyn.h>
#include <enigma_xmlrpc.h>
#include <enigma_main.h>
#include <timer.h>
#include <enigma_mount.h>
#include <enigma_plugins.h>

// #include <mcheck.h>

eWidget *currentFocus=0;

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
	if (currentFocus)
		currentFocus->event(eWidgetEvent(eWidgetEvent::evtKey, key));
	else if (main)
		main->event(eWidgetEvent(eWidgetEvent::evtKey, key));
}

void eZap::status()
{
}

void handle_sig_pipe (int i)
{
	eDebug("SIGPIPE");
	return;
}

void handle_sig_chld(int signum)
{
	pid_t cpid = 0;

	//make sure it is for sigchild
	if(signum != SIGCHLD)
		eDebug("error: signum = %d", signum);

	//wait for all children that have exited, returns 0 when no child is exited
	while( (cpid = waitpid(-1, NULL, WNOHANG)) > 0)
		eDebug("waited for child %d", cpid);

	//check for error
	if(cpid < 0)
		perror("waitpid");

	return;
}

extern void ezapInitializeWeb(eHTTPDynPathResolver *dyn_resolver);
// extern void ezapInitializeNetCMD(eHTTPD *httpd, eHTTPDynPathResolver *dyn_resolver);

eZap::eZap(int argc, char **argv)
	: eApplication(/*argc, argv, 0*/)
	,httpd(0), serialhttpd(0), dyn_resolver(0), fileresolver(0)
	,xmlrpcresolver(0), logresolver(0), serviceSelector(0), tts_fd(-1)
{
	init_eZap(argc, argv);
}

void eZap::init_eZap(int argc, char **argv)
{
	int bootcount;

#ifndef DISABLE_LCD
	eZapLCD *pLCD;
#endif

	instance = this;
	
	init = new eInit();

	FILE *pluginlist = fopen(CONFIGDIR "/enigma/plugins", "rb");
	if (pluginlist)
	{
		char filename[128];
		while (1)
		{
			if (!fgets(filename, 127, pluginlist))
				break;
			if (!filename)
				break;
			eString fname(filename);
			fname.removeChars('\n');
			void *handle=dlopen(fname.c_str(), RTLD_NOW);
			if (!handle)
				eWarning("[PLUGIN] load(%s) failed: %s", fname.c_str(), dlerror());
			else
				plugins.push_back(handle);
		}
		fclose(pluginlist);
	}

	{
		std::vector<eString> plugin_list;
		eZapPlugins::getAutostartPlugins(plugin_list);
		for (unsigned int i = 0; i < plugin_list.size(); i++)
		{
			void *handle = dlopen(plugin_list[i].c_str(), RTLD_NOW);
			if (!handle)
				eWarning("[PLUGIN] load(%s) failed: %s", plugin_list[i].c_str(), dlerror());
			else
				plugins.push_back(handle);
		}
	}
	init->setRunlevel(eAutoInitNumbers::configuration);
	Decoder::Initialize();

	char *language=0;
	if (eConfig::getInstance()->getKey("/elitedvb/language", language))
		language=strdup("");
	setlocale(LC_ALL, language);
	free(language);
	init->setRunlevel(eAutoInitNumbers::osd);

	if (eServiceHandlerExternal::getInstance())
	{
		std::vector<eZapPlugins::FileExtensionScriptInfo> plugin_list;
		eZapPlugins::getFileExtensionPlugins(plugin_list);
		for (unsigned int i = 0; i < plugin_list.size(); i++)
		{
			if (plugin_list[i].file_pattern.size())
			{
				eServiceHandlerExternal::getInstance()->addFileHandler(plugin_list[i].file_pattern, plugin_list[i].command, plugin_list[i].needfb, plugin_list[i].needrc, plugin_list[i].needlcd);
			}
			if (plugin_list[i].directory_pattern.size())
			{
				eServiceHandlerExternal::getInstance()->addDirectoryHandler(plugin_list[i].directory_pattern, plugin_list[i].command, plugin_list[i].needfb, plugin_list[i].needrc, plugin_list[i].needlcd);
			}
		}
	}

	CONNECT(eRCInput::getInstance()->keyEvent, eZap::keyEvent);

	desktop_fb=new eWidget();
	desktop_fb->setName("desktop_fb");
	desktop_fb->move(ePoint(0, 0));
	desktop_fb->resize(eSize(720, 576));
	desktop_fb->setTarget(gFBDC::getInstance());
	desktop_fb->makeRoot();
	desktop_fb->setBackgroundColor(gColor(0));
	desktop_fb->show();
#ifndef DISABLE_LCD
	desktop_lcd=new eWidget();
	desktop_lcd->setName("desktop_lcd");
	desktop_lcd->move(ePoint(0, 0));
	desktop_lcd->resize(eSize(128, 64));
	desktop_lcd->setTarget(gLCDDC::getInstance());
	desktop_lcd->setBackgroundColor(gColor(0));
	desktop_lcd->show();
#endif
 	eDebug("[ENIGMA] loading default keymaps...");

#if HAVE_DVB_API_VERSION < 3
#ifndef DISABLE_DREAMBOX_RC
	switch( eSystemInfo::getInstance()->getHwType() )
	{
		case eSystemInfo::DM600PVR:
		case eSystemInfo::DM7000:
		case eSystemInfo::DM7020:
			if ( eActionMapList::getInstance()->loadXML( CONFIGDIR "/enigma/resources/rcdm7000.xml") )
				if ( eActionMapList::getInstance()->loadXML( CONFIGDIR "/enigma/resources/rcdreambox2.xml") )
					if ( eActionMapList::getInstance()->loadXML( TUXBOXDATADIR "/enigma/resources/rcdm7000.xml") )
						if ( eActionMapList::getInstance()->loadXML( TUXBOXDATADIR "/enigma/resources/rcdreambox2.xml") )
							eFatal("couldn't load RC Mapping file for DM7000");
			break;
		case eSystemInfo::TR_DVB272S:
		case eSystemInfo::DM5600:
		case eSystemInfo::DM5620:
		case eSystemInfo::DM500:
		case eSystemInfo::DM500PLUS:
			if ( eActionMapList::getInstance()->loadXML( CONFIGDIR "/enigma/resources/rcdm5xxx.xml") )
				if ( eActionMapList::getInstance()->loadXML( TUXBOXDATADIR "/enigma/resources/rcdm5xxx.xml") )
					eFatal("couldn't load RC Mapping file for DM5XXX");
			break;
		default:
			break;
	}
#ifdef ENABLE_KEYBOARD
	if ( eSystemInfo::getInstance()->hasKeyboard() && eActionMapList::getInstance()->loadXML( CONFIGDIR "/enigma/resources/rcdreambox_keyboard.xml") )
		if ( eActionMapList::getInstance()->loadXML( TUXBOXDATADIR "/enigma/resources/rcdreambox_keyboard.xml") )
			eDebug("couldn't load Dreambox keyboard mapping (rcdreambox_keyboard.xml)");
#endif
#endif
#ifndef DISABLE_DBOX_RC
	if ( eActionMapList::getInstance()->loadXML( CONFIGDIR "/enigma/resources/rcdboxold.xml") )
		eActionMapList::getInstance()->loadXML( TUXBOXDATADIR "/enigma/resources/rcdboxold.xml");
	if ( eActionMapList::getInstance()->loadXML( CONFIGDIR "/enigma/resources/rcdboxnew.xml") )
		eActionMapList::getInstance()->loadXML( TUXBOXDATADIR "/enigma/resources/rcdboxnew.xml");
	if ( eActionMapList::getInstance()->loadXML( CONFIGDIR "/enigma/resources/rcdboxbuttons.xml") )
		eActionMapList::getInstance()->loadXML( TUXBOXDATADIR "/enigma/resources/rcdboxbuttons.xml");
#endif
#else
	if ( eActionMapList::getInstance()->loadXML( CONFIGDIR "/enigma/resources/rcdbox_inputdev.xml") )
		eActionMapList::getInstance()->loadXML( TUXBOXDATADIR "/enigma/resources/rcdbox_inputdev.xml");
	if ( eActionMapList::getInstance()->loadXML( CONFIGDIR "/enigma/resources/rcgeneric_inputdev.xml") )
		eActionMapList::getInstance()->loadXML( TUXBOXDATADIR "/enigma/resources/rcgeneric_inputdev.xml");
	if ( eActionMapList::getInstance()->loadXML( CONFIGDIR "/enigma/resources/rcdreambox_inputdev.xml") )
		eActionMapList::getInstance()->loadXML( TUXBOXDATADIR "/enigma/resources/rcdreambox_inputdev.xml");
#endif // HAVE_DVB_API_VERSION < 3

	for(std::map<eString,eRCDevice*>::iterator i(eRCInput::getInstance()->getDevices().begin());
		i != eRCInput::getInstance()->getDevices().end(); ++i)
			eActionMapList::getInstance()->loadDevice(i->second);

	eString::readEncodingFile();

	eDVB::getInstance()->configureNetwork();

	// build Service Selector
	serviceSelector = new eServiceSelector();

	main = new eZapMain();

#ifndef DISABLE_LCD
	pLCD = eZapLCD::getInstance();

	serviceSelector->setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
#endif

	reconfigureHTTPServer();
	
#ifdef ENABLE_EXPERT_WEBIF
	eMountMgr *mountMgr = new eMountMgr();
	mountMgr->automountMountPoints();
	delete mountMgr;
#ifndef DISABLE_FILE
	extern void initHDDparms(void);
	initHDDparms();

	int swapfile = 0;
	eConfig::getInstance()->getKey("/extras/swapfile", swapfile);
	char *swapfilename;
	if (eConfig::getInstance()->getKey("/extras/swapfilename", swapfilename))
		swapfilename = strdup("");
	extern void activateSwapFile(eString);
	if (swapfile == 1)
		activateSwapFile(eString(swapfilename));
	free(swapfilename);
#endif
#endif

	eDebug("[ENIGMA] ok, beginning mainloop");

	if (eConfig::getInstance()->getKey("/elitedvb/system/bootCount", bootcount))
	{
		bootcount = 1;
		if ( eSystemInfo::getInstance()->getHwType() == eSystemInfo::TR_DVB272S )
			eConfig::getInstance()->setKey("/ezap/osd/simpleMainMenu", 1);
#if 0
		eMessageBox msg(_("Welcome to enigma.\n\n"
											"Please do a transponder scan first.\n(mainmenu > setup > channels > transponder scan)"),
										_("First start of enigma"),
										eMessageBox::btOK|eMessageBox::iconInfo, eMessageBox::btOK );
		msg.show();
		msg.exec();
		msg.hide();
#endif
	}
	else
		bootcount++;

	eConfig::getInstance()->setKey("/elitedvb/system/bootCount", bootcount);

	if ( bootcount == 1 )
	{
//		eConfig::getInstance()->setKey("/ezap/rc/style", "classic");
//		eConfig::getInstance()->setKey("/ezap/rc/sselect_style", "sselect_classic" );
		eConfig::getInstance()->setKey("/ezap/serviceselector/showButtons", 1 );
		serviceSelector->setStyle( serviceSelector->getStyle(), true );
	}

	if(signal(SIGPIPE, handle_sig_pipe) == SIG_ERR)
		eFatal("couldn't install SIGPIPE handler");

	/*
	 * Don't install a SIGCHLD handler, our handler will call 'waitpid' on all children,
	 * causing a race with users that call waitpid on their own child processes (e.g. 'system()' calls)
	 */
	/*
	if(signal(SIGCHLD, handle_sig_chld) == SIG_ERR)
		eFatal("couldn't install SIGCHLD handler");
	*/

	init->setRunlevel(eAutoInitNumbers::main);
}

void eZap::reconfigureHTTPServer()
{
	delete serialhttpd;
	delete httpd;
	serialhttpd=0;
	httpd=0;

	dyn_resolver = new eHTTPDynPathResolver();
	ezapInitializeDyn(dyn_resolver);
	ezapInitializeWeb(dyn_resolver);

	fileresolver = new eHTTPFilePathResolver();
	fileresolver->addTranslation("/var/tuxbox/htdocs", "/www", 2); /* TODO: make user configurable */
	fileresolver->addTranslation(CONFIGDIR , "/config", 3);
	fileresolver->addTranslation("/", "/root", 3);
	fileresolver->addTranslation(TUXBOXDATADIR "/enigma/htdocs", "/", 2);

	logresolver = new eHTTPLogResolver();

#ifndef DISABLE_NETWORK
	xmlrpcresolver = new eHTTPXMLRPCResolver();
	ezapInitializeXMLRPC();
#endif

	int port=80;
	eConfig::getInstance()->getKey("/elitedvb/network/webifport", port);
	eDebug("[ENIGMA] starting httpd");
	httpd = new eHTTPD(port, eApp);
	httpd->addResolver(xmlrpcresolver);
	httpd->addResolver(logresolver);
	httpd->addResolver(dyn_resolver);
	httpd->addResolver(fileresolver);

	bool SerialConsoleActivated=false;
	FILE *f=fopen("/proc/cmdline", "rt");
	if (f)
	{
		char *cmdline=NULL;
		size_t len = 0;
		getline( &cmdline, &len, f );
		SerialConsoleActivated = strstr( cmdline, "console=ttyS0" ) != NULL;
		fclose(f);
		free(cmdline);
		if ( SerialConsoleActivated )
			eDebug("console=ttyS0 detected...disable enigma serial http interface");
		else
			eDebug("activate enigma serial http interface");
	}

	if ( tts_fd != -1)
	{
		::close(tts_fd);
		tts_fd=-1;
	}

#if 1
	logOutputConsole=1;
	int disableSerialDebugOutput=eSystemInfo::getInstance()->hasNetwork();
	eConfig::getInstance()->getKey("/ezap/extra/disableSerialOutput", disableSerialDebugOutput);
	eConfig::getInstance()->setKey("/ezap/extra/disableSerialOutput", disableSerialDebugOutput);
	if ( !SerialConsoleActivated && !disableSerialDebugOutput )
	{
		eDebug("[ENIGMA] starting httpd on serial port...");
		tts_fd=::open("/dev/tts/0", O_RDWR);

		if (tts_fd < 0)
			eDebug("[ENIGMA] serial port error (%m)");
		else
		{
			struct termios tio;
			memset(&tio, 0, sizeof(tio));
			tio.c_cflag = B115200 /*| CRTSCTS*/ | CS8 | CLOCAL | CREAD;
			tio.c_iflag = IGNPAR;
			tio.c_oflag = 0;
			tio.c_lflag = 0;
			tio.c_cc[VTIME] = 0;
			tio.c_cc[VMIN] = 1;
			tcflush(tts_fd, TCIFLUSH);
			tcsetattr(tts_fd, TCSANOW, &tio);

			logOutputConsole=0; // disable enigma logging to console
			klogctl(8, 0, 1); // disable kernel log to serial

			char *banner="Welcome to the enigma serial access.\r\n"
					"you may start a HTTP session now if you send a \"break \".\r\n";
			write(tts_fd, banner, strlen(banner));
			serialhttpd = new eHTTPConnection(tts_fd, 0, httpd, 1);
//			char *i="GET /version HTTP/1.0\n\n";
//			char *i="GET /menu.cr HTTP/1.0\n\n";
			char *i="GET /log/debug HTTP/1.0\n\n";
			serialhttpd->inject(i, strlen(i));
		}
	}
#endif
}

eZap::~eZap()
{
	eDebug("[ENIGMA] beginning clean shutdown");
	eDebug("[ENIGMA] main");
	delete main;
	eDebug("[ENIGMA] serviceSelector");
	delete serviceSelector;
#ifdef ENABLE_EXPERT_WEBIF
	eDebug("[ENIGMA] unmountAllMountPoints");
	eMountMgr *mountMgr = new eMountMgr();
	mountMgr->unmountAllMountPoints();
	delete mountMgr;
#endif
	eDebug("[ENIGMA] fertig");
	init->setRunlevel(-1);

	for (std::list<void*>::iterator i(plugins.begin()); i != plugins.end(); ++i)
		dlclose(*i);

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
#ifdef MEMLEAK_CHECK
	atexit(DumpUnfreed);
#endif
	int res;
//	signal(SIGSEGV, fault);
//	printf("(secret data: %x)\n", __mp_initsection);

	eDebug("%s", copyright);

	setlocale (LC_ALL, "");
	bindtextdomain ("tuxbox-enigma", LOCALEDIR);
	bind_textdomain_codeset("tuxbox-enigma", "UTF8");
	textdomain ("tuxbox-enigma");

//	mtrace();
//	mcheck(0);

	{
		eConfigOld c;
		c.convert("/var/tuxbox/config/enigma/config");
	}

	{
		eZap ezap(argc, argv);
		if ( !ezap.isAppQuitNowSet() )
			res=ezap.exec();
		else
			res=2;  // restart... (timezone changed)

		if (res == 4)  // when reboot is requeste set no deepstandbywakeup timer
			eTimerManager::getInstance()->disableDeepstandbyWakeup();

		if ( !res )  // only when shutdown disable pin8 voltage
		{
			if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM600PVR ||
				eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM500PLUS)
			{
				__u8 data[720*576];
				gPixmap pixmap;
				pixmap.x=720;
				pixmap.y=576;
				pixmap.bpp=8;
				pixmap.bypp=1;
				pixmap.stride=720;
				pixmap.data=data;
				pixmap.clut.colors=256;
				pixmap.clut.data=gFBDC::getInstance()->getPixmap().clut.data;
				gPixmapDC outputDC(&pixmap);
				eWidget virtualRoot;
				virtualRoot.move(ePoint(0, 0));
				virtualRoot.resize(eSize(720, 576));
				virtualRoot.setTarget(&outputDC);
				virtualRoot.makeRoot();
				virtualRoot.setBackgroundColor(gColor(0));
				virtualRoot.show();
				{
					eMessageBox mb(
						_("It's now safe to unplug power!"),
						_("Shutdown finished"), eMessageBox::iconInfo);
					mb.show();
					{
						while(gRC::getInstance().mustDraw())
							usleep(1000);
						int fd = open("/tmp/shutdown.raw", O_CREAT|O_WRONLY|O_TRUNC);
						if ( fd >= 0 )
						{
							write(fd, pixmap.data, 720*576*pixmap.bpp/8);
							::close(fd);
						}
						struct fb_cmap* cmap = fbClass::getInstance()->CMAP();
						fd = open("/tmp/cmap", O_WRONLY|O_CREAT|O_TRUNC);
						if ( fd >= 0 )
						{
							write(fd, &cmap->start, sizeof(cmap->start));
							write(fd, &cmap->len, sizeof(cmap->len));
							write(fd, cmap->red, cmap->len*sizeof(__u16));
							write(fd, cmap->green, cmap->len*sizeof(__u16));
							write(fd, cmap->blue, cmap->len*sizeof(__u16));
							write(fd, cmap->transp, cmap->len*sizeof(__u16));
							::close(fd);
						}
					}
				}
				eZap::getInstance()->getDesktop(eZap::desktopFB)->makeRoot();
				eMessageBox mb(
					_("Please wait... your Dreambox is shutting down!"),
					_("Shutdown..."), eMessageBox::iconInfo);
				mb.show();
				fbClass::getInstance()->lock();
				fbClass::getInstance()->setAvailable(0);
				pixmap.clut.data=0;
			}
			else
				eAVSwitch::getInstance()->setActive(0);
		}
	}

	Decoder::Flush();
	//signal(SIGCHLD, SIG_DFL);
	exit(res);
//	mcheck_check_all();
//	muntrace();
}

extern "C" void mkstemps();
void mkstemps()
{
}

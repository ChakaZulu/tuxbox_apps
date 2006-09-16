//=============================================================================
// YHTTPD
// Main Program
//=============================================================================

// system
#include <csignal>
#include <unistd.h>

// yhttpd
#include "yconfig.h"
#include "ylogging.h"
#include "yhook.h"

#ifdef Y_CONFIG_USE_YPARSER	
#include "mod_yparser.h"
static CyParser yParser;
#endif

//-----------------------------------------------------------------------------
// Setting yhttpd Instance
//-----------------------------------------------------------------------------
#include "yhttpd.h"
static Cyhttpd *yhttpd = NULL;
CStringList Cyhttpd::ConfigList;
//=============================================================================
// HOOKS: Definition & Instance for Hooks, attach/detach Hooks
//=============================================================================

#ifdef Y_CONFIG_USE_AUTHHOOK
#include "mod_auth.h"
static CmAuth *auth = NULL;
#endif

#ifdef Y_CONFIG_USE_TESTHOOK
#include "mod_testhook.h"
static CTesthook *testhook = NULL;
#endif

//-----------------------------------------------------------------------------
#ifdef CONFIG_SYSTEM_TUXBOX
#include "neutrinoapi.h"
static CNeutrinoAPI *NeutrinoAPI;
#endif

//=============================================================================
// Main: Main Entry, Command line passing, Webserver Instance creation & Loop
//=============================================================================
//-----------------------------------------------------------------------------
// Signal Handling
//-----------------------------------------------------------------------------
static void sig_catch(int msignal)
{
	switch (msignal) {
	case SIGPIPE:
		aprintf("got signal PIPE, nice!\n");
		break;
	case SIGHUP:
		aprintf("got signal HUP, reading config\n");
		if (yhttpd)
			yhttpd->ReadConfig();
		break;
	default:
		yhttpd->stop_webserver();
		delete yhttpd;
		exit(EXIT_SUCCESS); //FIXME: return to main() some way...
	}
}

//-----------------------------------------------------------------------------
// Main Entry
//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
	bool do_fork = true;
	yhttpd = new Cyhttpd();
	if(!yhttpd)
	{
		aprintf("Error initializing WebServer\n");
		return EXIT_FAILURE;
	}
	for (int i = 1; i < argc; i++)
	{
		if ((!strncmp(argv[i], "-d", 2)) || (!strncmp(argv[i], "--debug", 7)))
		{
			CLogging::getInstance()->setDebug(true);
			do_fork = false;
		}
		else if ((!strncmp(argv[i], "-f", 2)) || (!strncmp(argv[i], "--fork", 6)))
		{
			do_fork = false;
		}
		else if ((!strncmp(argv[i], "-h", 2)) || (!strncmp(argv[i], "--help", 6)))
		{
			yhttpd->usage(stdout);
			return EXIT_SUCCESS;
		}
		else if ((!strncmp(argv[i], "-v", 2)) || (!strncmp(argv[i],"--version", 9)))
		{
			yhttpd->version(stdout);
			return EXIT_SUCCESS;
		}
		else if ((!strncmp(argv[i], "-t", 2)) || (!strncmp(argv[i],"--thread-off", 12)))
		{
			yhttpd->flag_threading_off = true;
		}
		else if ((!strncmp(argv[i], "-l", 2)) )
		{
			if(argv[i][2] >= '0' && argv[i][2] <= '9')
				CLogging::getInstance()->LogLevel = (argv[i][2]-'0');
		}
		else
		{
			yhttpd->usage(stderr);
			return EXIT_FAILURE;
		}
	}
	yhttpd->flag_threading_off = true; //FIXME: Problem with multplexing socket to Threads
	// setup signal catching (subscribing)
	signal(SIGPIPE, sig_catch);
	signal(SIGINT, sig_catch);
	signal(SIGHUP, sig_catch);
	signal(SIGTERM, sig_catch);

	// Start Webserver: fork ist if not in debug mode
	aprintf("Webserver (%s) Version %s\n", HTTPD_NAME, HTTPD_VERSION);
	aprintf("Webserver starting...\n");
	if (do_fork)
	{
		log_level_printf(9,"do fork\n");
		switch (fork()) {
		case -1:
			dperror("fork");
			return -1;
		case 0:
			break;
		default:
			return EXIT_SUCCESS;
		}

		if (setsid() == -1)
		{
			dperror("Error setsid");
			return EXIT_FAILURE;
		}
	}
	dprintf("Start in Debug-Mode\n"); // non forked debugging loop
	
	yhttpd->run();
	delete yhttpd;
	
	aprintf("Main end\n");
	return EXIT_SUCCESS;
}

//=============================================================================
// Class yhttpd
//=============================================================================
Cyhttpd::Cyhttpd()
{
	webserver = new CWebserver();
	flag_threading_off = false;
}
//-----------------------------------------------------------------------------
Cyhttpd::~Cyhttpd()
{
	if(webserver)
		delete webserver;
	webserver = NULL;
}

//-----------------------------------------------------------------------------
void Cyhttpd::run()
{
	if(webserver)
	{
		hooks_attach();
		ReadConfig();
		if(flag_threading_off)
			webserver->is_threading = false;
		webserver->run();
		stop_webserver();
	}
	else
		aprintf("Error initializing WebServer\n");
}
		
//-----------------------------------------------------------------------------
// Show Version Text and Number
//-----------------------------------------------------------------------------
void Cyhttpd::version(FILE *dest)
{
	fprintf(dest, "%s - Webserver v%s\n", HTTPD_NAME, HTTPD_VERSION);
}

//-----------------------------------------------------------------------------
// Show Usage
//-----------------------------------------------------------------------------
void Cyhttpd::usage(FILE *dest)
{
	version(dest);
	fprintf(dest, "command line parameters:\n");
	fprintf(dest, "-d, --debug    enable debugging code (implies -f)\n");
	fprintf(dest, "-f, --fork     do not fork\n");
	fprintf(dest, "-h, --help     display this text and exit\n\n");
	fprintf(dest, "-v, --version  display version and exit\n");
	fprintf(dest, "-l<loglevel>,  set loglevel (0 .. 9)\n");
	fprintf(dest, "-t, --thread-off  set threading off\n");
}

//-----------------------------------------------------------------------------
// Stop WebServer 
//-----------------------------------------------------------------------------
void Cyhttpd::stop_webserver()
{
	aprintf("stop requested......\n");
	if (webserver) {
		webserver->stop();
		hooks_detach();
	}
}
//-----------------------------------------------------------------------------
// Attach hooks (use hook order carefully)
//-----------------------------------------------------------------------------
void Cyhttpd::hooks_attach()
{
#ifdef Y_CONFIG_USE_AUTHHOOK
	// First Check Authentication
	auth = new CmAuth();
	CyhookHandler::attach(auth);
#endif

#ifdef Y_CONFIG_USE_TESTHOOK
	testhook = new CTesthook();
	CyhookHandler::attach(testhook);
#endif

#ifdef CONFIG_SYSTEM_TUXBOX
	NeutrinoAPI = new CNeutrinoAPI();
	CyhookHandler::attach(NeutrinoAPI->NeutrinoYParser);
	CyhookHandler::attach(NeutrinoAPI->ControlAPI);
#else
#ifdef Y_CONFIG_USE_YPARSER	
	CyhookHandler::attach(&yParser);
#endif
#endif
}

//-----------------------------------------------------------------------------
// Detach hooks & Destroy 
//-----------------------------------------------------------------------------
void Cyhttpd::hooks_detach()
{
#ifdef Y_CONFIG_USE_AUTHHOOK
	CyhookHandler::detach(auth);
	delete auth;
#endif

#ifdef Y_CONFIG_USE_TESTHOOK
	CyhookHandler::detach(testhook);
	delete testhook;
#endif

#ifdef CONFIG_SYSTEM_TUXBOX
	CyhookHandler::detach(NeutrinoAPI->NeutrinoYParser);
#else
#ifdef Y_CONFIG_USE_YPARSER	
	CyhookHandler::detach(&yParser);
#endif
#endif
}

//-----------------------------------------------------------------------------
// Read Webserver Configurationfile
// Call "Hooks_ReadConfig" so Hooks can read/write own Configuration Values
//-----------------------------------------------------------------------------
void Cyhttpd::ReadConfig(void)
{
	log_level_printf(3,"ReadConfig Start\n");	
	CConfigFile *Config = new CConfigFile(',');

	Config->loadConfig(HTTPD_CONFIGFILE);
	
	// configure debugging & logging
	CLogging::getInstance()->Verbose 	= Config->getBool("VERBOSE", false);
	CLogging::getInstance()->Log 		= Config->getBool("LOG", false);
	if(CLogging::getInstance()->LogLevel == 0)
		CLogging::getInstance()->LogLevel = Config->getInt32("LogLevel", 0);

	// get variables
	webserver->init(Config->getInt32("Port", HTTPD_STANDARD_PORT), Config->getBool("THREADS", true));
	ConfigList["PrivatDocumentRoot"]= Config->getString("PrivatDocRoot", PRIVATEDOCUMENTROOT);
	ConfigList["PublicDocumentRoot"]= Config->getString("PublicDocRoot", PUBLICDOCUMENTROOT);
	ConfigList["HostedDocumentRoot"]= Config->getString("HostedDocRoot", HOSTEDDOCUMENTROOT);
#ifdef Y_CONFIG_USE_OPEN_SSL
	ConfigList["SSL"]		= Config->getString("SSL", "false");
	ConfigList["SSL_pemfile"]	= Config->getString("SSL_pemfile", SSL_PEMFILE);
	ConfigList["SSL_CA_file"]	= Config->getString("SSL_CA_file", SSL_CA_FILE);
	
	CySocket::SSL_pemfile 		= ConfigList["SSL_pemfile"];
	CySocket::SSL_CA_file 		= ConfigList["SSL_CA_file"];
	if(ConfigList["SSL"] == "true")
		CySocket::initSSL();
#endif
	// Read App specifig settings by Hook
	CyhookHandler::Hooks_ReadConfig(Config, ConfigList);
	
	// Save if new defaults are set
	if (Config->getUnknownKeyQueryedFlag() == true)
		Config->saveConfig(HTTPD_CONFIGFILE);
	log_level_printf(3,"ReadConfig End\n");	
	delete Config;
}

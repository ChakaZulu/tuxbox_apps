//=============================================================================
// YHTTPD
// Logging & Debugging
//=============================================================================

// c
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

// yhttpd
#include "yconfig.h"
#include "ytypes_globals.h"
#include "ylogging.h"
#include "yconnection.h"
//=============================================================================
// Instance Handling - like Singelton Pattern
//=============================================================================
//-----------------------------------------------------------------------------
// Init as Singelton
//-----------------------------------------------------------------------------
CLogging *CLogging::instance = NULL;

//-----------------------------------------------------------------------------
// There is only one Instance
//-----------------------------------------------------------------------------
CLogging *CLogging::getInstance(void)
{
	if (!instance)
		instance = new CLogging();
	return instance;
}

//-----------------------------------------------------------------------------
void CLogging::deleteInstance(void)
{
	if (instance)
		delete instance;
	instance = NULL;
}

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CLogging::CLogging(void)
{
	Debug 		= false;
	LogToFile 	= false;
	LogLevel	= 0;
	Logfile 	= NULL;
	Log		= false;
	Verbose 	= false;
	pthread_mutex_init(&Log_mutex, NULL);
}

//-----------------------------------------------------------------------------
CLogging::~CLogging(void)
{
}
//=============================================================================

//-----------------------------------------------------------------------------
void CLogging::setDebug(bool _debug)
{
		Debug = _debug;
}
//-----------------------------------------------------------------------------
bool CLogging::getDebug(void)
{
		return Debug;
}
//=============================================================================
// Logging Calls
// use mutex controlled calls to output resources
// Normal Logging to Stdout, if "Log" is true then Log to file
//=============================================================================
#define bufferlen 1024*8
//-----------------------------------------------------------------------------
void CLogging::printf ( const char *fmt, ... )
{
	char buffer[bufferlen];

	pthread_mutex_lock( &Log_mutex );

	va_list arglist;
	va_start( arglist, fmt );
	if(arglist)
		vsnprintf( buffer, bufferlen, fmt, arglist );
	va_end(arglist);

	::printf(buffer);
	if(LogToFile)
		;	//FIXME Logging to File
	pthread_mutex_unlock( &Log_mutex );
}
//-----------------------------------------------------------------------------
// Log WebAccess to Logfile
//-----------------------------------------------------------------------------
/* TODO: move to mod
void CLogging::LogRequest(CWebserverConnection *con)
{
	char buffer[bufferlen];
	
	if ((Log) || (Verbose))
	{
		if ((Log) && (!Logfile))
			Logfile = fopen("/tmp/httpd_log","a");

		pthread_mutex_lock(&Log_mutex);
		std::string method;

		switch (con->Method) {
		case M_GET:
			method = "GET";
			break;
		case M_POST:
			method = "POST";
			break;
		case M_HEAD:
			method = "HEAD";
			break;
		default:
			method = "unknown";
			break;
		}

		struct tm *time_now;
		time_t now = time(NULL);
		char zeit[80];

		time_now = localtime(&now);
		strftime(zeit, 80, "[%d/%b/%Y:%H:%M:%S]", time_now);
		::snprintf(buffer,bufferlen,"%s %s %s %d %s %s\n",
			con->Request.UrlData["clientaddr"].c_str(),
			zeit,
			method.c_str(),
			con->HttpStatus,
			con->Request.UrlData["url"].c_str(),
			//Request->ContentType.c_str(),
			con->Request.UrlData["paramstring"].c_str());
		if ((Log) && (Logfile))
			::fprintf(Logfile, "%s", buffer);
		if (Verbose)
			::printf("%s",buffer);
		pthread_mutex_unlock(&Log_mutex);
	}
}
*/


#include "debug.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


//-------------------------------------------------------------------------

CDEBUG* CDEBUG::getInstance()
{
	static CDEBUG* cdebug = NULL;
	if(cdebug == NULL)
	{
		cdebug = new CDEBUG();
	}
	return cdebug;
}
//-------------------------------------------------------------------------

CDEBUG::CDEBUG()
{
	Debug = false;
	Log = false;
	Verbose = false;
	Logfile = 0;
	buffer = new char[1024*4];
	pthread_mutex_init( &Log_mutex, NULL );
}
//-------------------------------------------------------------------------

CDEBUG::~CDEBUG()
{
	if(Logfile > 0)
	{
		fclose(Logfile);
		Logfile = 0;
	}
	delete[] buffer;
}
//-------------------------------------------------------------------------

void CDEBUG::LogRequest(CWebserverRequest *Request)
{
	if(Log || Verbose)
	{
		if(Log && Logfile == 0)
			Logfile = fopen("/tmp/httpd_log","a");
		pthread_mutex_lock( &Log_mutex );
		string method;
		switch(Request->Method)
		{
			case M_GET :	method = "GET";
				break;
			case M_POST	:	method = "POST";
				break;
			case M_HEAD :	method = "HEAD";
				break;
			default :
				method = "unknown";
		}
		struct tm *time_now;
		time_t now = time(NULL);
		char zeit[80];

		time_now = localtime(&now);
		strftime(zeit, 80, "[%d/%b/%Y:%H:%M:%S]", time_now);


		::sprintf(buffer,"%s %s %s %d %s %s %s\n",
			Request->Client_Addr.c_str(),
				zeit,
					method.c_str(),
						Request->HttpStatus,
							Request->URL.c_str(),
								Request->ContentType.c_str(),
									Request->Param_String.c_str());

		if(Log && Logfile > 0)
			::fprintf(Logfile,"%s",buffer);
		if(Verbose)
			::printf("%s",buffer);

		pthread_mutex_unlock( &Log_mutex );
	}
}

void CDEBUG::debugprintf ( const char *fmt, ... )
{
	if(Debug)
	{
		pthread_mutex_lock( &Log_mutex );

		va_list arglist;
		va_start( arglist, fmt );
		vsprintf( buffer, fmt, arglist );
		va_end(arglist);

		::printf(buffer);

		pthread_mutex_unlock( &Log_mutex );
	}
}
//-------------------------------------------------------------------------

void CDEBUG::logprintf ( const char *fmt, ... )
{
	if(Debug)
	{
		pthread_mutex_lock( &Log_mutex );

		va_list arglist;
		va_start( arglist, fmt );
		vsprintf( buffer, fmt, arglist );
		va_end(arglist);

		::printf(buffer);

		pthread_mutex_unlock( &Log_mutex );
	}
}
//-------------------------------------------------------------------------

void CDEBUG::printf ( const char *fmt, ... )
{
	pthread_mutex_lock( &Log_mutex );

	va_list arglist;
	va_start( arglist, fmt );
	if(arglist)
		vsprintf( buffer, fmt, arglist );
	va_end(arglist);

	::printf(buffer);

	pthread_mutex_unlock( &Log_mutex );
}
//-------------------------------------------------------------------------


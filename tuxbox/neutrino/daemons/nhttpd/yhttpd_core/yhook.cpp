//=============================================================================
// YHTTPD
// Hook and HookHandler
//=============================================================================
// C
#include <cstdarg>
#include <cstdio>

// yhttpd
#include "yhook.h"
#include "ylogging.h"

//=============================================================================
// Initialization of static variables
//=============================================================================
THookList CyhookHandler::HookList;

//=============================================================================
// Hook Handling 
//=============================================================================
//-----------------------------------------------------------------------------
// Hook Dispatcher for Session Hooks
// Execute every Hook in HookList until State change != HANDLED_NONE
//-----------------------------------------------------------------------------
THandleStatus CyhookHandler::Hooks_SendResponse()
{
	log_level_printf(4,"Response Hook-List Start\n");
	THandleStatus _status = HANDLED_NONE;
	THookList::iterator i = HookList.begin();
	for ( ; i!= HookList.end(); i++ )
	{
		log_level_printf(4,"Response Hook-List (%s) Start\n", ((*i)->getHookName()).c_str());
		// response Hook
		_status = (*i)->Hook_SendResponse(this);
		log_level_printf(4,"Response Hook-List (%s) End. Status (%d)\n", ((*i)->getHookName()).c_str(), status);
		if((_status != HANDLED_NONE) && (_status != HANDLED_CONTINUE))
			break;
	}
	log_level_printf(4,"Response Hook-List End\n");
	log_level_printf(8,"Response Hook-List Result:\n%s\n", yresult.c_str());
	status = _status;
	return _status;
}	
//-----------------------------------------------------------------------------
// Hook Dispatcher for Server based Hooks
// Execute every Hook in HookList until State change != HANDLED_NONE and
// != HANDLED_CONTINUE
//-----------------------------------------------------------------------------
THandleStatus CyhookHandler::Hooks_ReadConfig(CConfigFile *Config, CStringList &ConfigList)
{
	log_level_printf(4,"ReadConfig Hook-List Start\n");
	THandleStatus _status = HANDLED_NONE;
	THookList::iterator i = HookList.begin();
	for ( ; i!= HookList.end(); i++ )
	{
//		log_level_printf(4,"ReadConfig Hook-List (%s)  Start\n", ((*i)->getHookName()).c_str());
		// response Hook
		_status = (*i)->Hook_ReadConfig(Config, ConfigList);
		log_level_printf(4,"ReadConfig Hook-List (%s) Status (%d)\n", ((*i)->getHookName()).c_str(), _status);
		if((_status != HANDLED_NONE) && (_status != HANDLED_CONTINUE))
			break;
	}
	log_level_printf(4,"ReadConfig Hook-List End\n");
	return _status;
}	
//-----------------------------------------------------------------------------
// Hook Dispatcher for EndConnection
//-----------------------------------------------------------------------------
THandleStatus CyhookHandler::Hooks_EndConnection()
{
	log_level_printf(4,"EndConnection Hook-List Start\n");
	THandleStatus _status = HANDLED_NONE;
	THookList::iterator i = HookList.begin();
	for ( ; i!= HookList.end(); i++ )
	{
		log_level_printf(4,"EndConnection Hook-List (%s) Start\n", ((*i)->getHookName()).c_str());
		// response Hook
		_status = (*i)->Hook_EndConnection(this);
		log_level_printf(4,"EndConnection Hook-List (%s) End. Status (%d)\n", ((*i)->getHookName()).c_str(), _status);
		if((_status != HANDLED_NONE) && (_status != HANDLED_CONTINUE))
			break;
	}
	log_level_printf(4,"EndConnection Hook-List End\n");
	status = _status;
	return _status;
}
//-----------------------------------------------------------------------------
// Hook Dispatcher for UploadSetFilename
//-----------------------------------------------------------------------------
THandleStatus CyhookHandler::Hooks_UploadSetFilename(std::string &Filename)
{
	log_level_printf(4,"UploadSetFilename Hook-List Start. Filename:(%s)\n", Filename.c_str());
	THandleStatus _status = HANDLED_NONE;
	THookList::iterator i = HookList.begin();
	for ( ; i!= HookList.end(); i++ )
	{
		log_level_printf(4,"UploadSetFilename Hook-List (%s) Start\n", ((*i)->getHookName()).c_str());
		// response Hook
		_status = (*i)->Hook_UploadSetFilename(this, Filename);
		log_level_printf(4,"UploadSetFilename Hook-List (%s) End. Status (%d)\n", ((*i)->getHookName()).c_str(), _status);
		if((_status != HANDLED_NONE) && (_status != HANDLED_CONTINUE))
			break;
	}
	log_level_printf(4,"UploadSetFilename Hook-List End\n");
	status = _status;
	return _status;
}
//-----------------------------------------------------------------------------
// Hook Dispatcher for UploadSetFilename
//-----------------------------------------------------------------------------
THandleStatus CyhookHandler::Hooks_UploadReady(std::string Filename)
{
	log_level_printf(4,"UploadReady Hook-List Start. Filename:(%s)\n", Filename.c_str());
	THandleStatus _status = HANDLED_NONE;
	THookList::iterator i = HookList.begin();
	for ( ; i!= HookList.end(); i++ )
	{
		log_level_printf(4,"UploadReady Hook-List (%s) Start\n", ((*i)->getHookName()).c_str());
		// response Hook
		_status = (*i)->Hook_UploadReady(this, Filename);
		log_level_printf(4,"UploadReady Hook-List (%s) End. Status (%d)\n", ((*i)->getHookName()).c_str(), _status);
		if((_status != HANDLED_NONE) && (_status != HANDLED_CONTINUE))
			break;
	}
	log_level_printf(4,"UploadReady Hook-List End\n");
	status = _status;
	return _status;
}

//=============================================================================
// Output helpers
//=============================================================================
//-----------------------------------------------------------------------------
void CyhookHandler::SendHTMLHeader(std::string Titel)
{
	WriteLn("<html>\n<head><title>" + Titel + "</title>\n");
	WriteLn("<meta http-equiv=\"cache-control\" content=\"no-cache\" />");
	WriteLn("<meta http-equiv=\"expires\" content=\"0\" />\n</head>\n<body>\n");
}
//-----------------------------------------------------------------------------
void CyhookHandler::SendHTMLFooter(void)
{
	WriteLn("</body>\n</html>\n\n");
}

//-----------------------------------------------------------------------------
#define OUTBUFSIZE 4096
void CyhookHandler::printf ( const char *fmt, ... )
{
	char outbuf[OUTBUFSIZE];
	bzero(outbuf,OUTBUFSIZE);
	va_list arglist;
	va_start( arglist, fmt );
	vsnprintf( outbuf,OUTBUFSIZE, fmt, arglist );
	va_end(arglist);
	Write(outbuf);
}

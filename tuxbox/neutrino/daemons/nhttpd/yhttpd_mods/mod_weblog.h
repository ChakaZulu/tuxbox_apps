//=============================================================================
// YHTTPD
// mod_weblog : Logging of HTTPD-Requests/Responses
//=============================================================================
#ifndef __yhttpd_mod_weblog_h__
#define __yhttpd_mod_weblog_h__

#include "yconfig.h"
#include "yhook.h"
//-----------------------------------------------------------------------------
#ifndef LOG_FILE
#define LOG_FILE			"/tmp/yhhtpd.log"
#define LOG_FORMAT			"CLF"
#endif

//-----------------------------------------------------------------------------
class CmWebLog : public Cyhook
{
	static pthread_mutex_t	WebLog_mutex;
	static FILE 		*WebLogFile;
	static std::string	WebLogFilename;
	static int 		RefCounter;	// Count Instances
	
public:
	CmWebLog();
	~CmWebLog();

	static bool 	OpenLogFile();
	static void 	CloseLogFile();
	void 		AddLogEntry_CLF(CyhookHandler *hh);
	bool 		printf(const char *fmt, ...);
	
	// Hooks
	virtual THandleStatus 	Hook_EndConnection(CyhookHandler *hh); 
	virtual std::string 	getHookName(void) {return std::string("WebLogging");}
	virtual THandleStatus 	Hook_ReadConfig(CConfigFile *Config, CStringList &ConfigList); 
protected:
//	bool CheckAuth(CyhookHandler *hh);
//	std::string decodeBase64(const char *b64buffer);
};
#endif // __yhttpd_mod_weblog_h__


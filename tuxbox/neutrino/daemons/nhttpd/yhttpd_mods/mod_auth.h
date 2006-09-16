//=============================================================================
// YHTTPD
// mod_auth : Authentication
//=============================================================================
#ifndef __yhttpd_mod_auth_h__
#define __yhttpd_mod_auth_h__


#include "yhook.h"
class CmAuth : public Cyhook
{
public:
	CmAuth(){};
	~CmAuth(){};

	// Hooks
	virtual THandleStatus 	Hook_SendResponse(CyhookHandler *hh); 
	virtual std::string 	getHookName(void) {return std::string("Auth");}
	virtual THandleStatus 	Hook_ReadConfig(CConfigFile *Config, CStringList &ConfigList); 
protected:
	bool CheckAuth(CyhookHandler *hh);
	std::string decodeBase64(const char *b64buffer);
};
#endif // __yhttpd_mod_auth_h__


#ifndef __econfig_h
#define __econfig_h

#include <lib/system/nconfig.h>

class eConfig: public NConfig
{
	static eConfig *instance;
public:
	static eConfig *getInstance() { return instance; }
	eConfig();
	~eConfig();
};

#endif

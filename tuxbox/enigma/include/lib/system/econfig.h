#ifndef __econfig_h
#define __econfig_h

#include <lib/system/nconfig.h>

class eConfig: public NConfig
{
	static eConfig *instance;
	int ppin;
public:
	int locked;
	static eConfig *getInstance() { return instance; }
	void setParentalPin( int pin )
	{
		ppin = pin;
		setKey("/elitedvb/pins/parentallock", ppin );
	}
	int getParentalPin() { return ppin; }
	int pLockActive()
	{
		int tmp = ppin && locked;
		if ( tmp )
		{
			int hidelocked=0;
			if (eConfig::getInstance()->getKey("/elitedvb/hidelocked", hidelocked ))
				hidelocked=0;
			if ( hidelocked )
				tmp |= 2;
		}
		return tmp;
	}
	eConfig();
	~eConfig();
};

#endif

#ifndef __econfig_h
#define __econfig_h

// #include <lib/system/nconfig.h>

#include <map>
#include <string>

class eConfig // : public NConfig
{
	static eConfig *instance;
	int ppin;
	
	std::map<std::string, int> keys_int;
	std::map<std::string, std::string> keys_string;
	std::map<std::string, unsigned int> keys_uint;
	std::map<std::string, double> keys_double;
	
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
	
	
	int getKey(const char *, int &);
	int getKey(const char *, unsigned int &);
	int getKey(const char *, double &);
	int getKey(const char *, char * &string);
	
	int setKey(const char *, const int &);
	int setKey(const char *, const unsigned int &);
	int setKey(const char *, const double &);
	int setKey(const char *, const char *);
	
	void delKey(const char *);
	
	void flush();
	
	eConfig();
	~eConfig();
};

#endif

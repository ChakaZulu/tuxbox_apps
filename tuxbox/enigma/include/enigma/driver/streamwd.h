#ifndef __streamwd_h
#define __streamwd_h

#include <include/libsig_comp.h>
#include <core/base/ebase.h>
#include <core/dvb/si.h>
#include <core/dvb/dvb.h>

class eStreamWatchdog: public Object
{
	eSocketNotifier* sn;
	int handle;
	int isanamorph;
	static eStreamWatchdog *instance;
private:
	void check(int);
public:
	void reloadSettings();
	eStreamWatchdog();
	~eStreamWatchdog();
	static eStreamWatchdog *getInstance();
	int isAnamorph();
	Signal1<void, int> AspectRatioChanged;
};

#endif

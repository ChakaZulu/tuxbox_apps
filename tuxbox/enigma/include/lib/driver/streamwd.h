#ifndef __streamwd_h
#define __streamwd_h

#include <ebase.h>
#include <libsig_comp.h>
#include "si.h"
#include "dvb.h"

class eStreamWatchdog: public Object
{
	eSocketNotifier* sn;
	int handle;
	static eStreamWatchdog *instance;
private:
	void check(int);
public:
	void reloadSettings();
	eStreamWatchdog();
	~eStreamWatchdog();
	static eStreamWatchdog *getInstance();
	Signal1<void, int> AspectRatioChanged;
};

#endif

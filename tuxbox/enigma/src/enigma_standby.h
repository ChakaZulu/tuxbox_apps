#ifndef __SRC_ENIGMA_STANDBY_H__
#define __SRC_ENIGMA_STANDBY_H__

#include <lib/gui/ewidget.h>
#include <lib/dvb/dvb.h>

class eZapStandby: public eWidget
{
	static eZapStandby *instance;
	eServiceReference ref;
	int rezap;
	int oldpin8;
	int dontStopService;
protected:
	int eventHandler(const eWidgetEvent &);
public:
	void setDontStopService() { dontStopService=1; }
	void renewSleep();
	void wakeUp(int norezap);
	static eZapStandby *getInstance() { return instance; }
	static Signal0<void> enterStandby, leaveStandby, RCWakeUp;
	eZapStandby();
	~eZapStandby()
	{
		instance=0;
	}
};

#endif

#ifndef __streamwd_h
#define __streamwd_h

#include "qsocketnotifier.h"
#include <libsig_comp.h>
#include "si.h"
#include "dvb.h"

class eStreamWatchdog: public Object
{
//	Q_OBJECT
	QSocketNotifier* sn;
	int handle;
	static eStreamWatchdog *instance;
private:// slots:
	void check(int);
public:
	void reloadSettings();
	eStreamWatchdog();
	~eStreamWatchdog();
	static eStreamWatchdog *getInstance();
/*signals:
	void AspectRatioChanged(int);*/
	Signal1<void, int> AspectRatioChanged;
};

#endif

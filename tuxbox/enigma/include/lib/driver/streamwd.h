#ifndef __streamwd_h
#define __streamwd_h

#include "qtimer.h"
#include "si.h"
#include "dvb.h"

class eStreamWatchdog: public QObject
{
	Q_OBJECT
	int last;
	QTimer timer;
	static eStreamWatchdog *instance;
	PMTEntry* stream;
private slots:
	void checkstate();
public:
	eStreamWatchdog();
	~eStreamWatchdog();
	
	void reloadSettings();
	
	static eStreamWatchdog *getInstance();
signals:
	void AspectRatioChanged(int);
};

#endif

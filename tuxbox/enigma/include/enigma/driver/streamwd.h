#ifndef __streamwd_h
#define __streamwd_h

#include "qtimer.h"

class eStreamWatchdog: public QObject
{
	Q_OBJECT
	int last;
	QTimer timer;
	static eStreamWatchdog *instance;
private slots:
	void checkstate();
public:
	eStreamWatchdog();
	~eStreamWatchdog();
	
	void reloadSettings();
	
	static eStreamWatchdog *getInstance();
};

#endif

#ifndef __rc_h
#define __rc_h

#include <qobject.h>
#include <qfile.h>
#include <qsocketnotifier.h>
#include <qtimer.h>

class eRCInput: public QObject
{
	Q_OBJECT
signals:
	void keyDown(int code);
	void keyUp(int code);
private slots:
	int keyPressed(int);
	int timeOut();
	int repeat();
private:
	QFile rc;
	QSocketNotifier *sn;
	QTimer timeout, repeattimer;
	void processKeyEvent(int code, int isbreak=0);
	int ccode;
	int locked;
	int translate(int code);
	static eRCInput *instance;
public:
	enum
	{
		RC_0=0, RC_1=0x1, RC_2=0x2, RC_3=0x3, RC_4=0x4, RC_5=0x5, RC_6=0x6, RC_7=0x7,
		RC_8=0x8, RC_9=0x9,
		RC_RIGHT=10, RC_LEFT=11, RC_UP=12, RC_DOWN=13, RC_OK=14, RC_MUTE=15,
		RC_STANDBY=16, RC_GREEN=17, RC_YELLOW=18, RC_RED=19, RC_BLUE=20, RC_PLUS=21, RC_MINUS=22,
		RC_HELP=23, RC_DBOX=24,
		RC_UP_LEFT=27, RC_UP_RIGHT=28, RC_DOWN_LEFT=29, RC_DOWN_RIGHT=30, RC_HOME=31
	};
	eRCInput();
	~eRCInput();
	
	int rdelay, rrate;
	
	int lock();
	void unlock();
	
	static eRCInput *getInstance() { return instance; }
};

#endif

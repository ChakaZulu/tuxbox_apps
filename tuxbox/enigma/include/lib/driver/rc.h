#ifndef __rc_h
#define __rc_h

//#include <qobject.h>
#include <qsocketnotifier.h>
#include <qtimer.h>
#include <list>

#include <sigc++/signal_system.h>
#ifdef SIGC_CXX_NAMESPACES
using namespace SigC;
#endif

class eRCInput;
class eRCDriver;

class eRCDevice: public Object
{
protected:
	int rrate, rdelay;
	eRCInput *input;
	eRCDriver *driver;
public:
	eRCDevice(eRCDriver *input);
	~eRCDevice();
	virtual void handleCode(int code)=0;
	virtual const char *getDescription() const=0;
};

class eRCDriver: public Object
{
//	Q_OBJECT
protected:
	std::list<eRCDevice*> listeners;
	eRCInput *input;
public:
	eRCDriver(eRCInput *input);
	eRCInput *getInput() const { return input; }
	void addCodeListener(eRCDevice *dev)
	{
		listeners.push_back(dev);
	}
	void removeCodeListener(eRCDevice *dev)
	{
		listeners.remove(dev);
	}
	~eRCDriver();
};

class eRCShortDriver: public eRCDriver
{
//	Q_OBJECT
protected:
	int handle;
	QSocketNotifier *sn;
/*private slots:*/
	void keyPressed(int);
public:
	eRCShortDriver(const char *filename);
	~eRCShortDriver();
};

class eRCKey
{
protected:
	eRCDevice *producer;
	int code, flags;
public:
	eRCKey(eRCDevice *producer, int code, int flags): 
		producer(producer), code(code), flags(flags)
	{
	}
	enum
	{
		flagBreak=1,
		flagRepeat=2
	};
	virtual const char *getDescription() const =0;
	bool operator>(const eRCKey &o) const
	{
		return (o.code==code) && 
			((o.flags&flagBreak)==(flags&flagBreak)) && 
			((o.flags&flagRepeat) || !(flags&flagRepeat));
	}
	int getFlags() const { return flags; }
	eRCDevice *getProducer() const { return producer; }
	virtual int getCompatibleCode() const;
};

class eRCInput: public Object
{
//	Q_OBJECT
	int locked;	
	int handle;
	static eRCInput *instance;
//signals:
//	void keyEvent(const eRCKey &);
public:
	Signal1<void, const eRCKey&> keyEvent;
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
	
	int lock();
	void unlock();
	void close();
	bool open();

	void setFile(int handle);

	void keyPressed(const eRCKey &key)
	{
		/*emit*/ keyEvent(key);
	}
	
	static eRCInput *getInstance() { return instance; }
};

#endif

#ifndef __scan_h
#define __scan_h

#include <core/dvb/dvb.h>
#include <core/gui/multipage.h>
#include <core/gui/ewidget.h>

class eWindow;
class eLabel;
class eProgress;

class tsText: public eWidget
{
	eLabel *headline, *body;
protected:
	int eventHandler(const eWidgetEvent &event);
public:
	tsText(eString headline, eString body, eWidget *parent);
};

class tpPacket
{
public:
	tpPacket(eString name, int scanflags): name(name), scanflags(scanflags)	
	{
		possibleTransponders.setAutoDelete(true);	
	}
	eString name;
	int scanflags;
	ePtrList<eTransponder> possibleTransponders;
};

class tsFindInit: public eWidget
{
	eLabel *headline, *body;
	eProgress *signalbar;
	eTimer sstimer;
	ePtrList<tpPacket> packets;
	enum
	{
		sInactive, sSearching, sFound, sFailed
	};
	void scanPacket();
	int state;
protected:
	int eventHandler(const eWidgetEvent &event);
private:
	void tunedIn(eTransponder *trans, int error);
	void showSignalStrength();
public:
	void redrawWidget();
	tsFindInit(eWidget *parent);
	
	tpPacket *result;
};

class tsDoScan: public eWidget
{
	eTimer etatimer;
	eLabel *transp_found, *transp_scanned, *known_services, *eta, *dummy1, *dummy2;
	eProgress *bar, *dummybar;
	
	time_t start;
	time_t fexp;
	tsFindInit *init;
protected:
	void updateStats();
	virtual int eventFilter(const eWidgetEvent &event);
private:
	void stateChanged(int newstate);
	void eventOccured(int event);
	void updateETA();
public:
	tsDoScan(tsFindInit *init, eWidget *parent);
	void redrawWidget();
};

class TransponderScan
{
	eWindow *window;
	eProgress *progress;
	eLabel *progress_text;
	eMultipage mp;
public:
	TransponderScan();
	~TransponderScan();
	int exec();
};

#endif

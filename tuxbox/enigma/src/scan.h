#ifndef __scan_h
#define __scan_h

#include <lib/dvb/dvb.h>
#include <lib/gui/ewidget.h>
#include <lib/gui/combobox.h>
#include <lib/gui/statusbar.h>

class eWindow;
class eLabel;
class eProgress;
class eButton;
class eCheckbox;
class eTransponderWidget;
class eFEStatusWidget;
class eDVBEvent;
class eDVBState;

class tsSelectType: public eWidget
{
	eListBox<eListBoxEntryText> *list;
	void selected(eListBoxEntryText *entry);
	int eventHandler( const eWidgetEvent &e );
public:
	tsSelectType(eWidget *parent);
};

class tsManual: public eWidget
{
	eTransponder transponder;
	eButton *b_start;
	eTransponderWidget *transponder_widget;
	eFEStatusWidget *festatus_widget;
	eCheckbox *c_clearlist, *c_searchnit, *c_useonit, *c_usebat;
	eTimer updateTimer;
	void start();
	void abort();
	void retune();
	void update();
	int eventHandler(const eWidgetEvent &event);
public:
	tsManual(eWidget *parent, const eTransponder &transponder, eWidget* LCDTitle=0, eWidget* LCDElement=0);
	eTransponder &getTransponder() { return transponder; }
};

class tsAutomatic: public eWidget
{
	eButton *b_start;
	eComboBox *l_network;
	eCheckbox *c_eraseall;
	eCheckbox *c_nocircular;
	eLabel *l_status;
	std::list<eTransponder>::iterator current_tp, last_tp;
	int automatic;
	void start();
	void networkSelected(eListBoxEntryText *l);
	void dvbEvent(const eDVBEvent &event);
	int loadNetworks();
	int nextNetwork(int first=0);
	int nextTransponder(int next);
	int tuneNext(int next);
public:
	tsAutomatic(eWidget *parent);
};

class tsText: public eWidget
{
	eLabel *headline, *body;
protected:
	int eventHandler(const eWidgetEvent &event);
public:
	tsText(eString headline, eString body, eWidget *parent);
};

class tsScan: public eWidget
{
	eTimer timer;
	eLabel *timeleft, *service_name, *service_provider, *services_scanned, *transponder_scanned;
	eProgress *progress;
	int tpLeft, scantime;
protected:
	int eventHandler(const eWidgetEvent &event);
	void dvbEvent(const eDVBEvent &event);
	void dvbState(const eDVBState &event);
	void updateTime();
	void serviceFound( const eServiceReferenceDVB &, bool );
	void addedTransponder( eTransponder* );
public:
	int tpScanned, newTVServices, newRadioServices, newDataServices, servicesScanned, newTransponders;
	tsScan(eWidget *parent);
};

class TransponderScan: public eWindow
{
	eProgress *progress;
	eLabel *progress_text;
	eStatusBar *statusbar;	
	eWidget *current;
#ifndef DISABLE_LCD
	eWidget *LCDElement, *LCDTitle;
#endif
public:
	enum tState
	{
		stateMenu,
		stateManual,
		stateAutomatic,
		stateScan,
		stateDone,
		stateEnd
	};

	TransponderScan( eWidget* LCDTitle=0, eWidget* LCDElement=0 );
	~TransponderScan();
	int exec(tState initial=stateMenu);
	int eventHandler(const eWidgetEvent&);
};

#endif

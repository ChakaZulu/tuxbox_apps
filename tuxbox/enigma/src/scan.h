#ifndef __scan_h
#define __scan_h

#include <core/dvb/dvb.h>
#include <core/gui/ewidget.h>
#include <core/gui/listbox.h>

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
public:
	tsSelectType(eWidget *parent);
};

class tpPacket
{
public:
	std::string name;
	int scanflags;
	std::list<eTransponder> possibleTransponders;
};

class tsManual: public eWidget
{
	eTransponder transponder;
	eButton *b_start, *b_abort;
	eTransponderWidget *transponder_widget;
	eFEStatusWidget *festatus_widget;
	eCheckbox *c_clearlist, *c_searchnit, *c_useonit, *c_usebat;
	void start();
	void abort();
	void retune();
	int eventHandler(const eWidgetEvent &event);
public:
	tsManual(eWidget *parent, const eTransponder &transponder);
};

class tsAutomatic: public eWidget
{
	eButton *b_start, *b_abort;
	eListBox<eListBoxEntryText> *l_lnb, *l_network;
	eLabel *l_status;
	std::list<tpPacket> networks;
	std::list<eTransponder>::iterator current_tp, last_tp;
	int automatic;
	void start();
	void abort();
	void networkSelected(eListBoxEntryText *l);
	void dvbEvent(const eDVBEvent &event);
	int loadNetworks();
	int addNetwork(tpPacket &p, XMLTreeNode *node, int type);
	
	int nextNetwork(int first=0);
	int nextTransponder(int next, int lnb);
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
	eLabel *headline;
protected:
	int eventHandler(const eWidgetEvent &event);
	void dvbEvent(const eDVBEvent &event);
	void dvbState(const eDVBState &event);
public:
	tsScan(eWidget *parent);
};

class TransponderScan
{
	eWindow *window;
	eProgress *progress;
	eLabel *progress_text;
	
	eWidget *select_type, *manual_scan, *automatic_scan;
public:
	TransponderScan();
	~TransponderScan();
	int exec();
};

#endif

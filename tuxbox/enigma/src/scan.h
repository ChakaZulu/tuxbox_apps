#ifndef __scan_h
#define __scan_h

#include <core/dvb/dvb.h>
#include <core/gui/multipage.h>
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

class tpScanParameter
{
public:
	std::string name;
	int scanflags;
	std::list<eTransponder> possibleTransponders;
};

class tsManual: public eWidget
{
	tpScanParameter &param;
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
	tsManual(eWidget *parent, const eTransponder &transponder, tpScanParameter &param);
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
	eMultipage mp;
	
	eWidget *select_type, *manual_scan, *automatic_scan;

	tpScanParameter scanparam;
public:
	TransponderScan();
	~TransponderScan();
	int exec();
};

#endif

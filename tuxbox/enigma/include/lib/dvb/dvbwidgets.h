#ifndef __src_core_dvb_dvbwidgets_h
#define __src_core_dvb_dvbwidgets_h

#include <core/gui/ewidget.h>
#include <core/gui/listbox.h>
#include <core/base/ebase.h>

class eNumber;
class eTransponder;
class eCheckbox;
class eProgress;
class eFrontend;

class eTransponderWidget: public eWidget
{
	eNumber *frequency, *symbolrate, *lnb;
	eCheckbox *inversion;
	int type, edit;
	eListBoxEntryText *fecEntry[6], *polarityEntry[4];
	
	eListBox<eListBoxEntryText> *fec, *polarity;
	void nextField0(int *);
	void nextField1(eListBoxEntryText *);
public:
	enum type
	{
		deliveryCable, deliverySatellite
	};
	eTransponderWidget(eWidget *parent, int edit, int type);
	int load();
	int setTransponder(eTransponder *transponder);
};

class eFEStatusWidget: public eWidget
{
	eProgress *p_snr, *p_agc;
	eCheckbox *c_sync, *c_lock;
	eFrontend *fe;
	eTimer updatetimer;
	void update();
	int eventHandler(const eWidgetEvent &);
public:
	eFEStatusWidget(eWidget *parent, eFrontend *fe);
};

#endif

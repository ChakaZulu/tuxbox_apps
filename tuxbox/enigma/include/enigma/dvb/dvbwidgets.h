#ifndef __src_core_dvb_dvbwidgets_h
#define __src_core_dvb_dvbwidgets_h

#include <core/gui/ewidget.h>
#include <core/gui/eListBox.h>

class eNumber;
class eTransponder;
class eCheckbox;

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

#endif

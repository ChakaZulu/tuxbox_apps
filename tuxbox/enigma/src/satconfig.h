#ifndef __satconfig_h
#define __satconfig_h

#include <list>

#include <core/gui/ebutton.h>
#include <core/gui/enumber.h>
#include <core/gui/listbox.h>
#include <core/dvb/dvb.h>

class eButton;

class eSatelliteConfiguration: public eWindow
{
public:
	eSatelliteConfiguration(int sat);
};

class eSatelliteConfigurationManager: public eWindow
{
	eWidget *w_buttons;
	eButton *button_close;
	int eventFilter(const eWidgetEvent &event);
public:
	void okPressed();
	bool lnbSelected(eString& str);
public:
	eSatelliteConfigurationManager();
	~eSatelliteConfigurationManager();
};

class eListBoxEntryLNB: public eListBoxEntryText
{
	friend class eListBox<eListBoxEntryLNB>;
public:
	const eLNB* lnb;
	eListBoxEntryLNB(eListBox<eListBoxEntryLNB>* lb, const eLNB* lnb, eString text)
		:eListBoxEntryText(((eListBox<eListBoxEntryText>*)lb), text), lnb(lnb)
	{
	}		
};


class eLNBSelitor: public eWindow  // Selitor = "Sel"ector + Ed"itor" :-)
{
	eListBox<eListBoxEntryLNB> *lnb_list;
	eNumber *lofH, *lofL, *lnbThreshold;
	eButton *use; 	// use this LNB for Satelite and close LNBSelitor
	eButton *apply; // apply changed to selected LNB
	eButton *remove; // remove the selected LNB
public:
	eLNBSelitor();

	void setCurrentLNB(int num)
	{
	}

	int getCurrentLNB()
	{
		return 0;
	}
};

inline eLNBSelitor::eLNBSelitor()
{
	// add all LNBs

	int i=0;

	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		new eListBoxEntryLNB(lnb_list, &(*it), eString().sprintf("LNB %i", i++));
	
	// add a None LNB
	new eListBoxEntryLNB(lnb_list, 0, _("New"));
}


#endif

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
	int eventHandler(const eWidgetEvent &event);
public:
	void okPressed();
	bool lnbSelected(eString& str);
	bool satSelected(eString& str);
	bool DISEqCSelected(eString& str);	
public:
	eSatelliteConfigurationManager();
	~eSatelliteConfigurationManager();
};

class eLNBSelitor: public eWindow  // Selitor = "Sel"ector + Ed"itor" :-)
{
	eListBox<eListBoxEntryText> *lnb_list;
	eNumber *lofH, *lofL, *threshold;
	eButton *use; 	// use this LNB for Satelite and close LNBSelitor
	eButton *apply; // apply changed to selected LNB
	eButton *remove; // remove the selected LNB
	eButton *cancel; // close the window without apply changes
	int eventHandler(const eWidgetEvent &event);
	void selected(int*);
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

#endif

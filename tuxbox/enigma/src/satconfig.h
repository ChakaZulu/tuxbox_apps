#ifndef __satconfig_h
#define __satconfig_h

#include <list>

#include <apps/enigma/scan.h>
#include <core/gui/ebutton.h>
#include <core/gui/combobox.h>
#include <core/gui/enumber.h>
#include <core/gui/listbox.h>
#include <core/dvb/dvb.h>


class eButton;

struct SatelliteEntry
{
	eComboBox *sat, *voltage, *hilo;
	eButton *lnb;
};

class eSatelliteConfigurationManager: public eWindow, public existNetworks
{
	eTimer* refresh;
	eWidget *w_buttons;
	eButton *button_close, *button_new;
	SatelliteEntry* deleteThisEntry;
	int eventHandler(const eWidgetEvent &event);
	std::map< eSatellite*, SatelliteEntry > entryMap;
	eSatellite *getSat4SatCombo( const eComboBox* );
	eSatellite *getSat4HiLoCombo( const eComboBox* );
	eSatellite *getSat4VoltageCombo( const eComboBox* );
	eSatellite *getSat4LnbButton( const eButton* );
	void createControlElements();
	void addSatellite(eSatellite* sat);
	void repositionWidgets();
public:
	void closePressed();
	void newPressed();
	void lnbSelected(eButton *who);
	void satChanged(eComboBox *who, eListBoxEntryText *le);
	void hiloChanged(eComboBox *who, eListBoxEntryText *le);
	void voltageChanged(eComboBox *who, eListBoxEntryText *le);
public:
	eSatelliteConfigurationManager();
	~eSatelliteConfigurationManager();
};

class eLNBSelitor: public eWindow  // Selitor = "Sel"ector + Ed"itor" :-)
{
	struct selectlnb;
	eListBox<eListBoxEntryText> *lnb_list;
	eNumber *lofH, *lofL, *threshold;
	eButton *save; 	// use this LNB for Satelite and close LNBSelitor
	eButton *cancel; // close the window without apply changes
	eComboBox *DiSEqCMode, *DiSEqCParam;
	int eventHandler(const eWidgetEvent &event);
	void numSelected(int*);
	void lnbSelected(eListBoxEntryText*);
	void lnbChanged( eListBoxEntryText* );
	void DiSEqCModeChanged ( eListBoxEntryText* );
	void onSave();
	eSatellite* sat;
public:
	eLNBSelitor( eSatellite *sat );
};

#endif

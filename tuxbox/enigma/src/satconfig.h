#ifndef __satconfig_h
#define __satconfig_h

#include <list>

#include <scan.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/combobox.h>
#include <lib/gui/enumber.h>
#include <lib/gui/multipage.h>
#include <lib/gui/listbox.h>
#include <lib/dvb/dvb.h>

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

class eLNBSetup : public eWindow // Selitor = "Sel"ector + Ed"itor" :-)
{
  eMultipage mp;
  eWidget *DiSEqCPage, *LNBPage;
  eSatellite* sat;
  void onSave();
  void onNext() { mp.next(); }
  void onPrev() { mp.prev(); }  
  int eventHandler(const eWidgetEvent &event);
public:
	eLNBSetup( eSatellite *sat );
};

class eLNBPage : public eWidget
{
  friend class eLNBSetup;
  struct selectlnb;
  eSatellite *sat;
	eListBox<eListBoxEntryText> *lnb_list;
  eNumber *lofH, *lofL, *threshold;
	eButton *save; 	 // use this LNB for Satelite and close LNBSelitor
	eButton *cancel; // close the window without apply changes
  eButton *next; // shows the DiSEqC Configuration Dialog
  eStatusBar *statusbar;
    
  int eventHandler(const eWidgetEvent &event);
  void numSelected(int*);
  void lnbChanged( eListBoxEntryText* );
	void lnbSelected(eListBoxEntryText*);
public:
  eLNBPage( eWidget *parent, eSatellite *sat );

};

class eDiSEqCPage : public eWidget
{
  friend class eLNBSetup;
  eSatellite *sat;
	eComboBox *DiSEqCMode, *DiSEqCParam, *MiniDiSEqCParam, *DiSEqCRepeats;
  eCheckbox *SeqRepeat, *uncommitted, *uncommitted_gap, *useGotoXX;
  eNumber *rotorOffset;
  eButton *save; 	 // use this LNB for Satelite and close LNBSelitor
	eButton *cancel; // close the window without apply changes
  eButton *prev; // shows the LNB Configuration Dialog
  eLabel *lRotorOffset, *lDiSEqCRepeats, *lDiSEqCParam;
  eStatusBar *statusbar;
          
  int eventHandler(const eWidgetEvent &event);
  void lnbChanged( eListBoxEntryText* );
  void DiSEqCModeChanged( eListBoxEntryText* );
public:
  eDiSEqCPage( eWidget *parent, eSatellite *sat );
};

/*
class eRotorPage : public eWidget
{
  friend class eLNBSetup
  eSatellite *sat;
  eListBox *positions;
  eNumber *orbital_position, *number;
  eButton *add, *remove;
  eCheckbox *fine;  // if is checked.. then left..right is for finetuning
  eFEStatusWidget *state
};*/     

#endif

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

class eFEStatusWidget;

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

class eLNBPage;
class eDiSEqCPage;
class eRotorPage;
                 
class eLNBSetup : public eWindow // Selitor = "Sel"ector + Ed"itor" :-)
{
  eMultipage mp;
  eDiSEqCPage *DiSEqCPage;
  eLNBPage *LNBPage;
  eRotorPage *RotorPage;
  eSatellite* sat;
  void onSave();
  void onNext() { mp.next(); }
  void onPrev() { mp.prev(); }  
  int eventHandler(const eWidgetEvent &event);
public:
	eLNBSetup( eSatellite *sat, eWidget* lcdTitle, eWidget* lcdElement );
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
  eCheckbox *increased_voltage;
  eStatusBar *statusbar;
    
  void numSelected(int*);
  void lnbChanged( eListBoxEntryText* );
	void lnbSelected(eListBoxEntryText*);
  void updateText(const eWidget* w);  // for Statusbar....
public:
  eLNBPage( eWidget *parent, eSatellite *sat );
};

class eDiSEqCPage : public eWidget
{
  friend class eLNBSetup;
  eSatellite *sat;
	eComboBox *DiSEqCMode, *DiSEqCParam, *MiniDiSEqCParam, *DiSEqCRepeats;
  eCheckbox *SeqRepeat, *uncommitted, *uncommitted_gap;
  eButton *save; 	 // use this LNB for Satelite and close LNBSelitor
	eButton *cancel; // close the window without apply changes
  eButton *prev; // shows the LNB Configuration Dialog
  eButton *next; // shows the Rotor Setup (for non GotoXX Rotors)
  eLabel *lDiSEqCRepeats, *lDiSEqCParam;
  eStatusBar *statusbar;
          
  void lnbChanged( eListBoxEntryText* );
  void DiSEqCModeChanged( eListBoxEntryText* );
  void numSelected(int*);
  void updateText(const eWidget* w);  // for Statusbar....
public:
  eDiSEqCPage( eWidget *parent, eSatellite *sat );
};


class eRotorPage : public eWidget
{
  friend class eLNBSetup;
  eSatellite *sat;
  eLNB *curlnb;
  eListBox<eListBoxEntryText> *positions;
  eLabel *lRotorOffset;
  eNumber *orbital_position, *number, *RotorOffset;
  eButton *add, *remove, *prev, *save, *cancel;
  eCheckbox *fine, *useGotoXX;  // if is checked.. then left..right is for finetuning
  eComboBox *direction;
  eFEStatusWidget *feState;
  eStatusBar* statusbar;
  void onAdd();
  void onRemove();
  void numSelected(int*);
  void lnbChanged( eListBoxEntryText* );
  void posChanged( eListBoxEntryText* );
  void updateText(const eWidget* w);  // for Statusbar....
public:  
  eRotorPage( eWidget *parent, eSatellite *sat );
};

#endif

#ifdef ENABLE_RFMOD

#ifndef __setuprfmod_h
#define __setuprfmod_h

#include <lib/gui/ewindow.h>
#include <lib/gui/listbox.h>
#include <lib/gui/statusbar.h>

class eNumber;
class eButton;
class eCheckbox;

class eZapRFmodSetup: public eWindow
{
	eButton *ok;
	eStatusBar *status;

	eLabel *sscl, *cl, *ftl;
	eCheckbox *TestPatternEnable, *SoundEnable, *Standby;
	eListBox<eListBoxEntryText> *SoundSubcarrier, *Channel, *FineTune;

	void TestPatternEnable_selected();
	void SoundEnable_selected();
	void Standby_selected();
	void SoundSubcarrier_selected(eListBoxEntryText* entry);
	void Channel_selected(eListBoxEntryText* entry);
	void FineTune_selected(eListBoxEntryText* entry);
	int eventHandler( const eWidgetEvent& );

	int chan,soundenable,ssc,finetune,standby;

public:
	eZapRFmodSetup();
};

#endif

#endif

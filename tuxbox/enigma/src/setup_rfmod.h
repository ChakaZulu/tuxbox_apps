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
	eButton *abort, *ok;
	eStatusBar *status;

	eCheckbox* TestPatternEnable;
	eCheckbox* SoundEnable;
	
	eListBox<eListBoxEntryText> *SoundSubcarrier;
	eListBox<eListBoxEntryText> *Channel;
	eListBox<eListBoxEntryText> *FineTune;
	
private:
	void okPressed();
	void abortPressed();
	void TestPatternEnable_selected();
	void SoundEnable_selected();
	void SoundSubcarrier_selected(eListBoxEntryText* entry);
	void Channel_selected(eListBoxEntryText* entry);
	void FineTune_selected(eListBoxEntryText* entry);

	int SFD,SO,DIV;

public:
	eZapRFmodSetup();
	~eZapRFmodSetup();
};

#endif

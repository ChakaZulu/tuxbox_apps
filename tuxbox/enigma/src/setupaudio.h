#ifndef __setupaudio_h
#define __setupaudio_h

#include "ewindow.h"

class eListbox;
class eListboxEntry;
class eButton;

class eZapAudioSetup: public eWindow
{
	Q_OBJECT
	eListbox *listbox;
	eButton *ok, *abort;
	int useAC3;
private slots:
  int eventFilter(const eWidgetEvent &event);
	void okPressed();
	void abortPressed();
  void sel_AC3(eListboxEntry *);
public:
	eZapAudioSetup();
	~eZapAudioSetup();
};

#endif

#ifndef __enigmaci_h
#define __enigmaci_h

#include <lib/gui/ewindow.h>
#include <lib/gui/listbox.h>
#include <lib/gui/statusbar.h>

class eNumber;
class eButton;
class eDVBCI;

class eListBoxMenuEntry: public eListBoxEntryText
{
	friend class eListBox<eListBoxMenuEntry>;
public:
	eListBoxMenuEntry(eListBox<eListBoxMenuEntry> *parent, eString name)
	:eListBoxEntryText((eListBox<eListBoxEntryText>*)parent, name)
	{
	}
};	

class enigmaCImmi: public eWindow
{
	eButton *ok,*abort;
	eListBox<eListBoxMenuEntry> *lentrys;
	eStatusBar *status;
	eLabel *tt,*stt,*bt,*cistate;
	eDVBCI *DVBCI;

private:
	void okPressed();
	void abortPressed();
	void getmmi(const char *buffer);
	int ci_state;

public:
	enigmaCImmi();
	~enigmaCImmi();
};

class enigmaCI: public eWindow
{
	eButton *ok,*reset,*init,*app;
	eStatusBar *status;
	eDVBCI *DVBCI;

private:
	void okPressed();
	void abortPressed();
	void resetPressed();
	void initPressed();
	void appPressed();
	void updateCIinfo(const char*);

public:
	enigmaCI();
	~enigmaCI();
};

#endif

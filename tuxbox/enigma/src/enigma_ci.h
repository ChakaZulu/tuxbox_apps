#ifndef __enigmaci_h
#define __enigmaci_h

#include <lib/gui/ewindow.h>
#include <lib/gui/enumber.h>
#include <lib/gui/listbox.h>
#include <lib/gui/statusbar.h>

class eNumber;
class eButton;
class eDVBCI;

class eCImmi: public eWidget
{
public:
	eNumber *answer;
	eCImmi(eWidget *parent);
};	

class eListBoxMenuEntry: public eListBoxEntryText
{
	friend class eListBox<eListBoxMenuEntry>;
	int entry;
public:
	eListBoxMenuEntry(eListBox<eListBoxMenuEntry> *parent, eString name, int entry)
	:eListBoxEntryText((eListBox<eListBoxEntryText>*)parent, name),entry(entry)
	{
	}
	
	const int &getEntry() const {return entry;};
};	

class enigmaCImmi: public eWindow
{
	eButton *ok,*abort,*answok;
	eListBox<eListBoxMenuEntry> *lentrys;
	eStatusBar *status;
	eLabel *tt,*stt,*bt,*cistate,*headansw;
	eNumber *answ;
	eDVBCI *DVBCI;
	eCImmi *mmi;

private:
	void okPressed();
	void abortPressed();
	void entrySelected(eListBoxMenuEntry *choice);
	void getmmi(const char *buffer);
	long LengthField(unsigned char *lengthfield,long maxlength,int *fieldlen);
	void answokPressed();

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

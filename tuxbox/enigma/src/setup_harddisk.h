#ifndef __lib_apps_enigma_setup_harddisk_h
#define __lib_apps_enigma_setup_harddisk_h

#include <lib/gui/listbox.h>
#include <lib/gui/ebutton.h>

class eHarddiskSetup: public eListBoxWindow<eListBoxEntryText>
{
	int nr;
	void selectedHarddisk(eListBoxEntryText *sel);
public:
	eHarddiskSetup();
	int getNr() const { return nr; }
};

class eHarddiskMenu: public eWindow
{
	eButton *close, *format;
	eLabel *status, *model, *capacity, *bus;
	int dev;
	int numpart;
	void s_format();
	void readStatus();
public:
	eHarddiskMenu(int dev);
};

#endif

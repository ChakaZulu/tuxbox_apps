#ifndef __enigma_scan_h
#define __enigma_scan_h

#include <lib/gui/ewidget.h>
#include <lib/gui/listbox.h>

class eListboxEntry;

class eZapScan: public eListBoxWindow<eListBoxEntryMenu>
{
private:
	void sel_close();
	void sel_scan();
	void sel_bouquet();
	void sel_satconfig();	

public:
	eZapScan();
	~eZapScan();
};

#endif /* __enigma_scan_h */

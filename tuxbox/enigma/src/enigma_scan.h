#ifndef __enigma_scan_h
#define __enigma_scan_h

#include <core/gui/ewidget.h>
#include <core/gui/listbox.h>

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

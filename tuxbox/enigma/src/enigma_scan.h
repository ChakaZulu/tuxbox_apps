#ifndef __enigma_scan_h
#define __enigma_scan_h

#include <lib/gui/ewidget.h>
#include <lib/gui/listbox.h>

class eLNB;
class eListboxEntry;

class eLNBSelector: public eListBoxWindow<eListBoxEntryText>
{
	private:
		void selected( eListBoxEntryText* e);
	public:
		eLNBSelector();
};

class eZapScan: public eListBoxWindow<eListBoxEntryMenu>
{
private:
	void sel_close();
	void sel_scan();
	void sel_bouquet();
	void sel_satconfig();
	void sel_rotorConfig();
	eLNB* getRotorLNB();
public:
	eZapScan();
	~eZapScan();
};

#endif /* __enigma_scan_h */

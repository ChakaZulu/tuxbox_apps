#ifndef __enigma_scan_h
#define __enigma_scan_h

#include <setup_window.h>
#include <lib/gui/ewidget.h>

class eLNB;
class eListboxEntry;

class eLNBSelector: public eListBoxWindow<eListBoxEntryText>
{
	private:
		void selected( eListBoxEntryText* e);
	public:
		eLNBSelector();
};

class eZapScan: public eSetupWindow
{
private:
	void sel_satconfig();
	void sel_rotorConfig();
	void sel_transponderEdit();
	void sel_autoScan();
	void sel_manualScan();
public:
	static eLNB* getRotorLNB(int silent);
	eZapScan();
};

#endif /* __enigma_scan_h */

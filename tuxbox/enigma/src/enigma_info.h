#ifndef __enigma_info_h
#define __enigma_info_h

#include <lib/gui/ewidget.h>
#include <lib/gui/listbox.h>

class eZapInfo: public eListBoxWindow<eListBoxEntryMenu>
{
private:
	void sel_satfind();
	void sel_streaminfo();
	void sel_bnversion();
	void sel_about();	
public:
	eZapInfo();
	~eZapInfo();
};

#endif /* __enigma_info_h */

#ifndef __enigma_info_h
#define __enigma_info_h

#include <core/gui/ewidget.h>
#include <core/gui/listbox.h>

class eZapInfo: public eListBoxWindow<eListBoxEntryMenu>
{
private:
	void sel_close();
	void sel_streaminfo();
	void sel_bnversion();
	void sel_about();	

public:
	eZapInfo();
	~eZapInfo();
};

#endif /* __enigma_info_h */

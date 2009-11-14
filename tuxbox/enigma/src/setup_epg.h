
#ifndef __setup_epgcache_h_
#define __setup_epgcache_h_

#include <lib/gui/ebutton.h>
#include <lib/gui/textinput.h>
#include <lib/gui/elabel.h>
#include <lib/gui/emessage.h>
#include <lib/gui/echeckbox.h>


class eEPGSetup: public eWindow
{
	eButton *bt_seldir;
	eCheckbox *cb_cachebouquets, *cb_infobarcache;
	eTextInputField *tb_path, *tb_file;

	void selectDir();
	void clearCache();
	void storePressed();

	void init_eEPGSetup();
public:
	eEPGSetup();
	~eEPGSetup();
};

#endif

 

#ifndef DISABLE_HDD
#ifndef DISABLE_FILE

#ifndef __lib_apps_enigma_setup_harddisk_h
#define __lib_apps_enigma_setup_harddisk_h

#include <lib/dvb/edvb.h>
#include <lib/gui/listbox.h>
#include <lib/base/console.h>
#include <lib/gui/enumber.h>

class eButton;
class eComboBox;
class eStatusBar;

class eHarddiskSetup: public eListBoxWindow<eListBoxEntryText>
{
	int nr;
	void selectedHarddisk(eListBoxEntryText *sel);
	void init_eHarddiskSetup();
public:
	eHarddiskSetup();
	int getNr() const { return nr; }
};

class eHarddiskMenu: public eWindow
{
	eButton *ext, *format, *bcheck;
	eLabel *status, *model, *capacity, *bus, *lfs;
	eLabel *lbltimeout, *lblacoustic;
	eNumber *timeout, *acoustic;
	eButton *store, *standby;
	eComboBox *fs;
	eStatusBar *sbar;
	int dev;
	bool restartNet;
	int numpart;
	int visible;

	void storevalues();
	void hddstandby();
	void s_format();
	void extPressed();
	void check();
	void readStatus();
	void init_eHarddiskMenu();
public:
	eHarddiskMenu(int dev);
	~eHarddiskMenu()
	{
		if ( restartNet )
			eDVB::getInstance()->configureNetwork();
	}
};

class ePartitionCheck: public eWindow
{
	eLabel *lState;
	eButton *bCancel, *bClose;
	int dev;
	void onCancel();
	void fsckClosed(int);
	int eventHandler( const eWidgetEvent &e );
	void getData( eString );
	eConsoleAppContainer *fsck;
	void init_ePartitionCheck();
public:
	ePartitionCheck( int dev );
};

#endif

#endif //DISABLE_FILE
#endif //DISABLE_HDD

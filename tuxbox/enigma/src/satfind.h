#ifndef __src_upgrade_h
#define __src_upgrade_h

#include <lib/gui/ewindow.h>
#include <lib/base/ebase.h>

class eLabel;
class eNumber;
class eProgress;
class eButton;
class eFrontend;
class eCheckbox;

class eSatfind: public eWindow
{
	eProgress *p_snr, *p_agc, *p_ber;
	eLabel *lsnr_num, *lsync_num, *lber_num;
	eCheckbox *c_sync, *c_lock;
	eTimer updateTimer;
	eButton *ok;
	eFrontend *fe;
	int eventHandler( const eWidgetEvent& e);
public:
	eSatfind(eFrontend*);
	void closeWnd();
	void update();
};

#endif

#ifndef __setupvideo_h
#define __setupvideo_h

#include <lib/gui/ewindow.h>
#include <lib/gui/listbox.h>
#include <lib/gui/statusbar.h>

class eNumber;
class eButton;
class eCheckbox;

class eZapVideoSetup: public eWindow
{
	eButton *ok;
	eStatusBar *status;
	eCheckbox *c_disableWSS, *ac3default, *VCRSwitching;
	eListBox<eListBoxEntryText> *colorformat, *pin8, *tvsystem;

	unsigned int v_colorformat, v_pin8, v_disableWSS, v_tvsystem, v_VCRSwitching;
	eStatusBar *statusbar;
private:
	void ac3defaultChanged( int i );
	void CFormatChanged( eListBoxEntryText * );
	void VPin8Changed( eListBoxEntryText *);
	void DisableWSSChanged(int);
	void TVSystemChanged( eListBoxEntryText * );
	void VCRChanged(int);	
	void okPressed();
	int eventHandler( const eWidgetEvent &e );

public:
	eZapVideoSetup();
	~eZapVideoSetup();
};

#endif

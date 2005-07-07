#ifndef __setuprc_h
#define __setuprc_h

#include <lib/gui/ewindow.h>
#include <lib/gui/statusbar.h>

class eLabel;
class eButton;
class eSlider;
class eComboBox;
class eListBoxEntryText;
class eCheckbox;
class eNumber;

class eZapRCSetup: public eWindow
{
	eSlider *srrate, *srdelay;
	eLabel *lrrate, *lrdelay, *lrcStyle, *lNextCharTimeout;
	eNumber *NextCharTimeout;
	eStatusBar* statusbar;
	eComboBox* rcStyle;
	eString curstyle;

	int rdelay;
	int rrate;
	           
	eButton *ok;
	void okPressed();
	int eventHandler( const eWidgetEvent& );
	void repeatChanged( int );
	void delayChanged( int );
	void styleChanged( eListBoxEntryText* );
	void update();
	void setStyle();
	void nextField(int *);
	void init_eZapRCSetup();
public:
	eZapRCSetup();
	~eZapRCSetup();
private:
};

#endif

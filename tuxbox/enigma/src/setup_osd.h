#ifndef __setuposd_h
#define __setuposd_h

#include "ewindow.h"
class eCheckbox;
class eLabel;
class eButton;

class eZapOsdSetup: public eWindow
{
	eLabel *labelOsd, *labelFonts;
	eButton *ok, *abort;
	eCheckbox* bigosd;
	eCheckbox* bigfonts;
private:// slots:
	void fieldSelected(int *number);
	void okPressed();
	void abortPressed();
protected:
	int eventFilter(const eWidgetEvent &event);
public:
	eZapOsdSetup();
	~eZapOsdSetup();
private:
};

#endif

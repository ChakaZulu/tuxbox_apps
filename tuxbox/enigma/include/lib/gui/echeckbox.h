#ifndef __echeckbox_h
#define __echeckbox_h

#include <core/gui/ebutton.h>

class eCheckbox: public eButton
{
protected:
	int ischecked;
private:
	void sel();
	int eventFilter(const eWidgetEvent &event);
	void gotFocus();
public:
	Signal1<void, int> checked;
	eCheckbox(eWidget *parent, int checked=0, int takefocus=1, int Size=25);
	~eCheckbox();
	void setCheck(int c);
	int isChecked() { return ischecked; }
};

#endif

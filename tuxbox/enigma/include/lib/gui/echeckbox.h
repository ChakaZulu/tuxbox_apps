#ifndef __echeckbox_h
#define __echeckbox_h

#include <core/gui/ebutton.h>

class eCheckbox: public eButton
{
protected:
	int ischecked;
private:
	void sel();
	Signal1<void, int> checked;
	int eventFilter(const eWidgetEvent &event);
public:
	eCheckbox(eWidget *parent, int checked=0, int Size=25, eLabel* descr=0);
	~eCheckbox();
	void setCheck(int c);
	int isChecked() { return ischecked; }
};

#endif

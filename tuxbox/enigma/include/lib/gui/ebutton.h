#ifndef __ebutton_h
#define __ebutton_h

#include "elabel.h"
#include "grc.h"

class eButton: public eLabel
{
	gColor focus, normal;
	QString descr;
	Q_OBJECT
protected:
	void keyUp(int key);
	void gotFocus();
	void lostFocus();
	int eventFilter(const eWidgetEvent &event);
signals:
	void selected();
public:
	eButton(eWidget *parent, eLabel* descr=0);
};

#endif

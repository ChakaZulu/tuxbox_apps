#ifndef __echeckbox_h
#define __echeckbox_h

#include "ebutton.h"

class eCheckbox: public eButton
{
	Q_OBJECT
protected:
	int ischecked;
private slots:
	void sel();
signals:
	void checked(int);
public:
	eCheckbox(eWidget *parent, int checked=0, int Size=25, eLabel* descr=0);
	~eCheckbox();
	void setCheck(int c);
	int isChecked() { return ischecked; }
};

#endif

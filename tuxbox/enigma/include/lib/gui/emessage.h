#ifndef __emessage_h
#define __emessage_h

#include "ewindow.h"

class eLabel;

class eMessageBox: public eWindow
{
//	Q_OBJECT
	eLabel *text;
public:// slots:
	void okPressed();
public:
	eMessageBox(QString string, QString caption);
	~eMessageBox();
};

#endif

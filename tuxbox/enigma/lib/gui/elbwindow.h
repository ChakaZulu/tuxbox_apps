#ifndef __elbwindow_h
#define __elbwindow_h

#include "ewindow.h"
#include "elistbox.h"

class eLBWindow: public eWindow
{
//	Q_OBJECT
	void OnFontSizeChanged(int NewFontSize);
protected:
	int Entrys;
	int width;
public:
	eListbox *list;
	eLBWindow(QString Title="", int LBtype=0, int Entrys=0, int FontSize=0, int width=400);
	~eLBWindow();
};

#endif

#ifndef __elbwindow_h
#define __elbwindow_h

#include <core/gui/ewindow.h>
#include <core/gui/elistbox.h>

class eLBWindow: public eWindow
{
protected:
	int Entrys;
	int width;
public:
	eListbox list;
	eLBWindow(eString Title="", int Entrys=0, int FontSize=0, int width=400);
};

inline eLBWindow::eLBWindow(eString Title, int Entrys, int FontSize, int width)
	: eWindow(0), Entrys(Entrys), width(width),	list(this, FontSize)
{
	setText(Title);
	cresize(eSize(width, 10+Entrys*(FontSize+4)));

	list.move(ePoint(10, 5));

	eSize size = getClientSize();
	size.setWidth(size.width()-20);
	size.setHeight(size.height()-10);

	list.resize(size);
}

#endif

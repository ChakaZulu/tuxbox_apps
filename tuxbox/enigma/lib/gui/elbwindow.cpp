#include "elbwindow.h"

eLBWindow::eLBWindow(eString Title, int LBType, int Entrys, int FontSize, int width)
	: eWindow(0), Entrys(Entrys), width(width)
{
	setText(Title);
	cresize(eSize(width, 10+Entrys*(FontSize+4)));

	list=new eListbox(this, LBType, FontSize);
	list->move(ePoint(10, 5));

	eSize size = getClientSize();
	size.setWidth(size.width()-20);
	size.setHeight(size.height()-10);

	list->resize(size);
}

void eLBWindow::OnFontSizeChanged(int NewFontSize)
{
}

eLBWindow::~eLBWindow()
{
}

#include "elbwindow.h"

eLBWindow::eLBWindow(QString Title, int LBType, int Entrys, int FontSize, int width)
	: eWindow(0), Entrys(Entrys), width(width)
{
	setText(Title);
	resize(QSize(width, Entrys*(FontSize+4)));

	list=new eListbox(this, LBType, FontSize);
	list->move(QPoint(0, 0));
	list->resize(getClientSize());
}

void eLBWindow::OnFontSizeChanged(int NewFontSize)
{
}

eLBWindow::~eLBWindow()
{
}

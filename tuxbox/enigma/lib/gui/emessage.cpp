#include "emessage.h"

#include <core/gui/elabel.h>
#include <core/gui/ebutton.h>
#include <core/gui/eskin.h>
#include <core/gdi/font.h>

eMessageBox::eMessageBox(eString message, eString caption, bool display_only): eWindow(0)
{
	int fontsize=eSkin::getActive()->queryValue("fontsize", 20);
	setText(caption);
	move(ePoint(100, 70));
	resize(eSize(520, 430));

	text=new eLabel(this);
	text->setText(message);
	text->move(ePoint(10, 10));
	text->resize( eSize(clientrect.width()-20, clientrect.height() ));
	text->setFlags(RS_WRAP|eLabel::flagVCenter);

	eSize ext=text->getExtend();

// HACK ... the size given by getExtend is okay... but the the renderer sucks....
	ext+=eSize(10,0);
////////////////////////////

	if (ext.width()<150)
		ext.setWidth(150);

	text->resize(ext);

	if (!display_only)
	{
		eButton *b=new eButton(this);
		b->resize(eSize(size.width()-20, fontsize+4));
		b->setText("close");
		eSize bSize=b->getExtend();
		bSize+=eSize(10,10);
		b->resize(bSize);
		resize( eSize( ext.width()+ 20 + size.width() - clientrect.width() , ext.height() + size.height() - clientrect.height() + bSize.height() + 20 ));
		b->move( ePoint( clientrect.width() - ( bSize.width()+10 ), clientrect.height() - ( bSize.height() + 10) ));	// right align
		CONNECT(b->selected, eMessageBox::okPressed);
	}
	else
	{
		resize(eSize(ext.width() + 20 + size.width()-clientrect.width(), ext.height() + size.height() - clientrect.height() + 20 ));
		zOrderRaise();
	}
}

eMessageBox::~eMessageBox()
{
}

void eMessageBox::okPressed()
{
	if ( in_loop )
	  close(0);
	else
		hide();
}

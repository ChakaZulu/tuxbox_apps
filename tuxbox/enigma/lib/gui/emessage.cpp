#include "emessage.h"

#include <core/gui/elabel.h>
#include <core/gui/ebutton.h>
#include <core/gui/eskin.h>
#include <core/gdi/font.h>
#include <core/base/i18n.h>

eMessageBox::eMessageBox(eString message, eString caption, int flags, int def): eWindow(0)
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
	ext+=eSize(10,10);
////////////////////////////

	if (ext.width()<150)
		ext.setWidth(150);

	text->resize(ext);
	
	int numbuttons=0;
			numbuttons++;
	
	int xpos=10;
	
	for (int i=btOK; i<btMax; i<<=1)
		if (flags & i)
		{
			eButton *b=new eButton(this);
			b->resize(eSize(size.width()-20, fontsize+4));
			const char *t="";
			switch (i)
			{
			case btOK: t=_("OK"); CONNECT(b->selected, eMessageBox::pressedOK); break;
			case btCancel: t=_("Cancel"); CONNECT(b->selected, eMessageBox::pressedCancel); break;
			case btYes: t=_("Yes"); CONNECT(b->selected, eMessageBox::pressedYes); break;
			case btNo: t=_("No"); CONNECT(b->selected, eMessageBox::pressedNo); break;
			}
//			b->loadDeco();
			b->setText(t);
			eSize bSize=b->getExtend();
			bSize+=eSize(10,10);
			b->resize(bSize);
			resize( eSize( ext.width() + 20 + size.width() - clientrect.width() , ext.height() + size.height() - clientrect.height() + bSize.height() + 20 ));
			b->move( ePoint(xpos, clientrect.height() - ( bSize.height() + 10) ));	// right align
			
			if (def == i)
				setFocus(b);
			
			xpos += bSize.width()+10;
		}
	
	if (!flags)
	{
		resize(eSize(ext.width() + 20 + size.width() - clientrect.width(), ext.height() + size.height() - clientrect.height() + 20 ));
		zOrderRaise();
	}
}

eMessageBox::~eMessageBox()
{
}

void eMessageBox::pressedOK()
{
	if ( in_loop )
	  close(btOK);
	else
		hide();
}

void eMessageBox::pressedCancel()
{
	if ( in_loop )
	  close(btCancel);
	else
		hide();
}

void eMessageBox::pressedYes()
{
	if ( in_loop )
	  close(btYes);
	else
		hide();
}

void eMessageBox::pressedNo()
{
	if ( in_loop )
	  close(btNo);
	else
		hide();
}

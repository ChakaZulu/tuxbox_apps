#include "listbox.h"

#include <core/gdi/font.h>

gFont eListBoxEntryText::font;
gFont eListBoxEntryTextStream::font;

eListBoxBase::eListBoxBase(eWidget* parent, const eWidget* descr, const char *deco)
:   eDecoWidget(parent, 1, deco),
		iArrowUpDown(eSkin::getActive()->queryImage("eListBox.arrow.updown")),
		iArrowUp(eSkin::getActive()->queryImage("eListBox.arrow.up")),
		iArrowDown(eSkin::getActive()->queryImage("eListBox.arrow.down")),
		iArrowLeft(eSkin::getActive()->queryImage("eListBox.arrow.left")),
		iArrowRight(eSkin::getActive()->queryImage("eListBox.arrow.right")),
		descr(descr),
		tmpDescr(0),
		colorActiveB(eSkin::getActive()->queryScheme("global.selected.background")),
		colorActiveF(eSkin::getActive()->queryScheme("global.selected.foreground")),
		flags(0)
{
}

void eListBoxBase::setFlags(int _flags)	
{		
	flags |= _flags;	
}

void eListBoxBase::removeFlags(int _flags)	
{		
	flags &= ~_flags;	
}

void eListBoxBase::recalcMaxEntries()
{
	if (deco_selected && have_focus)
		MaxEntries = crect_selected.height() / item_height;
	else if (deco)
		MaxEntries = crect.height() / item_height;
	else
		MaxEntries = height() / item_height;
}

eRect eListBoxBase::getEntryRect(int pos)
{
	if ( deco_selected && have_focus )
		return eRect( ePoint( deco_selected.borderLeft, deco_selected.borderTop+pos*item_height ), eSize( crect_selected.width(), item_height ) );
	else if (deco )
		return eRect( ePoint( deco.borderLeft, deco.borderTop+pos*item_height ), eSize( crect.width(), item_height ) );
	else
		return eRect( ePoint(0, pos*item_height), eSize(size.width(), item_height));
}

int eListBoxBase::setProperty(const eString &prop, const eString &value)
{
	if (prop == "noPageMovement")
	{
    if (value == "off")
			flags |= ~flagNoPageMovement;
		else
			flags |= flagNoPageMovement;
	}
	else if (prop == "noUpDownMovement")
	{
    if (value == "off")
			flags |= ~flagNoUpDownMovement;
		else
			flags |= flagNoUpDownMovement;
	}
	else if (prop=="activeForegroundColor")
		colorActiveF=eSkin::getActive()->queryScheme(value);
	else if (prop=="activeBackgroundColor")
		colorActiveB=eSkin::getActive()->queryScheme(value);
	else if (prop=="showEntryHelp")
		setFlags( flagShowEntryHelp );
	else
		return eDecoWidget::setProperty(prop, value);

	return 0;
}

void eListBoxBase::redrawBorder(gPainter *target, eRect& where)
{
	if ( where.contains( eRect(0, 0, width(), height() ) ) )
	{
		if (deco_selected && have_focus)
		{
			deco_selected.drawDecoration(target, ePoint(width(), height()));
			where = crect_selected;
		}
		else if (deco)
		{
			deco.drawDecoration(target, ePoint(width(), height()));
			where = crect;
		}
	}
}

void eListBoxBase::recalcClientRect()
{
		if (deco)
		{
			crect.setLeft( deco.borderLeft );
			crect.setTop( deco.borderTop );
			crect.setRight( width() - 1 - deco.borderRight );
			crect.setBottom( height() - 1 - deco.borderBottom );
		}
		if (deco_selected)
		{
			crect_selected.setLeft( deco_selected.borderLeft );
			crect_selected.setTop( deco_selected.borderTop );
			crect_selected.setRight( width() - 1 - deco_selected.borderRight );
			crect_selected.setBottom( height() - 1 -deco_selected.borderBottom );
		}
}

int eListBoxBase::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::changedSize:
			recalcClientRect();
			recalcMaxEntries();
		break;

		default:
		break;
	}
	return 0;
}

void eListBoxBase::gotFocus()
{
	if (parent && parent->LCDElement)  // detect if LCD Avail
		if (descr)
		{
			parent->LCDElement->setText("");
			LCDTmp = new eLabel(parent->LCDElement);
			LCDTmp->hide();
			eSize s = parent->LCDElement->getSize();
			LCDTmp->move(ePoint(0,s.height()/2));
			LCDTmp->resize(eSize(s.width(), s.height()/2));
			LCDTmp->show();
			tmpDescr = new eLabel(parent->LCDElement);
			tmpDescr->hide();
			tmpDescr->move(ePoint(0,0));
			tmpDescr->resize(eSize(s.width(), s.height()/2));
			tmpDescr->setText( descr->getText() );
			tmpDescr->show();
		}
}

void eListBoxBase::lostFocus()
{
	if ( descr )
	{
		delete LCDTmp;
		LCDTmp=0;
		delete tmpDescr;
		tmpDescr=0;
	}
}

int eListBoxBase::newFocus()
{
	if (deco && deco_selected)
	{
		recalcMaxEntries();

		if (isVisible())
			invalidate();

		return 1;
	}
	return 0;
}

void eListBoxEntry::drawEntryRect( gPainter* rc, const eRect& rect, const gColor& coActiveB, const gColor& coActiveF, const gColor& coNormalB, const gColor& coNormalF, int state )
{	
	if ( (coNormalB != -1 && !state) || (state && coActiveB != -1) )
	{
		rc->setForegroundColor(state?coActiveB:coNormalB);
		rc->fill(rect);
		rc->setBackgroundColor(state?coActiveB:coNormalB);
	}
	else
	{
		eWidget *w=listbox->getNonTransparentBackground();
		rc->setForegroundColor(w->getBackgroundColor());
		rc->fill(rect);
		rc->setBackgroundColor(w->getBackgroundColor());
	}
	rc->setForegroundColor(state?coActiveF:coNormalF);
}

eListBoxEntryText::~eListBoxEntryText()
{
	if (para)
	{
		para->destroy();
		para = 0;
	}
}

int eListBoxEntryText::getEntryHeight()
{
	if ( !font.pointSize)
		font = eSkin::getActive()->queryFont("eListBox.EntryText.normal");

	return calcFontHeight( font ) + 4;
}

int eListBoxEntryTextStream::getEntryHeight()
{
	if ( !font.pointSize)
		font = eSkin::getActive()->queryFont("eListBox.EntryText.normal");

	return calcFontHeight( font ) + 4;
}

int calcFontHeight( const gFont& font)
{
	eTextPara *test;
	test = new eTextPara( eRect(0,0,100,50) );
	test->setFont( font );
	test->renderString("Mjdyl");
	int i =  test->getBoundBox().height();
	test->destroy();
	return i;
}

const eString& eListBoxEntryText::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state)
{
  bool b;

	if ( (b = (state == 2)) )
		state = 0;

	drawEntryRect( rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, state );
	
	if (!para)
	{
		para = new eTextPara( eRect( rect.left(), 0, rect.width(), rect.height() ) );
		para->setFont( font );
		para->renderString(text);
		para->realign(align);
		yOffs = ((rect.height() - para->getBoundBox().height()) / 2 + 0) - para->getBoundBox().top() ;
	}		
	rc->renderPara(*para, ePoint(0, rect.top() + yOffs ) );

	if (b)
	{
		rc->setForegroundColor(coActiveB);
		rc->line( ePoint(rect.left(), rect.bottom()), ePoint(rect.right(), rect.bottom()) );
		rc->line( ePoint(rect.left(), rect.top()), ePoint(rect.right(), rect.top()) );
		rc->line( ePoint(rect.left(), rect.top()), ePoint(rect.left(), rect.bottom()) );
		rc->line( ePoint(rect.right(), rect.top()), ePoint(rect.right(), rect.bottom()) );
		rc->line( ePoint(rect.left()+1, rect.bottom()-1), ePoint(rect.right()-1, rect.bottom()-1) );
		rc->line( ePoint(rect.left()+1, rect.top()+1), ePoint(rect.right()-1, rect.top()+1) );
		rc->line( ePoint(rect.left()+1, rect.top()+2), ePoint(rect.left()+1, rect.bottom()-2) );
		rc->line( ePoint(rect.right()-1, rect.top()+2), ePoint(rect.right()-1, rect.bottom()-2) );
		rc->line( ePoint(rect.left()+2, rect.bottom()-2), ePoint(rect.right()-2, rect.bottom()-2) );
		rc->line( ePoint(rect.left()+2, rect.top()+2), ePoint(rect.right()-2, rect.top()+2) );
		rc->line( ePoint(rect.left()+2, rect.top()+3), ePoint(rect.left()+2, rect.bottom()-3) );
		rc->line( ePoint(rect.right()-2, rect.top()+3), ePoint(rect.right()-2, rect.bottom()-3) );
	}
	return text;
}

eString eListBoxEntryTextStream::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state)
{
	rc->setFont( font );

	drawEntryRect( rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, state );

	rc->setForegroundColor(state?coActiveF:coNormalF);
	rc->renderText(rect, text.str());

	return text.str();
}

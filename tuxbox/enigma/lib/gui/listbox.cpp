#include "listbox.h"

#include <core/gdi/font.h>

eListBoxBase::eListBoxBase(eWidget* parent)
:   eWidget(parent, 1),
		iArrowUpDown(eSkin::getActive()->queryImage("eListBox.arrow.updown")),
		iArrowUp(eSkin::getActive()->queryImage("eListBox.arrow.up")),
		iArrowDown(eSkin::getActive()->queryImage("eListBox.arrow.down")),
		iArrowLeft(eSkin::getActive()->queryImage("eListBox.arrow.left")),
		iArrowRight(eSkin::getActive()->queryImage("eListBox.arrow.right")),
		colorActiveB(eSkin::getActive()->queryScheme("global.selected.background")),
		colorActiveF(eSkin::getActive()->queryScheme("global.selected.foreground")),
		item_height(font.pointSize),   // das suckt !! das ist ein anderer Font, als der für die Entrys genommen wir...
		flags(0)
{
}

void eListBoxBase::setFlags(int _flags)	
{		
	flags=_flags;	

	if (flags & flagLoadDeco)
		loadDeco();
}

void eListBoxBase::recalc()
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

void eListBoxBase::loadDeco()
{
	if (!deco)
		deco.load("eListBox");
	if (!deco_selected)
		deco_selected.load("eListBox.selected");
}

int eListBoxBase::setProperty(const eString &prop, const eString &value)
{
	if (prop=="loadDeco")
	{
		loadDeco();
		event(eWidgetEvent(eWidgetEvent::changedSize));
	}
	else if (prop == "noPageMovement")
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
	else
		return eWidget::setProperty(prop, value);

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

int eListBoxBase::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::changedSize:
			if (deco)
			{
				crect.setLeft( deco.borderLeft );
				crect.setTop( deco.borderTop );
				crect.setRight( width() - deco.borderRight );
				crect.setBottom( height() - deco.borderBottom );
			}
			if (deco_selected)
			{
				crect_selected.setLeft( deco_selected.borderLeft );
				crect_selected.setTop( deco_selected.borderTop );
				crect_selected.setRight( width() - deco_selected.borderRight );
				crect_selected.setBottom( height() - deco_selected.borderBottom );
			}
			recalc();
		break;

		default:
		break;
	}
	return 0;
}

int eListBoxBase::focusChanged()
{
	if (deco && deco_selected)
	{
		recalc();

		if (isVisible())
			invalidate();

		return 1;
	}
	return 0;
}

eListBoxEntryText::~eListBoxEntryText()
{
	if (para)
	{
		para->destroy();
		para = 0;
	}
}

void eListBoxEntryText::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state)
{
	if ((coNormalB != -1 && !state) || (state && coActiveB != -1))
	{
		rc->setForegroundColor(state?coActiveB:coNormalB);
		rc->fill(rect);
		rc->setBackgroundColor(state?coActiveB:coNormalB);
	} else
	{
		eWidget *w=listbox->getNonTransparentBackground();
		rc->setForegroundColor(w->getBackgroundColor());
		rc->fill(rect);
		rc->setBackgroundColor(w->getBackgroundColor());
	}
	rc->setForegroundColor(state?coActiveF:coNormalF);

	if (!para)
	{
		para = new eTextPara( eRect( rect.left(), 0, rect.width(), rect.height() ) );
		para->setFont( font );
		para->renderString(text);
		para->realign(align);

		yOffs = ((rect.height() - para->boundBox.height()) / 2 + 0) - para->boundBox.top() ;
	}		
 	
	rc->renderPara(*para, ePoint(0, rect.top()+yOffs ) );

	eWidget* p = listbox->getParent();			
	if (state && p && p->LCDElement)
		p->LCDElement->setText(text);
}

void eListBoxEntryTextStream::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state)
{
	rc->setFont( font );

	if ((coNormalB != -1 && !state) || (state && coActiveB != -1))
	{
		rc->setForegroundColor(state?coActiveB:coNormalB);
		rc->fill(rect);
		rc->setBackgroundColor(state?coActiveB:coNormalB);
	} else
	{
		eWidget *w=listbox->getNonTransparentBackground();
		rc->setForegroundColor(w->getBackgroundColor());
		rc->fill(rect);
		rc->setBackgroundColor(w->getBackgroundColor());
	}

	rc->setForegroundColor(state?coActiveF:coNormalF);
	rc->renderText(rect, text.str());

	eWidget* p = listbox->getParent();			
	if (state && p && p->LCDElement)
		p->LCDElement->setText(text.str());
}


/*eSize eListBoxEntryText::getTextExtend()	
{
	return para?para->getExtend():eSize();
} */

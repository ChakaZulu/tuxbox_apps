#include "listbox.h"

#include <core/gdi/font.h>

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
	static eString oldtext;

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

	if (text != oldtext || !para)
	{
		oldtext = text;

		if (para)
		{
			para->destroy();
			para = 0;
		}

		para = new eTextPara(rect);
		para->setFont( font );
		para->renderString(text);
		para->realign(align);
	}
 	
	rc->renderPara(*para);

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


eSize eListBoxEntryText::getExtend()	
{
	return para?para->getExtend():eSize();
}

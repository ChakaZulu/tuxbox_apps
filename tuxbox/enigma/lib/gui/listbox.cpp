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

void eListBoxEntryText::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, bool highlited)
{
	static eString oldtext;

	if ((coNormalB != -1 && !highlited) || (highlited && coActiveB != -1))
	{
		rc->setForegroundColor(highlited?coActiveB:coNormalB);
		rc->fill(rect);
		rc->setBackgroundColor(highlited?coActiveB:coNormalB);
	} else
	{
		eWidget *w=listbox->getNonTransparentBackground();
		rc->setForegroundColor(w->getBackgroundColor());
		rc->fill(rect);
		rc->setBackgroundColor(w->getBackgroundColor());
	}
	rc->setForegroundColor(highlited?coActiveF:coNormalF);

	if (text != oldtext || !para)
	{
		oldtext = text;

		if (para)
		{
			para->destroy();
			para = 0;
		}

		para = new eTextPara(rect);
		para->setFont(listbox->getFont());
		para->renderString(text);
		para->realign(align);
	}
 	
	rc->renderPara(*para);

	eWidget* p = listbox->getParent();			
	if (highlited && p && p->LCDElement)
		p->LCDElement->setText(text);
}

void eListBoxEntryTextStream::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, bool highlited)
{
	rc->setFont(listbox->getFont());

	if ((coNormalB != -1 && !highlited) || (highlited && coActiveB != -1))
	{
		rc->setForegroundColor(highlited?coActiveB:coNormalB);
		rc->fill(rect);
		rc->setBackgroundColor(highlited?coActiveB:coNormalB);
	} else
	{
		eWidget *w=listbox->getNonTransparentBackground();
		rc->setForegroundColor(w->getBackgroundColor());
		rc->fill(rect);
		rc->setBackgroundColor(w->getBackgroundColor());
	}

	rc->setForegroundColor(highlited?coActiveF:coNormalF);
	rc->renderText(rect, text.str());

	eWidget* p = listbox->getParent();			
	if (highlited && p && p->LCDElement)
		p->LCDElement->setText(text.str());
}


eSize eListBoxEntryText::getExtend()	
{
	return para?para->getExtend():eSize();
}
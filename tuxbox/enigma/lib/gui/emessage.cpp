#include "enigma.h"
#include "elabel.h"
#include "emessage.h"
#include "ebutton.h"
#include "font.h"
#include "eskin.h"

eMessageBox::eMessageBox(eString message, eString caption): eWindow(0)
{
	int fontsize=eSkin::getActive()->queryValue("fontsize", 20);
	setText(caption);
	move(ePoint(100, 70));
	resize(eSize(520, 430));

	text=new eLabel(this);
	text->setText(message);
	text->move(ePoint(0, 0));
	
	text->resize(eSize(clientrect.width()-20, clientrect.height()));
	text->setFlags(RS_WRAP);

	eSize ext=text->getExtend();
	if (ext.width()<150)
		ext.setWidth(150);
	text->resize(ext); 

	resize(eSize(ext.width()+20+size.width()-clientrect.width(), ext.height()+size.height()-clientrect.height() + fontsize +14));

	eButton *b=new eButton(this);
	b->resize(eSize(size.width()-20, fontsize+4));
	b->setText("...OK!");
	ext=b->getExtend();
	b->resize(ext);
//	b->move(ePoint((clientrect.width()-ext.width())/2, clientrect.height()-fontsize-14));	// center
	b->move(ePoint(clientrect.width()-ext.width(), clientrect.height()-fontsize-14));	// right align
//	connect(b, SIGNAL(selected()), SLOT(okPressed()));
	CONNECT(b->selected, eMessageBox::okPressed);
}

eMessageBox::~eMessageBox()
{
}

void eMessageBox::okPressed()
{
  close(0);
}

#include "echeckbox.h"
#include "font.h"

eCheckbox::eCheckbox(eWidget *parent, int checked=0, int Size, eWidget* descr):
	eButton(parent), descr(descr?descr->getText():"")
{
	setFlags(RS_DIRECT);
	setFont(gFont("Marlett Regular", Size));
	setCheck(checked);
	connect(this, SIGNAL(selected()), SLOT(sel()));
}

eCheckbox::~eCheckbox()
{
}

void eCheckbox::gotFocus()
{
  eButton::gotFocus();

	if (parent && parent->LCDElement)
		parent->LCDElement->setText(descr+'\n'+(ischecked?"[X]":"[  ]"));
}

void eCheckbox::lostFocus()
{
  eButton::lostFocus();

	if (parent && parent->LCDElement)
		parent->LCDElement->setText("");
}

void eCheckbox::sel()
{
	setCheck(ischecked^1);
	emit checked(ischecked);
}

void eCheckbox::setCheck(int c)
{
	ischecked=c;
	setText(ischecked?"\x19":"\x18");

	if (parent && parent->LCDElement)
		parent->LCDElement->setText(descr+'\n'+(ischecked?"[X]":"[  ]"));
}

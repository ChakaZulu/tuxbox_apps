#include "echeckbox.h"
#include "font.h"

eCheckbox::eCheckbox(eWidget *parent, int checked=0, int Size):
	eButton(parent)
{
	setFlags(RS_DIRECT);
	setFont(gFont("Marlett Regular", Size));
	setCheck(checked);
	connect(this, SIGNAL(selected()), SLOT(sel()));
}

eCheckbox::~eCheckbox()
{
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
}

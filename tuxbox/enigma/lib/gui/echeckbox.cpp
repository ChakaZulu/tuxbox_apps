#include "echeckbox.h"
#include "font.h"
#include "init.h"
#include "eskin.h"

eCheckbox::eCheckbox(eWidget *parent, int checked=0, int Size, eLabel* descr):
	eButton(parent, descr)
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

static eWidget *create_eCheckbox(eWidget *parent)
{
	return new eCheckbox(parent);
}

class eCheckboxSkinInit
{
public:
	eCheckboxSkinInit()
	{
		eSkin::addWidgetCreator("eCheckbox", create_eCheckbox);
	}
	~eCheckboxSkinInit()
	{
		eSkin::removeWidgetCreator("eCheckbox", create_eCheckbox);
	}
};

eAutoInitP0<eCheckboxSkinInit> init_eCheckboxSkinInit(3, "eCheckbox");

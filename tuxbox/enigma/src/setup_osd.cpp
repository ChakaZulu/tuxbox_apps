#include <netinet/in.h>
#include "setup_osd.h"
#include "elabel.h"
#include "edvb.h"
#include "echeckbox.h"
#include "rc.h"
#include "enigma.h"

eZapOsdSetup::eZapOsdSetup(): eWindow(0)
{
	setText("OSD Setup");
	move(ePoint(150, 136));
	resize(eSize(300, 200));

	int	useBigOSD=0;
	eDVB::getInstance()->config.getKey("/ezap/osd/useBigOSD", useBigOSD);

	int useBigFonts=0;
	eDVB::getInstance()->config.getKey("/ezap/osd/useBigFonts", useBigFonts);

	labelOsd=new eLabel(this);
	labelOsd->setText("Big OSD");
	labelOsd->move(ePoint(60, 15));
	labelOsd->resize(eSize(100, eZap::FontSize+4));

	bigosd=new eCheckbox(this, useBigOSD);
	bigosd->move(ePoint(20, 18));
	bigosd->resize(eSize(eZap::FontSize+4, eZap::FontSize+4));

	bigfonts=new eCheckbox(this, useBigFonts);
	bigfonts->move(ePoint(20, 48));
	bigfonts->resize(eSize(eZap::FontSize+4, eZap::FontSize+4));

	ok=new eButton(this);
	ok->setText("[SAVE]");
	ok->move(ePoint(20, 100));
	ok->resize(eSize(90, eZap::FontSize+4));
	
//	connect(ok, SIGNAL(selected()), SLOT(okPressed()));
	CONNECT(ok->selected, eZapOsdSetup::okPressed);

	abort=new eButton(this);
	abort->setText("[ABORT]");
	abort->move(ePoint(140, 100));
	abort->resize(eSize(100, eZap::FontSize+4));

//	connect(ok, SIGNAL(selected()), SLOT(okPressed()));
	CONNECT(ok->selected, eZapOsdSetup::okPressed);

	labelFonts=new eLabel(this);
	labelFonts->setText("Big Fonts");
	labelFonts->move(ePoint(60, 45));
	labelFonts->resize(eSize(110, eZap::FontSize+4));
}

eZapOsdSetup::~eZapOsdSetup()
{
}

void eZapOsdSetup::fieldSelected(int *number)
{
	focusNext();
}

void eZapOsdSetup::okPressed()
{
	eDVB::getInstance()->config.setKey("/ezap/osd/useBigOSD", bigosd->isChecked());
	eDVB::getInstance()->config.setKey("/ezap/osd/useBigFonts", bigfonts->isChecked());
	eZap::getInstance()->useBigFonts=bigfonts->isChecked();
	eZap::getInstance()->switchFontSize();
	close(1);
}

void eZapOsdSetup::abortPressed()
{
	close(0);
}

int eZapOsdSetup::eventFilter(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::keyDown:
		switch(event.parameter)
		{
		case eRCInput::RC_DOWN:
			focusNext(0);
			return 1;
			break;
		case eRCInput::RC_UP:
			focusNext(1);
			return 1;
			break;
		}
	}
	return 0;
}

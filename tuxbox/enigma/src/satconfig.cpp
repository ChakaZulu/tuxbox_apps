#include "satconfig.h"

#include <core/gui/eskin.h>
#include <core/gui/ebutton.h>
#include <core/driver/rc.h>

eSatelliteConfigurationManager::eSatelliteConfigurationManager()
{
	button_close=new eButton(this);
	button_close->setName("close");
	CONNECT(button_close->selected, eSatelliteConfigurationManager::okPressed);

	w_buttons=new eWidget(this);
	w_buttons->setName("buttons");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eSatelliteConfigurationManager"))
		eFatal("skin load of \"eSatelliteConfigurationManager\" failed");
	
  int lnb=0;

	int x = 0;
	int y = 0;

	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
		{
			eButton* b = new eButton(w_buttons);
			b->setText(eString().sprintf("%i", lnb) );
			b->move(ePoint(x,y));
			b->resize(eSize(100, 30));
			CONNECT(b->selected_id, eSatelliteConfigurationManager::lnbSelected);
			y+=40;
			lnb++;
		}
}

eSatelliteConfigurationManager::~eSatelliteConfigurationManager()
{
}

bool eSatelliteConfigurationManager::lnbSelected(eString& descr)
{
/*	int lnbnumber;
	if (descr == _("none") )
		lnbnumber = -1;
	else
		lnbnumber = atoi(descr.c_str());
	
	int oldlnb = lnbnumber;

	eLNBSelitor sel;
	
	sel.setCurrentLNB(lnbnumber);

	if ( !sel.exec() )
	{
		if ( (lnbnumber = sel.getCurrentLNB()) != oldlnb)
		{
			descr.sprintf("%i", lnbnumber);
			return true;
		}
	}*/
	return false;
}


void eSatelliteConfigurationManager::okPressed()
{
	close(0);
}

int eSatelliteConfigurationManager::eventFilter(const eWidgetEvent &event)
{
#if 0
	switch (event.type)
	{
	case eWidgetEvent::keyDown:
		switch(event.parameter)
		{
		case eRCInput::RC_RIGHT:
			focusNext(eWidget::focusDirE);
			return 1;
		case eRCInput::RC_DOWN:
			focusNext(eWidget::focusDirS);
			return 1;
		case eRCInput::RC_LEFT:
			focusNext(eWidget::focusDirW);
			return 1;
		case eRCInput::RC_UP:
			focusNext(eWidget::focusDirN);
			return 1;
		}
	}
#endif
	return 0;
}

#include "satconfig.h"

#include <core/gui/eskin.h>
#include <core/gui/ebutton.h>
#include <core/driver/rc.h>
#include <core/gui/emessage.h>

eSatelliteConfigurationManager::eSatelliteConfigurationManager()
{

	eMessageBox b("Satconfig isn't ready, it has NO function yet", "under construction !!");
	b.show();
	b.exec();
	b.hide();

	button_close=new eButton(this);
	button_close->setName("close");
	CONNECT(button_close->selected, eSatelliteConfigurationManager::okPressed);

	w_buttons=new eWidget(this);
	w_buttons->setName("buttons");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eSatelliteConfigurationManager"))
		eFatal("skin load of \"eSatelliteConfigurationManager\" failed");

	int sx = 90, sy=0;	
  int lnb=0, lx = 380, ly = 0;
	int dx = 0, dy = 0;
	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
	{
		int sat=0;
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
		{
			eButton* b = new eButton(w_buttons);
			b->move(ePoint(sx,sy));
			b->resize(eSize(280, 30));
			b->setText( s->getDescription() );
			CONNECT(b->selected_id, eSatelliteConfigurationManager::satSelected);
			sy+=40;
			sat++;
  	}
		while (sat--)
		{
			eButton* b = new eButton(w_buttons);
			b->move(ePoint(lx,ly));
			b->resize(eSize(80, 30));

			b->setText(eString().sprintf("%i", lnb++) );
			CONNECT(b->selected_id, eSatelliteConfigurationManager::lnbSelected);
			ly+=40;

			b = new eButton(w_buttons);
			b->move(ePoint(dx,dy));
			b->resize(eSize(80, 30));
			CONNECT(b->selected_id, eSatelliteConfigurationManager::DISEqCSelected);
			switch ( it->getDiSEqC().sat )
			{
			case 0: // A/A
				b->setText("A/A");	
				break;
			case 1: // A/B
				b->setText("A/B");	
				break;
			case 2: // B/A
				b->setText("B/A");	
				break;
			case 3: // B/B
				b->setText("B/B");	
				break;
			case 4:	// user
				b->setText("User");	
				break;
			default:
				eDebug("diseq.sat = %i", it->getDiSEqC().sat);
			}
			dy+=40;
		}
	}
}

eSatelliteConfigurationManager::~eSatelliteConfigurationManager()
{
}

bool eSatelliteConfigurationManager::lnbSelected(eString& descr)
{
	int lnbnumber;

	if (descr == _("none") )
		lnbnumber = -1;
	else
		lnbnumber = atoi(descr.c_str());
	
	int oldlnb = lnbnumber;

	eLNBSelitor sel;
	
	sel.setCurrentLNB(lnbnumber);
	hide();
	sel.show();
	if ( !sel.exec() )
	{
		if ( (lnbnumber = sel.getCurrentLNB()) != oldlnb)
		{
			descr.sprintf("%i", lnbnumber);
			return true;
		}
	}
	sel.hide();
	show();
	return false;
}

bool eSatelliteConfigurationManager::satSelected(eString& descr)
{
	return false;
}

bool eSatelliteConfigurationManager::DISEqCSelected(eString& descr)
{
	return false;
}

void eSatelliteConfigurationManager::okPressed()
{
	close(0);
}

int eSatelliteConfigurationManager::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
		if (event.action == &i_cursorActions->left)
			focusNext(eWidget::focusDirW);
		else if (event.action == &i_cursorActions->right)
			focusNext(eWidget::focusDirE);
		else if (event.action == &i_cursorActions->up)
			focusNext(eWidget::focusDirN);
		else if (event.action == &i_cursorActions->down)
			focusNext(eWidget::focusDirS);
		else
			break;
		return 1;

	default:
		break;
	}
	return eWindow::eventHandler(event);
}

eLNBSelitor::eLNBSelitor()
{
	int init_l[5]={0,9,7,5,0};
	int init_h[5]={1,0,6,0,0};
	int init_t[5]={1,1,7,0,0};
	lnb_list = new eListBox<eListBoxEntryText>(this);
	lnb_list->setFlags( eListBoxBase::flagNoPageMovement );
	lnb_list->setName("lnblist");
	lofH = new eNumber(this, 5, 0, 9, 1, init_h, 0, 0, 1 );  // todo descr label im skin mit name versehen für lcd anzeige
	lofH->setName("lofH");
	lofL = new eNumber(this, 5, 0, 9, 1, init_l, 0, 0, 1 );  // todo descr label im skin mit name versehen für lcd anzeige
	lofL->setName("lofL");
	threshold = new eNumber(this, 5, 0 ,9, 1, init_t, 0, 0, 1);
	threshold->setName("threshold");
	use = new eButton(this);
	use->setName("use");
	apply = new eButton(this);
	apply->setName("apply");
	remove = new eButton(this);
	remove->setName("delete");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eLNBSelitor"))
		eFatal("skin load of \"eLNBSelitor\" failed");

	// add all LNBs

	int i=0;
	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		new eListBoxEntryText(lnb_list, eString().sprintf("LNB %i", i++), (void*)&(*it) );

	// add a None LNB
	new eListBoxEntryText(lnb_list, _("New"), 0);
}

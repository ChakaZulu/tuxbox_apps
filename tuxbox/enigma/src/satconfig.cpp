#include "satconfig.h"

#include <core/gui/eskin.h>
#include <core/gui/ebutton.h>
#include <core/driver/rc.h>
#include <core/gui/emessage.h>

eSatelliteConfigurationManager::eSatelliteConfigurationManager()
	:refresh(0)
{
/*	eMessageBox b("Satconfig isn't ready, it has NO function yet", "under construction !!");
	b.show();
	b.exec();
	b.hide();*/

	button_close=new eButton(this);
	button_close->setName("close");
	CONNECT(button_close->selected, eSatelliteConfigurationManager::closePressed);

	w_buttons=new eWidget(this);
	w_buttons->setName("buttons");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eSatelliteConfigurationManager"))
		eFatal("skin load of \"eSatelliteConfigurationManager\" failed");

	parseNetworks();  // load all networks from satellite.xml or cable.xml
	redrawButtons();
}

eSatelliteConfigurationManager::~eSatelliteConfigurationManager()
{
	if (refresh)
		delete refresh;
}


void eSatelliteConfigurationManager::redrawButtons()
{
	if ( sat.size() )
		for ( std::map< eComboBox*, std::pair< int, eSatellite* > >::iterator it ( sat.begin() ); it != sat.end(); it++)
			delete it->first;

	if ( lnb.size() )
		for ( std::map< eButton*, std::pair< int, eLNB* > >::iterator it ( lnb.begin() ); it != lnb.end(); it++)
			delete it->first;

	if ( hilo.size() )
		for ( std::map< eComboBox*, eSwitchParameter::SIG22* >::iterator it ( hilo.begin() ); it != hilo.end(); it++)
			delete it->first;

	if ( voltage.size() )
		for ( std::map< eComboBox*, eSwitchParameter::VMODE* >::iterator it ( voltage.begin() ); it != voltage.end(); it++)
			delete it->first;

	lnb.clear();
	sat.clear();
	hilo.clear();
	voltage.clear();

	int sx = 0, sy=0, lnbcount=0, hx=270, vx=370, lx = 200, ly = 0;
	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
	{
		int satcount=0;
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
		{
			eComboBox* c = new eComboBox(w_buttons, 10);
			sat.insert ( std::pair< eComboBox*, std::pair< int, eSatellite*> >( c, std::pair< int, eSatellite* >( satcount, *s ) ) );
			c->loadDeco();
			c->move(ePoint(sx,sy));
			c->resize(eSize(190, 30));
			c->setHelpText( _("press ok to select another satellite"));
  		for (std::list<tpPacket>::const_iterator i(networks.begin()); i != networks.end(); ++i)
				if ( i->possibleTransponders.size() )
					new eListBoxEntryText( *c, i->name, (void*) i->possibleTransponders.begin()->satellite.orbital_position );

			int err;
			if ( (err =	c->setCurrent( (void*) s->getOrbitalPosition() ) ) )
				if (err == eComboBox::E_COULDNT_FIND)  // hmm current entry not in Combobox... we add manually
					c->setCurrent( new eListBoxEntryText( *c, s->getDescription(), (void*) s->getOrbitalPosition() ) );
			eDebug("Sat Error = %i", err);

			CONNECT( c->selchanged_id, eSatelliteConfigurationManager::satChanged );
			satcount++;

			c = new eComboBox(w_buttons,3);
			hilo.insert ( std::pair< eComboBox*, eSwitchParameter::SIG22* > ( c, &s->getSwitchParams().HiLoSignal ) );
			c->loadDeco();
			c->move( ePoint( hx, sy ) );
			c->resize( eSize( 90, 30 ) );
			c->setHelpText( _("press ok to select another 22Khz mode") );
			new eListBoxEntryText( *c, "Auto", (void*)eSwitchParameter::HILO, 0, _("22khz signal is automaticaly switched") );
			new eListBoxEntryText( *c, "Hi", (void*)eSwitchParameter::ON, 0, _("22Khz is always enabled") );
			new eListBoxEntryText( *c, "Lo", (void*)eSwitchParameter::OFF, 0, _("22khz is always disabled") );
			CONNECT( c->selchanged_id, eSatelliteConfigurationManager::hiloChanged);
			eDebug( "HiLo Error = %i", c->setCurrent( (void*) (int) s->getSwitchParams().HiLoSignal ) );

			c = new eComboBox(w_buttons,3);
			voltage.insert ( std::pair< eComboBox*, eSwitchParameter::VMODE* > ( c, &s->getSwitchParams().VoltageMode ) );
			c->loadDeco();
			c->move( ePoint( vx, sy ) );
			c->resize( eSize( 90, 30 ) );
			c->setHelpText( _("press ok to select another LNB Voltage mode") );
			new eListBoxEntryText( *c, "Auto", (void*)eSwitchParameter::HV, 0, _("Voltage is automaticaly changed") );
			new eListBoxEntryText( *c, "14V", (void*)eSwitchParameter::_14V, 0, _("Voltage is always 14V (horizontal)") );
			new eListBoxEntryText( *c, "18V", (void*)eSwitchParameter::_18V, 0, _("Voltage is always 18V (vertical") );
			CONNECT( c->selchanged_id, eSatelliteConfigurationManager::voltageChanged);
			eDebug("Voltage Error = %i", c->setCurrent( (void*) (int) s->getSwitchParams().VoltageMode ) );

			sy+=40;
  	}
		while (satcount--)
		{
			eButton* b = new eButton(w_buttons);
			lnb.insert ( std::pair<eButton*, std::pair< int, eLNB*> >( b, std::pair< int, eLNB* >( lnbcount, &*it ) ) );
			b->loadDeco();
			b->move(ePoint(lx,ly));
			b->resize(eSize(60, 30));
			b->setText(eString().sprintf("%i", lnbcount) );
			b->setHelpText( _("press ok to goto LNB config"));
			CONNECT(b->selected_id, eSatelliteConfigurationManager::lnbSelected);
			ly+=40;
		}
		lnbcount++;
	}

	if (refresh)
		show();
}

void eSatelliteConfigurationManager::lnbSelected(eButton* who)
{
	eLNBSelitor sel;
	std::map< eComboBox*, std::pair< int, eSatellite* > >::iterator it = sat.begin();
	int i = lnb[ (eButton*)who ].first;
	while (i--)
		it++;
	sel.setLCD(LCDTitle, LCDElement);
	sel.setData( lnb[(eButton*)who].second, it->second.second );
	hide();

	if ( !sel.exec() )
	{    // if eLnbSelitor is closed with return value 0 then buttons must be deleted and new created
		if (!refresh)
		{
			refresh=new eTimer( eApp );
			CONNECT( refresh->timeout, eSatelliteConfigurationManager::redrawButtons );
		}
		sel.hide();  // hide window
		refresh->start(50, true);  // start the timer
		// show is called when the timer calls redrawButtons....
	}
	else
	{
		sel.hide();   // hide eLnbSelitor
		show();				// show eSatelliteConfigurationManager
	}

	return;
}

void eSatelliteConfigurationManager::satChanged(eComboBox* who, eListBoxEntryText *le)
{
	sat[ who ].second->setOrbitalPosition( (int) le->getKey() );
	sat[ who ].second->setDescription( le->getText() );
}

void eSatelliteConfigurationManager::hiloChanged(eComboBox* who, eListBoxEntryText *le)
{
	 *hilo[ who ] = (eSwitchParameter::SIG22) (int) le->getKey();
}

void eSatelliteConfigurationManager::voltageChanged(eComboBox* who, eListBoxEntryText *le)
{
	 *voltage[ who ] = (eSwitchParameter::VMODE) (int) le->getKey();
}

void eSatelliteConfigurationManager::closePressed()
{
	close(0);
}

int eSatelliteConfigurationManager::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
		if (event.action == &i_focusActions->left)
			focusNext(eWidget::focusDirW);
		else if (event.action == &i_focusActions->right)
			focusNext(eWidget::focusDirE);
		else if (event.action == &i_focusActions->up)
			focusNext(eWidget::focusDirN);
		else if (event.action == &i_focusActions->down)
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
	lnb_list = new eListBox<eListBoxEntryText>(this);
	lnb_list->setFlags( eListBoxBase::flagNoPageMovement );
	lnb_list->setName("lnblist");
	eLabel *l = new eLabel(this);
	l->setName("lLofL");
	lofL = new eNumber(this, 5, 0, 9, 1, 0, 0, l, 1 );  // todo descr label im skin mit name versehen für lcd anzeige
	lofL->setName("lofL");
	l = new eLabel(this);
	l->setName("lLofH");
	lofH = new eNumber(this, 5, 0, 9, 1, 0, 0, l, 1 );  // todo descr label im skin mit name versehen für lcd anzeige
	lofH->setName("lofH");
	l = new eLabel(this);
	l->setName("lThreshold");
	threshold = new eNumber(this, 5, 0 ,9, 1, 0, 0, l, 1);
	threshold->setName("threshold");
	diseqcMode = new eComboBox( this, 3 );
	diseqcMode->setName("DISEqCMode");
	// *diseqcMode... the we use the operator eListBox* from eComboBox !
	new eListBoxEntryText( *diseqcMode, "Mini-DISEqC", (void*)eDISEqC::MINI, 0, _("Use Mini DISEqC Mode (for simple switches)") );
	new eListBoxEntryText( *diseqcMode, "Version 1.0", (void*)eDISEqC::V1_0, 0, _("Use DISEqC Version 1.0") );
	new eListBoxEntryText( *diseqcMode, "Version 1.1", (void*)eDISEqC::V1_1, 0, _("Use DISEqC Version 1.1") );
	diseqcParam = new eComboBox( this, 4 );
	diseqcParam->setName("DISEqCParam");

	save = new eButton(this);
	save->setName("save");
	cancel = new eButton(this);
	cancel->setName("cancel");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eLNBSelitor"))
		eFatal("skin load of \"eLNBSelitor\" failed");

	// add all LNBs

	int i=0;
	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		new eListBoxEntryText(lnb_list, eString().sprintf("LNB %i", i++), (void*)&(*it) );
	
	// add a None LNB
	new eListBoxEntryText(lnb_list, _("New"), (void*) 0 );

	CONNECT( lofL->selected, eLNBSelitor::numSelected);
	CONNECT( lofH->selected, eLNBSelitor::numSelected);
	CONNECT( threshold->selected, eLNBSelitor::numSelected);
	CONNECT( save->selected, eLNBSelitor::onSave);
	CONNECT( cancel->selected, eLNBSelitor::reject);
	CONNECT( lnb_list->selchanged, eLNBSelitor::lnbChanged);
	CONNECT( lnb_list->selected, eLNBSelitor::lnbSelected);
	CONNECT( diseqcMode->selchanged, eLNBSelitor::diseqcModeChanged);
}

void eLNBSelitor::lnbSelected( eListBoxEntryText*)
{
	focusNext( eWidget::focusDirNext );
}

void eLNBSelitor::lnbChanged( eListBoxEntryText *lnb )
{
	int l1 = 9750000;
	int l2 = 10600000;
	int l3 = 11700000;
	if ( lnb && lnb->getKey() )
	{
		l1 = ((eLNB*)lnb->getKey())->getLOFLo();
		l2 = ((eLNB*)lnb->getKey())->getLOFHi();
		l3 = ((eLNB*)lnb->getKey())->getLOFThreshold();
	}
	lofL->setNumber( l1 / 1000 );
	lofL->invalidate();
	lofH->setNumber( l2 / 1000 );
	lofH->invalidate();
	threshold->setNumber( l3 / 1000 );
	threshold->invalidate();
	if (lnb->getKey())
	{
		diseqcMode->setCurrent( (void*) (int) ((eLNB*)lnb->getKey())->getDISEqC().DISEqCMode );
		diseqcModeChanged( diseqcMode->getCurrent() );  // fake selchanged for initialize diseqcParam ComboBox...	
	}
}

struct eLNBSelitor::selectlnb: public std::unary_function<const eListBoxEntryText&, void>
{
	const eLNB *lnb;
	eListBox<eListBoxEntryText> *lb;

	selectlnb(const eLNB* lnb, eListBox<eListBoxEntryText>* lb): lnb(lnb), lb(lb)
	{
	}

	bool operator()(const eListBoxEntryText& s)
	{
		if ( lnb == (eLNB*)s.getKey() )
		{
	 		lb->setCurrent(&s);
			return 1;
		}
		return 0;
	}
};

void eLNBSelitor::setData( eLNB* lnb, eSatellite* sat)
{
	this->lnb = lnb;
	this->sat = sat;
	lnb_list->forEachEntry(	selectlnb( lnb, lnb_list ) );
}

void eLNBSelitor::onSave()
{
	eLNB *p = (eLNB*) lnb_list->getCurrent()->getKey();

	if ( !p )  // then we must create new LNB;
	{
    eTransponderList::getInstance()->getLNBs().push_back( eLNB( *eTransponderList::getInstance() ) );  // add new LNB
		p = &eTransponderList::getInstance()->getLNBs().back();   // get adresse from the new lnb
	}

	p->setLOFLo( lofL->getNumber() * 1000 );
	p->setLOFHi( lofH->getNumber() * 1000 );
	p->setLOFThreshold( threshold->getNumber() * 1000 );
	p->getDISEqC().DISEqCMode = (eDISEqC::tDISEqCMode) (int) diseqcMode->getCurrent()->getKey();
	p->getDISEqC().DISEqCParam = (eDISEqC::tDISEqCParam) (int) diseqcParam->getCurrent()->getKey();

	if ( p != lnb )  // the satellite must removed from the old lnb and inserts in the new
	{
		p->addSatellite( lnb->takeSatellite( sat ) );
		eDebug("remove satellite from lnb... now %i satellites left", lnb->getSatelliteList().size() );
		eDebug("added satellite to lnb... now %i satellites in lnb", p->getSatelliteList().size() );		

		if ( !lnb->getSatelliteList().size() )   // the the lnb that have no more satellites must delete
		{
			eDebug("delete no more used lnb");
		  eTransponderList::getInstance()->getLNBs().remove( *lnb );
		}
		else
			eDebug("lnb not deleted");

		close(0);
	}
	else
		close(-1);
}

void eLNBSelitor::numSelected(int*)
{
	focusNext( eWidget::focusDirNext );
}

void eLNBSelitor::diseqcModeChanged ( eListBoxEntryText* le)
{
	if ( diseqcParam->getCount() )  // then still elements in the ComboBox... we remove it...
		diseqcParam->clear();

	if ( (eDISEqC::tDISEqCMode) (int) le->getKey() == eDISEqC::MINI )
	{
		new eListBoxEntryText( *diseqcParam, "A", (void*)eDISEqC::AA, 0, _("sends no MiniDISEqC burst") );
		new eListBoxEntryText( *diseqcParam, "B", (void*)eDISEqC::AB, 0, _("sends MiniDISEqC burst") );
	}
	else // diseqc 1.0 or 1.1
	{
		new eListBoxEntryText( *diseqcParam, "A/A", (void*)eDISEqC::AA, 0, _("sends DISEqC cmd A/A") );
		new eListBoxEntryText( *diseqcParam, "A/B", (void*)eDISEqC::AB, 0, _("sends DISEqC cmd A/B") );
		new eListBoxEntryText( *diseqcParam, "B/A", (void*)eDISEqC::AA, 0, _("sends DISEqC cmd B/A") );
		new eListBoxEntryText( *diseqcParam, "B/B", (void*)eDISEqC::AB, 0, _("sends DISEqC cmd B/B") );
	}
	diseqcParam->setCurrent( (void*) ((eLNB*)lnb_list->getCurrent()->getKey())->getDISEqC().DISEqCParam );
}

int eLNBSelitor::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
/*	case eWidgetEvent::evtAction:
			if (focus != lofL && focus != lofH && focus != threshold && event.action == &i_focusActions->left)
				focusNext(eWidget::focusDirW);
			else if (focus != lofL && focus != lofH && focus != threshold && event.action == &i_focusActions->right)
				focusNext(eWidget::focusDirE);
			else if (focus != lnb_list && event.action == &i_focusActions->up)
				focusNext(eWidget::focusDirN);
			else if (focus != lnb_list && event.action == &i_focusActions->down)
				focusNext(eWidget::focusDirS);
			else
				break;
			return 1;*/
	
	case eWidgetEvent::execBegin:
		lnbChanged( lnb_list->getCurrent() );   // fake selchanged for initialize lofl, lofh, threshold...
		diseqcModeChanged( diseqcMode->getCurrent() );  // fake selchanged for initialize diseqcParam ComboBox...
		show();
	break;

	default:
		break;
	}
	return eWindow::eventHandler(event);
}

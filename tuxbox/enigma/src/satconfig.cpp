#include <satconfig.h>

#include <lib/base/i18n.h>
#include <lib/gui/eskin.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/emessage.h>
//#include <lib/driver/rc.h>

eSatelliteConfigurationManager::eSatelliteConfigurationManager()
	:refresh(0), deleteThisEntry( 0 )
{
	button_close=new eButton(this);
	button_close->setName("close");
	CONNECT(button_close->selected, eSatelliteConfigurationManager::closePressed);

	button_new=new eButton(this);
	button_new->setName("new");
	CONNECT(button_new->selected, eSatelliteConfigurationManager::newPressed);

	w_buttons=new eWidget(this);
	w_buttons->setName("buttons");
	w_buttons->setLCD( LCDTitle, LCDElement );

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eSatelliteConfigurationManager"))
		eFatal("skin load of \"eSatelliteConfigurationManager\" failed");

	parseNetworks();  // load all networks from satellite.xml or cable.xml
	createControlElements();
	repositionWidgets();
}

eSatelliteConfigurationManager::~eSatelliteConfigurationManager()
{
	if (refresh)
		delete refresh;
}

eSatellite *eSatelliteConfigurationManager::getSat4SatCombo( const eComboBox *c )
{
	std::map<eSatellite*, SatelliteEntry>::iterator it ( entryMap.begin() );
	for ( ; it != entryMap.end(); it++)
 		if ( it->second.sat == c)
			break;
	return it->first;
}

eSatellite *eSatelliteConfigurationManager::getSat4HiLoCombo( const eComboBox *c )
{
	std::map<eSatellite*, SatelliteEntry>::iterator it ( entryMap.begin() );
	for ( ; it != entryMap.end(); it++)
 		if ( it->second.hilo == c)
			break;
	return it->first;
}

eSatellite *eSatelliteConfigurationManager::getSat4VoltageCombo( const eComboBox *c )
{
	std::map<eSatellite*, SatelliteEntry>::iterator it ( entryMap.begin() );
	for ( ; it != entryMap.end(); it++)
 		if ( it->second.voltage == c)
			break;
	return it->first;
}

eSatellite *eSatelliteConfigurationManager::getSat4LnbButton( const eButton *b )
{
	std::map<eSatellite*, SatelliteEntry>::iterator it ( entryMap.begin() );
	for ( ; it != entryMap.end(); it++)
 		if ( it->second.lnb == b)
			break;
	return it->first;
}


#define SAT_POS_X  0
#define LNB_POS_X  200
#define HILO_POS_X  270
#define VOLTAGE_POS_X  370
#define POS_Y 0

void eSatelliteConfigurationManager::repositionWidgets()
{
	if (deleteThisEntry)
	{
		delete deleteThisEntry->sat;
		delete deleteThisEntry->lnb;
		delete deleteThisEntry->voltage;
		delete deleteThisEntry->hilo;
		// search Entry in Map;		
		std::map< eSatellite*, SatelliteEntry >::iterator it( entryMap.begin() );
		for ( ; it != entryMap.end() && &it->second != deleteThisEntry ; it++);
		if (it != entryMap.end() )
			entryMap.erase( it );		
		deleteThisEntry=0;
		
	}
	int sx=SAT_POS_X, y=POS_Y, hx=HILO_POS_X, vx=VOLTAGE_POS_X, lx=LNB_POS_X, count=0;

	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
	{
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
		{
			SatelliteEntry& entry = entryMap[ *s ];
			// search eComboBox for this eSatellite and move
			if (isVisible())
			{
				entry.sat->hide();
				entry.lnb->hide();
				entry.voltage->hide();
				entry.hilo->hide();
			}
    	entry.sat->move( ePoint(sx, y) );
			entry.lnb->move( ePoint(lx, y) );
			entry.lnb->setText( eString().sprintf("%i", count) );
			entry.hilo->move( ePoint(hx, y) );
			entry.voltage->move( ePoint(vx, y) );

			if (isVisible())
			{
				entry.sat->show();
				entry.lnb->show();
				entry.voltage->show();
				entry.hilo->show();
			}
			y+=40;
  	}
		count++;
	}
}


void eSatelliteConfigurationManager::createControlElements()
{
	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
  		addSatellite( *s );
}

void eSatelliteConfigurationManager::lnbSelected(eButton* who)
{
	eLNBSelitor sel( getSat4LnbButton(who) );
	sel.setLCD(LCDTitle, LCDElement);

	hide();
	sel.show();
	
	if ( sel.exec() )  // the we must reposition widgets
		repositionWidgets();

	sel.hide();
	show();

	return;
}


void eSatelliteConfigurationManager::satChanged(eComboBox* who, eListBoxEntryText *le)
{
	eSatellite *s = getSat4SatCombo(who);
	if ( le->getKey() && le->getText() )
	{
		s->setOrbitalPosition( (int) le->getKey() );
		s->setDescription( le->getText() );
	}
	else  // *delete* selected -->> satellite and empty lnbs were now deleted
	{
		eLNB* lnb = s->getLNB();
		lnb->deleteSatellite( s );
		eDebug("Satellite is now removed");
		if ( !lnb->getSatelliteList().size() )   // the lnb that have no more satellites must be deleted
		{
			eDebug("delete no more used lnb");
			eTransponderList::getInstance()->getLNBs().remove( *s->getLNB() );
		}
		else
	    eDebug("do not delete lnb");		

		deleteThisEntry = &entryMap[ s ];
		if (!refresh)
		{
			refresh = new eTimer( eApp );
			CONNECT( refresh->timeout, eSatelliteConfigurationManager::repositionWidgets );
		}
		refresh->start(50, true );
	}
}

void eSatelliteConfigurationManager::hiloChanged(eComboBox* who, eListBoxEntryText *le)
{
	 getSat4HiLoCombo(who)->getSwitchParams().HiLoSignal = (eSwitchParameter::SIG22) (int) le->getKey();
}

void eSatelliteConfigurationManager::voltageChanged(eComboBox* who, eListBoxEntryText *le)
{
	getSat4VoltageCombo(who)->getSwitchParams().VoltageMode = (eSwitchParameter::VMODE) (int) le->getKey();
}

void eSatelliteConfigurationManager::closePressed()
{
	close(0);
}

void eSatelliteConfigurationManager::addSatellite( eSatellite *s )
{
	SatelliteEntry sat;

	eLabel *l = new eLabel(this);
	l->setName("lSatPos");
	eComboBox* c = new eComboBox(w_buttons, 6, l);
	sat.sat=c;
	c->loadDeco();
//			c->move(ePoint(sx,y));
	c->resize(eSize(190, 30));
	c->setHelpText( _("press ok to select another satellite, or delete this satellite"));

	new eListBoxEntryText( *c, _("*delete*"), (void*) 0 );   // this is to delete an satellite
	for (std::list<tpPacket>::const_iterator i(networks.begin()); i != networks.end(); ++i)
		if ( i->possibleTransponders.size() )
			new eListBoxEntryText( *c, i->name, (void*) i->possibleTransponders.begin()->satellite.orbital_position );

	int err;
	if ( (err = c->setCurrent( (void*) s->getOrbitalPosition() ) ) )
		if (err == eComboBox::E_COULDNT_FIND)  // hmm current entry not in Combobox... we add manually
			c->setCurrent( new eListBoxEntryText( *c, s->getDescription(), (void*) s->getOrbitalPosition() ) );
	CONNECT( c->selchanged_id, eSatelliteConfigurationManager::satChanged );

	l = new eLabel(this);
	l->setName("lLnb");
	eButton* b = new eButton(w_buttons, l);
	sat.lnb=b;
	b->loadDeco();
//			b->move(ePoint(lx, y));
	b->resize(eSize(60, 30));
	b->setHelpText( _("press ok to goto LNB config"));
	CONNECT(b->selected_id, eSatelliteConfigurationManager::lnbSelected);

	l = new eLabel(this);
	l->setName("l22khz");
	c = new eComboBox(w_buttons, 3, l);
	sat.hilo=c;
	c->loadDeco();
//			c->move( ePoint( hx, y ) );
	c->resize( eSize( 90, 30 ) );
	c->setHelpText( _("press ok to select another 22kHz mode") );
	new eListBoxEntryText( *c, "Auto", (void*)eSwitchParameter::HILO, 0, _("22kHz signal is automaticaly switched") );
	new eListBoxEntryText( *c, "Hi", (void*)eSwitchParameter::ON, 0, _("22kHz is always enabled (high band)") );
	new eListBoxEntryText( *c, "Lo", (void*)eSwitchParameter::OFF, 0, _("22kHz is always disabled (low band)") );
	c->setCurrent( (void*) (int) s->getSwitchParams().HiLoSignal );
	CONNECT( c->selchanged_id, eSatelliteConfigurationManager::hiloChanged);

	l = new eLabel(this);
	l->setName("lVoltage");
	c = new eComboBox(w_buttons, 3, l	);
	sat.voltage=c;
	c->loadDeco();
//			c->move( ePoint( vx, y ) );
	c->resize( eSize( 90, 30 ) );
	c->setHelpText( _("press ok to select another LNB Voltage mode") );
	new eListBoxEntryText( *c, "Auto", (void*)eSwitchParameter::HV, 0, _("Voltage is automaticaly changed") );
	new eListBoxEntryText( *c, "14V", (void*)eSwitchParameter::_14V, 0, _("Voltage is always 14V (vertical)") );
	new eListBoxEntryText( *c, "18V", (void*)eSwitchParameter::_18V, 0, _("Voltage is always 18V (horizontal") );
	c->setCurrent( (void*) (int) s->getSwitchParams().VoltageMode );
	CONNECT( c->selchanged_id, eSatelliteConfigurationManager::voltageChanged);
	entryMap.insert( std::pair <eSatellite*, SatelliteEntry> ( s, sat ) );
}

void eSatelliteConfigurationManager::newPressed()
{
	std::list<tpPacket>::const_iterator i(networks.begin());
	// we search the next unused Satellite in list...
	int found=0;
	for (; i != networks.end(); ++i)
	{
		std::map< eSatellite*, SatelliteEntry > :: iterator it ( entryMap.begin() );
		for ( ; it != entryMap.end(); it++)
		{
			if ( i->possibleTransponders.begin()->satellite.orbital_position == it->first->getOrbitalPosition() )
				break;  // test the next...
		}
		if ( it == entryMap.end() )  // all Entrys have been checked...
		{
			found++;
			break;
		}
	}
	if ( !found )
	{
		eMessageBox("no more unused Satellites in satellites.xml, please add a new Satellite with possible Transponder(s)","couldnt add Satellite" );
		return;
	}

	eLNB *lnb;
	if ( !eTransponderList::getInstance()->getLNBs().size() )  // lnb list is empty ?
	{
		eTransponderList::getInstance()->getLNBs().push_back( eLNB(*eTransponderList::getInstance()) );
		lnb = &eTransponderList::getInstance()->getLNBs().back();
		lnb->setLOFHi(10600000);
		lnb->setLOFLo(9750000);
		lnb->setLOFThreshold(11700000);
		lnb->getDiSEqC().DiSEqCParam=eDiSEqC::AA;
		lnb->getDiSEqC().DiSEqCMode=eDiSEqC::V1_0;
	}
	else // we use the last lnb in the list for the new Satellite
		lnb = &eTransponderList::getInstance()->getLNBs().back();

	eSatellite *satellite = lnb->addSatellite( i->possibleTransponders.begin()->satellite.orbital_position );
	satellite->setDescription(i->name);
	eSwitchParameter &sParams = satellite->getSwitchParams();
	sParams.VoltageMode = eSwitchParameter::HV;
	sParams.HiLoSignal = eSwitchParameter::HILO;

	// here we must add the new Comboboxes and the button to the hash maps...
	addSatellite(satellite);
	repositionWidgets();
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


eLNBSelitor::eLNBSelitor( eSatellite* sat )
:sat(sat)
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
	l = new eLabel(this);
	l->setName("lDiSEqCMode");
	DiSEqCMode = new eComboBox( this, 3, l );
	DiSEqCMode->setName("DiSEqCMode");
	// *DiSEqCMode... here we use the operator eListBox* from eComboBox !
	new eListBoxEntryText( *DiSEqCMode, "Mini-DiSEqC", (void*)eDiSEqC::MINI, 0, _("Use Mini DiSEqC Mode (for simple switches)") );
	new eListBoxEntryText( *DiSEqCMode, "Version 1.0", (void*)eDiSEqC::V1_0, 0, _("Use DiSEqC Version 1.0") );
//	new eListBoxEntryText( *DiSEqCMode, "Version 1.1", (void*)eDiSEqC::V1_1, 0, _("Use DiSEqC Version 1.1") );
	l = new eLabel(this);
	l->setName("lDiSEqCParam");
	DiSEqCParam = new eComboBox( this, 4, l );
	DiSEqCParam->setName("DiSEqCParam");

	cancel = new eButton(this);
	cancel->setName("cancel");

	save = new eButton(this);
	save->setName("save");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eLNBSelitor"))
		eFatal("skin load of \"eLNBSelitor\" failed");

	// add all LNBs

	int i=0;
	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		new eListBoxEntryText(lnb_list, eString().sprintf("LNB %i", i++), (void*)&(*it) );
	
	// add a None LNB
	new eListBoxEntryText(lnb_list, _("New"), (void*) 0 );

	lnb_list->forEachEntry(	selectlnb( sat->getLNB(), lnb_list ) );

	CONNECT( lofL->selected, eLNBSelitor::numSelected);
	CONNECT( lofH->selected, eLNBSelitor::numSelected);
	CONNECT( threshold->selected, eLNBSelitor::numSelected);
	CONNECT( save->selected, eLNBSelitor::onSave);
	CONNECT( cancel->selected, eLNBSelitor::reject);
	CONNECT( lnb_list->selchanged, eLNBSelitor::lnbChanged);
	CONNECT( lnb_list->selected, eLNBSelitor::lnbSelected);
	CONNECT( DiSEqCMode->selchanged, eLNBSelitor::DiSEqCModeChanged);

	// on exec we begin in eventHandler execBegin
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
		DiSEqCMode->setCurrent( (void*) (int) ((eLNB*)lnb->getKey())->getDiSEqC().DiSEqCMode );
	else
		DiSEqCMode->setCurrent(0);

	DiSEqCModeChanged( DiSEqCMode->getCurrent() );  // fake selchanged for initialize DiSEqCParam ComboBox...	
}

void eLNBSelitor::onSave()
{
	eLNB *p = (eLNB*) lnb_list->getCurrent()->getKey();
//	eDebug("onSave lnb_list->getCurrent()->getKey() = %p", p );

	if ( !p )  // then we must create new LNB; (New is selected)
	{
    eTransponderList::getInstance()->getLNBs().push_back( eLNB( *eTransponderList::getInstance() ) );  // add new LNB
		p = &eTransponderList::getInstance()->getLNBs().back();   // get adresse from the new lnb
//		eDebug("now we have a new LNB Created = %p", p );
	}
/*	else
		eDebug("do not create LNB");*/

	p->setLOFLo( lofL->getNumber() * 1000 );
	p->setLOFHi( lofH->getNumber() * 1000 );
	p->setLOFThreshold( threshold->getNumber() * 1000 );

	p->getDiSEqC().DiSEqCMode = (eDiSEqC::tDiSEqCMode) (int) DiSEqCMode->getCurrent()->getKey();
	p->getDiSEqC().DiSEqCParam = (eDiSEqC::tDiSEqCParam) (int) DiSEqCParam->getCurrent()->getKey();

	if ( p != sat->getLNB() )  // the satellite must removed from the old lnb and inserts in the new
	{
		p->addSatellite( sat->getLNB()->takeSatellite( sat ) );
/*		eDebug("remove satellite from lnb... now %i satellites left", sat->getLNB()->getSatelliteList().size() );
		eDebug("added satellite to lnb... now %i satellites in lnb", p->getSatelliteList().size() );		*/

		if ( !sat->getLNB()->getSatelliteList().size() )   // the lnb that have no more satellites must delete
		{
//			eDebug("delete no more used lnb");
		  eTransponderList::getInstance()->getLNBs().remove( *sat->getLNB() );
		}
/*		else
			eDebug("lnb not deleted");*/

		// now we must set the LNB Pointer in eSatellite...
		sat->setLNB(p);		

		close(-1); // we must reposition control elements in eSatelliteConfigurationManager
	}
	else
		close(0); // we must not reposition...
}

void eLNBSelitor::numSelected(int*)
{
	focusNext( eWidget::focusDirNext );
}

void eLNBSelitor::DiSEqCModeChanged ( eListBoxEntryText* le)
{
	if ( DiSEqCParam->getCount() )  // then still elements in the ComboBox... we remove it...
		DiSEqCParam->clear();

	if ( (eDiSEqC::tDiSEqCMode) (int) le->getKey() == eDiSEqC::MINI )
	{
		new eListBoxEntryText( *DiSEqCParam, "A", (void*)eDiSEqC::AA, 0, _("sends no MiniDiSEqC burst") );
		new eListBoxEntryText( *DiSEqCParam, "B", (void*)eDiSEqC::AB, 0, _("sends MiniDiSEqC burst") );
	}
	else // DiSEqC 1.0 or 1.1
	{
		new eListBoxEntryText( *DiSEqCParam, "A/A", (void*)eDiSEqC::AA, 0, _("sends DiSEqC cmd A/A") );
		new eListBoxEntryText( *DiSEqCParam, "A/B", (void*)eDiSEqC::AB, 0, _("sends DiSEqC cmd A/B") );
		new eListBoxEntryText( *DiSEqCParam, "B/A", (void*)eDiSEqC::BA, 0, _("sends DiSEqC cmd B/A") );
		new eListBoxEntryText( *DiSEqCParam, "B/B", (void*)eDiSEqC::BB, 0, _("sends DiSEqC cmd B/B") );
	}
	if ( lnb_list->getCurrent() && lnb_list->getCurrent()->getKey() )
		DiSEqCParam->setCurrent( (void*) ((eLNB*)lnb_list->getCurrent()->getKey())->getDiSEqC().DiSEqCParam );
	else
		DiSEqCParam->setCurrent( 0 );
}

int eLNBSelitor::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
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
		return 1;
	
	case eWidgetEvent::execBegin:
		lnbChanged( lnb_list->getCurrent() );   // fake selchanged for initialize lofl, lofh, threshold...
		DiSEqCModeChanged( DiSEqCMode->getCurrent() );  // fake selchanged for initialize DiSEqCParam ComboBox...
	break;

	default:
		break;
	}
	return eWindow::eventHandler(event);
}

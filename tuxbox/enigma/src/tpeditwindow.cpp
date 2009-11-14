#include <tpeditwindow.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/dvbscan.h>
#include <lib/dvb/dvbwidgets.h>
#include <lib/gui/actions.h>
#include <lib/gui/enumber.h>
#include <lib/gui/combobox.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/eskin.h>
#include <lib/gui/textinput.h>
#include <lib/gui/emessage.h>
#include <lib/gdi/font.h>
#include <lib/system/init_num.h>
#include <lib/system/init.h>

gFont eListBoxEntryTransponder::font;

eListBoxEntryTransponder::eListBoxEntryTransponder( eListBox<eListBoxEntryTransponder>* lb, eTransponder* tp )
	:eListBoxEntry( (eListBox<eListBoxEntry>*)lb ), tp(tp)
{
}

const eString& eListBoxEntryTransponder::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state)
{
	bool b = (state == 2);

	drawEntryRect( rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, b?0:state );

	static eString text;
	text.sprintf("%d / %d / %c", tp->satellite.frequency/1000, tp->satellite.symbol_rate/1000, tp->satellite.polarisation?'V':'H' );
	rc->setFont(font);
	rc->renderText( rect, text );

	if ( b )
		drawEntryBorder( rc, rect, coActiveB, coActiveF, coNormalB, coNormalF );

	return text;
}

int eListBoxEntryTransponder::getEntryHeight()
{
	if ( !font.pointSize)
		font = eSkin::getActive()->queryFont("eListBox.EntryText.normal");

	return calcFontHeight( font ) + 4;
}

bool eListBoxEntryTransponder::operator < ( const eListBoxEntry& e ) const
{
	return tp->operator<(*((eListBoxEntryTransponder&)e).tp);
}

eSatEditDialog::eSatEditDialog( tpPacket *tp )
	:eWindow(0), tp(tp)
{
	init_eSatEditDialog();
}
void eSatEditDialog::init_eSatEditDialog()
{
	name = CreateSkinnedTextInputField("name",tp->name.c_str());
	name->setMaxChars(50);
	name->setUseableChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,/()-°");
	OrbitalPos = CreateSkinnedNumberWithLabel( "OrbitalPos",abs(tp->orbital_position), 1, 0, 3600, 4, 0, 0, "lOrbitalPos");
	direction = CreateSkinnedComboBox( "direction", 2);
	new eListBoxEntryText( *direction, _("East"), (void*)0, 0, _("East") );
	new eListBoxEntryText( *direction, _("West"), (void*)1, 0, _("West") );
	direction->setCurrent( (void*) (tp->orbital_position<0) );
	doNetworkSearch = CreateSkinnedCheckbox("doNetworkSearch",tp->scanflags&eDVBScanController::flagNetworkSearch);
	useONIT = CreateSkinnedCheckbox("useONIT", tp->scanflags&eDVBScanController::flagUseONIT );
	useBAT = CreateSkinnedCheckbox("useBAT",tp->scanflags&eDVBScanController::flagUseBAT?1:0 );
	CONNECT(CreateSkinnedButton("save")->selected, eSatEditDialog::savePressed );
	BuildSkin("eSatEditDialog");
}

void eSatEditDialog::savePressed()
{
	tp->orbital_position =
		direction->getCurrent()->getKey() ?
			-OrbitalPos->getNumber() : OrbitalPos->getNumber();
	tp->scanflags=0;
	if ( doNetworkSearch->isChecked() )
		tp->scanflags |= eDVBScanController::flagNetworkSearch;
	if ( useONIT->isChecked() )
		tp->scanflags |= eDVBScanController::flagUseONIT;
	if ( useBAT->isChecked() )
		tp->scanflags |= eDVBScanController::flagUseBAT;
	tp->name=name->getText();
	close(0);
}

eTPEditDialog::eTPEditDialog( eTransponder *tp )
	:eWindow(0), tp(tp)
{
	init_eTPEditDialog();
}
void eTPEditDialog::init_eTPEditDialog()
{
	setText(_("Transponder Edit"));
	cmove( ePoint( 120, 200 ) );
	cresize( eSize( 460, 210 ) );
	tpWidget=new eTransponderWidget(this, 1,
		eTransponderWidget::deliverySatellite|
		eTransponderWidget::flagNoSat|
		eTransponderWidget::flagNoInv);
	tpWidget->resize( eSize( 460, 130 ) );
	tpWidget->load();
	tpWidget->setTransponder( tp );
	tpWidget->move( ePoint(0,-40) );
	save=new eButton( this );
	save->setText(_("save"));
	save->setShortcut("green");
	save->setShortcutPixmap("green");
	save->setHelpText(_("save changes and return"));
	save->move(ePoint( 10, getClientSize().height()-80) );
	save->resize( eSize( 220, 40 ) );
	save->loadDeco();
	CONNECT( save->selected, eTPEditDialog::savePressed );
	eStatusBar *sbar = new eStatusBar(this);
	sbar->move( ePoint( 0, getClientSize().height()-30) );
	sbar->resize( eSize( getClientSize().width(), 30 ) );
	sbar->loadDeco();
	setHelpID(63);
}

void eTPEditDialog::savePressed()
{
	tpWidget->getTransponder( tp );
	close(0);
}

struct TransponderEditWindowActions
{
	eActionMap map;
	eAction addNetwork, removeNetwork;
	TransponderEditWindowActions():
		map("TransponderEditWindow", "TransponderEditWindow"),
		addNetwork(map, "addNetwork", _("add new Network"), eAction::prioWidget),
		removeNetwork(map, "removeNetwork", _("remove selected Network"), eAction::prioWidget)
	{
	}
};

eAutoInitP0<TransponderEditWindowActions> i_TransponderEditWindowActions(eAutoInitNumbers::actions, "TransponderEditWindow Actions");

eTransponderEditWindow::eTransponderEditWindow()
	:eWindow(0), changed(0)
{
	init_eTransponderEditWindow();
}
void eTransponderEditWindow::init_eTransponderEditWindow()
{
	addActionMap(&i_TransponderEditWindowActions->map);
	sat = CreateSkinnedButton("sat");
	CONNECT(sat->selected, eTransponderEditWindow::satPressed);
	CONNECT(CreateSkinnedButton("add")->selected, eTransponderEditWindow::addPressed);
	CONNECT(CreateSkinnedButton("edit")->selected, eTransponderEditWindow::editPressed);
	CONNECT(CreateSkinnedButton("remove")->selected, eTransponderEditWindow::removePressed);
	satellites = new eListBox<eListBoxEntryText>( this, 0, 0 );
	satellites->setName("satlist");
	transponders = new eListBox<eListBoxEntryTransponder>( this, 0, 0 );
	transponders->setName("transponderlist");
	transponders->FakeFocus( 0 );
	satellites->FakeFocus( 1 );
	BuildSkin("TransponderWindow");

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	eTransponder *tp=0;

	if (sapi && sapi->transponder)
		tp = sapi->transponder;

	eListBoxEntryText *sel=0;

	if( !eTransponderList::getInstance()->reloadNetworks() )
	{
		for ( std::list<tpPacket>::iterator i(eTransponderList::getInstance()->getNetworks().begin()); i != eTransponderList::getInstance()->getNetworks().end(); ++i)
			if ( tp && i->orbital_position == tp->satellite.orbital_position )
				sel = new eListBoxEntryText(satellites, i->name, (void*) &(*i));
			else
				new eListBoxEntryText(satellites, i->name, (void*) &(*i));
	}
	CONNECT(satellites->selchanged, eTransponderEditWindow::satSelChanged );

	if ( sel )
		satellites->setCurrent(sel);
	else
		satellites->setCurrent(0);

	CONNECT( eWidget::focusChanged, eTransponderEditWindow::focusChanged );
	satSelChanged( satellites->getCurrent() );
	setHelpID(64);
}

int eTransponderEditWindow::eventHandler( const eWidgetEvent & event )
{
	switch( event.type )
	{
		case eWidgetEvent::execDone:
			if ( changed )
			{
				int ret = eMessageBox::ShowBox(_("Save changed Transponders?"),
					_("Transponders changed"),
					eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion,
					eMessageBox::btYes );
				eTransponderList::getInstance()->invalidateNetworks();
				if ( ret == eMessageBox::btNo || ret == -1 )
					return 0;
				if ( eTransponderList::getInstance()->saveNetworks() )
				{
					eMessageBox::ShowBox(_("Transponders couldn't be written to file!"),
						_("Write Error"),
						eMessageBox::btOK|eMessageBox::iconWarning);
				}
			}
			return 0;
		case eWidgetEvent::evtAction:
		{
			if ( event.action == &i_focusActions->up )
			{
				if ( focus == sat )
					satellites->goPrev();
				else
					transponders->goPrev();
			}
			else if ( event.action == &i_focusActions->down )
			{
				if ( focus == sat )
					satellites->goNext();
				else
					transponders->goNext();
			}
			else if ( event.action == &i_TransponderEditWindowActions->addNetwork )
				addNetwork();
			else if ( event.action == &i_TransponderEditWindowActions->removeNetwork )
				removeNetwork();
			else
				break;
			return 0;
		}
		default:
			break;
	}
	return eWindow::eventHandler( event );
}

void eTransponderEditWindow::focusChanged( const eWidget* w )
{
	static bool b = true;
	if ( in_loop && ( w && w->getName() && b != (w->getName()=="sat") ) )
	{
		b=w->getName()=="sat";
		transponders->FakeFocus( !b );
		satellites->FakeFocus( b );
		satellites->invalidateCurrent();
		transponders->invalidateCurrent();
	}
}

eTransponderEditWindow::~eTransponderEditWindow()
{
}

void eTransponderEditWindow::satSelChanged( eListBoxEntryText* sat )
{
	transponders->beginAtomic();
	transponders->clearList();
	if (sat && sat->getKey())
	{
		tpPacket *satTPs = (tpPacket*) (sat->getKey());
		for (std::list<eTransponder>::iterator it( satTPs->possibleTransponders.begin() ); it != satTPs->possibleTransponders.end(); it++)
			new eListBoxEntryTransponder( transponders, &(*it) );

		if (transponders->getCount())
			transponders->setCurrent(0);
	}
	transponders->endAtomic();
}

void eTransponderEditWindow::satPressed()
{
	eListBoxEntryText *sat = satellites->getCurrent();
	if ( sat && sat->getKey() )
	{
		hide();
		eSatEditDialog dlg( (tpPacket*)sat->getKey() );
#ifndef DISABLE_LCD
		dlg.setLCD(LCDTitle, LCDElement);
#endif
		dlg.show();
		if ( !dlg.exec() )
		{
			tpPacket *p = (tpPacket*)sat->getKey();
			sat->SetText( p->name );
			changed++;
		}
		dlg.hide();
		show();
	}
}

void eTransponderEditWindow::addPressed()
{
	eTransponder t(*eDVB::getInstance()->settings->getTransponders());
	eTPEditDialog dlg( &t );
#ifndef DISABLE_LCD
	dlg.setLCD( LCDTitle, LCDElement );
#endif
	dlg.show();
	int ret = dlg.exec();
	dlg.hide();
	if ( !ret )
	{
		changed++;
		eListBoxEntryText *sat = satellites->getCurrent();
		if ( !sat )
			return;
		tpPacket *dest = (tpPacket*)sat->getKey();
		if ( !dest )
			return;
		dest->possibleTransponders.push_back( t );
		transponders->beginAtomic();
		eListBoxEntryTransponder *e = new eListBoxEntryTransponder( transponders, &dest->possibleTransponders.back() );
		dest->possibleTransponders.sort();
		transponders->sort();
		transponders->setCurrent(e);
		transponders->endAtomic();
	}
}

void eTransponderEditWindow::editPressed()
{
	eListBoxEntryTransponder *te = transponders->getCurrent();
	if ( !te )
		return;
	eTransponder *tp = te->getTransponder();
	if ( !tp )
		return;
	eTPEditDialog dlg( tp );
#ifndef DISABLE_LCD
	dlg.setLCD( LCDTitle, LCDElement );
#endif
	dlg.show();
	int ret = dlg.exec();
	dlg.hide();

	if ( !ret )
	{
		changed++;
		transponders->beginAtomic();
		transponders->sort();
		transponders->endAtomic();
	}
}

void eTransponderEditWindow::removePressed()
{
	eListBoxEntryTransponder *te = transponders->getCurrent();
	if ( !te )
		return;
	eTransponder *tp = te->getTransponder();
	if ( !tp )
		return;
	eListBoxEntryText *se = satellites->getCurrent();
	if ( !se )
		return;
	tpPacket *packet = (tpPacket*)se->getKey();
	if ( !packet )
		return;
	std::list<eTransponder>::iterator it =
		packet->possibleTransponders.begin();
	for ( ; it != packet->possibleTransponders.end(); ++it )
		if ( &(*it) == tp )
			break;
	if ( it == packet->possibleTransponders.end() )
		return;
	packet->possibleTransponders.erase(it);
	if ( transponders->getNext() )
		transponders->goNext();
	else
		transponders->goPrev();
	transponders->remove( te, true );
	changed++;
}

void eTransponderEditWindow::addNetwork()
{
	tpPacket *p = new tpPacket();
	p->name=_("new Satellite");
	p->orbital_position=0;
	p->scanflags=0;
	hide();
	eSatEditDialog dlg( p );
#ifndef DISABLE_LCD
	dlg.setLCD(LCDTitle, LCDElement);
#endif
	dlg.show();
	if ( !dlg.exec() )
	{
		eTransponderList::getInstance()->getNetworks().push_back( *p );
		satellites->setCurrent(
			new eListBoxEntryText(satellites, p->name, (void*)&eTransponderList::getInstance()->getNetworks().back() ));
		changed++;
	}
	else
		delete p;
	dlg.hide();
	show();
}

void eTransponderEditWindow::removeNetwork()
{
	eListBoxEntryText* entry = satellites->getCurrent();
	if ( entry && entry->getKey() )
	{
		tpPacket *p = (tpPacket*) entry->getKey();
		eMessageBox mb(
			_("Really delete the selected satellite?"),
			eString().sprintf(_("Delete %s"), p->name.c_str() ),
			eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion, eMessageBox::btNo );
		hide();
		mb.show();
		if ( mb.exec() == eMessageBox::btYes )
		{
			satellites->beginAtomic();
			satellites->goNext();
			eTransponderList::getInstance()->getNetworks().remove( *p );
			satellites->remove( entry, true );
			satellites->endAtomic();
			changed++;
		}
		mb.hide();
		show();
	}
}

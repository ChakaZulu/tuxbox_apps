#include <algorithm>
#include <list>

#include <enigma.h>
#include <enigma_epg.h>
#include <enigma_main.h>
#include <sselect.h>

#include <lib/base/i18n.h>
#include <lib/driver/rc.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/service.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/epgcache.h>
#include <lib/dvb/frontend.h>
#include <lib/dvb/serviceplaylist.h>
#include <lib/dvb/record.h>
#include <lib/gdi/font.h>
#include <lib/gui/actions.h>
#include <lib/gui/eskin.h>
#include <lib/gui/elabel.h>
#include <lib/gui/numberactions.h>
#include <lib/system/info.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>

gFont eListBoxEntryService::serviceFont;
gFont eListBoxEntryService::descrFont;
gFont eListBoxEntryService::numberFont;
gPixmap *eListBoxEntryService::folder=0;
gPixmap *eListBoxEntryService::marker=0;
gPixmap *eListBoxEntryService::locked=0;
int eListBoxEntryService::maxNumSize=0;
std::set<eServiceReference> eListBoxEntryService::hilitedEntrys;
eListBoxEntryService *eListBoxEntryService::selectedToMove=0;

struct EPGStyleSelectorActions
{
	eActionMap map;
	eAction infoPressed;
	EPGStyleSelectorActions():
		map("EPGStyleSelector", _("EPG Style Selector")),
		infoPressed(map, "infoPressed", _("open the EPG with selected style"), eAction::prioDialog)
	{
	}
};
eAutoInitP0<EPGStyleSelectorActions> i_EPGStyleSelectorActions(eAutoInitNumbers::actions, "EPG Style Selector actions");

class eEPGStyleSelector: public eListBoxWindow<eListBoxEntryText>
{
public:
	eEPGStyleSelector()
		:eListBoxWindow<eListBoxEntryText>(_("EPG Style"), 5, 350, true)
	{
		addActionMap( &i_EPGStyleSelectorActions->map );
		move(ePoint(100,100));
		int last=1;
		eListBoxEntryText*sel=0;
		eConfig::getInstance()->getKey("/ezap/serviceselector/lastEPGStyle", last);
		new eListBoxEntryText(&list,_("Channel EPG"), (void*)1, 0, _("open EPG for selected Channel") );
		sel = new eListBoxEntryText(&list,_("Multi EPG"), (void*)2, 0, _("open EPG for next five channels") );
		if ( last==2 )
			list.setCurrent(sel);
		CONNECT( list.selected, eEPGStyleSelector::entrySelected );
	}
	int eventHandler( const eWidgetEvent &event )
	{
		switch (event.type)
		{
			case eWidgetEvent::evtAction:
				if ( event.action == &i_EPGStyleSelectorActions->infoPressed )
					entrySelected( list.getCurrent() );
				else
					break;
				return 1;
			default:
				break;
		}
		return eWindow::eventHandler( event );
	}
	void entrySelected( eListBoxEntryText* e )
	{
		if (e)
		{
			int last=1;
			eConfig::getInstance()->getKey("/ezap/serviceselector/lastEPGStyle", last);
			if ( last != (int) e->getKey() )
				eConfig::getInstance()->setKey("/ezap/serviceselector/lastEPGStyle", (int)e->getKey());
			close( (int)e->getKey() );
		}
		else
			close(-1);
	}
};

struct serviceSelectorActions
{
	eActionMap map;
	eAction nextBouquet, prevBouquet, pathUp, showEPGSelector, showMenu, 
			addService, addServiceToUserBouquet, modeTV, modeRadio,
			modeFile, toggleStyle, toggleFocus, gotoPrevMarker, gotoNextMarker,
			showAll, showSatellites, showProvider, showBouquets, deletePressed,
			markPressed, renamePressed, newMarkerPressed;
	serviceSelectorActions():
		map("serviceSelector", _("service selector")),
		nextBouquet(map, "nextBouquet", _("switch to next bouquet"), eAction::prioDialogHi),
		prevBouquet(map, "prevBouquet", _("switch to previous bouquet"), eAction::prioDialogHi),
		pathUp(map, "pathUp", _("go up a directory"), eAction::prioDialog),
		showEPGSelector(map, "showEPGSelector", _("shows the EPG selector for the highlighted channel"), eAction::prioDialog),
		showMenu(map, "showMenu", _("show service selector menu"), eAction::prioDialog),
		addService(map, "addService", _("add service to playlist"), eAction::prioDialog),
		addServiceToUserBouquet(map, "addServiceToUserBouquet", _("add service to a specific bouquet"), eAction::prioDialog),
		modeTV(map, "modeTV", _("switch to TV mode"), eAction::prioDialog),
		modeRadio(map, "modeRadio", _("switch to Radio mode"), eAction::prioDialog),
		modeFile(map, "modeFile", _("switch to File mode"), eAction::prioDialog),
		toggleStyle(map, "toggleStyle", _("toggle between classic and multi column style"), eAction::prioDialog),
		toggleFocus(map, "toggleFocus", _("toggle focus between service and bouquet list (in combi column style)"), eAction::prioDialog),
		gotoPrevMarker(map, "gotoPrevMarker", _("go to the prev Marker if exist.. else goto first service"), eAction::prioDialogHi),
		gotoNextMarker(map, "gotoNextMarker", _("go to the next Marker if exist.. else goto last service"), eAction::prioDialogHi),
		
		showAll(map, "showAll", _("show all services"), eAction::prioDialog),
		showSatellites(map, "showSatellites", _("show satellite list"), eAction::prioDialog),
		showProvider(map, "showProvider", _("show provider list"), eAction::prioDialog),
		showBouquets(map, "showBouquets", _("show bouquet list"), eAction::prioDialog),

		deletePressed(map, "delete", _("delete selected entry"), eAction::prioDialog),
		markPressed(map, "mark", _("mark selected entry for move"), eAction::prioDialog),
		renamePressed(map, "rename", _("rename selected entry"), eAction::prioDialog),
		newMarkerPressed(map, "marker", _("create new marker entry"), eAction::prioDialog)
	{
	}
};
eAutoInitP0<serviceSelectorActions> i_serviceSelectorActions(eAutoInitNumbers::actions, "service selector actions");

eListBoxEntryService::eListBoxEntryService(eListBox<eListBoxEntryService> *lb, const eServiceReference &service, int flags, int num)
	:eListBoxEntry((eListBox<eListBoxEntry>*)lb), numPara(0),
	namePara(0), descrPara(0), nameXOffs(0), flags(flags),
	num(num), curEventId(-1), service(service)
{
	static char strfilter[4] = { 0xC2, 0x87, 0x86, 0x00 };
	if (!(flags & flagIsReturn))
	{
#if 0
		sort=eString().sprintf("%06d", service->service_number);
#else
		if( service.descr )
			sort = service.descr;
		else
		{
			const eService *pservice=eServiceInterface::getInstance()->addRef(service);
			if ( pservice )
			{
				sort=pservice?pservice->service_name:"";
				eServiceInterface::getInstance()->removeRef(service);
			}
		}
		sort.upper();

		// filter short name brakets...
		for (eString::iterator it(sort.begin()); it != sort.end();)
			strchr( strfilter, *it ) ? it = sort.erase(it) : it++;

#endif
	} else
		sort="";
}

eListBoxEntryService::~eListBoxEntryService()
{
	invalidate();
}

int eListBoxEntryService::getEntryHeight()
{
	if (!descrFont.pointSize)
		descrFont = eSkin::getActive()->queryFont("eServiceSelector.Entry.Description");

	return calcFontHeight(serviceFont)+4;
}

void eListBoxEntryService::invalidate()
{
	if (numPara)
	{
		numPara->destroy();
		numPara=0;
	}
	if (descrPara)
	{
		descrPara->destroy();
		descrPara=0;
	}
	if (namePara)
	{
		namePara->destroy();
		namePara=0;
	}
}

void eListBoxEntryService::invalidateDescr()
{
	if (descrPara)
	{
		descrPara->destroy();
		descrPara=0;
	}
}

const eString &eListBoxEntryService::redraw(gPainter *rc, const eRect &rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int hilited)
{
	bool b;

	if ( (b = (hilited == 2)) )
		hilited = 0;

	if (this == selectedToMove)
		drawEntryRect(rc, rect, eSkin::getActive()->queryColor("eServiceSelector.entrySelectedToMove"), coActiveF, coNormalB, coNormalF, hilited );
	else  
		drawEntryRect(rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, hilited );

	const eService *pservice=eServiceInterface::getInstance()->addRef(service);

	std::set<eServiceReference>::iterator it = hilitedEntrys.find( service );
	if ( it != hilitedEntrys.end() )
		rc->setForegroundColor( eSkin::getActive()->queryColor("eServiceSelector.entryHilited") );

	if ( service.flags & eServiceReference::isDirectory && folder )  // we draw the folder pixmap
	{
		nameXOffs = folder->x + 20;
		int ypos = (rect.height() - folder->y) / 2;
		rc->blit( *folder, ePoint(10, rect.top()+ypos ), eRect(), gPixmap::blitAlphaTest);
	}
	else if ( service.flags & eServiceReference::isMarker && marker )
	{
		nameXOffs = marker->x + 20;
		int ypos = (rect.height() - marker->y) / 2;
		rc->blit( *marker, ePoint(10, rect.top()+ypos ), eRect(), gPixmap::blitAlphaTest);
	}
	else if (flags & flagShowNumber && listbox->getColumns() == 1)
	{
		int n=-1;
		if (flags & flagOwnNumber)
			n=num;
		else if ( pservice && pservice->dvb )
			n=pservice->dvb->service_number;

		if (n != -1)
		{
			if (!numPara)
			{
				numPara = new eTextPara( eRect( 0, 0, maxNumSize, rect.height() ) );
				numPara->setFont( numberFont );
				numPara->renderString( eString().setNum(n) );
				numPara->realign(eTextPara::dirRight);
				numYOffs = ((rect.height() - numPara->getBoundBox().height()) / 2 ) - numPara->getBoundBox().top();
				nameXOffs = maxNumSize+numPara->getBoundBox().height();
			}
			rc->renderPara(*numPara, ePoint( rect.left(), rect.top() + numYOffs ) );
		}
	}

	if (!namePara)
	{
		eString sname;

		if (service.descr.length())
			sname=service.descr;
		else if (pservice)
			sname=pservice->service_name;
		else if (flags & flagIsReturn)
			sname=_("[GO UP]");
		else
			sname=_("(removed service)");

		namePara = new eTextPara( eRect( 0, 0, rect.width()-nameXOffs, rect.height() ) );
		namePara->setFont( serviceFont );
		namePara->renderString( sname );
		if (flags & flagIsReturn )
			namePara->realign(eTextPara::dirCenter);
		nameYOffs = ((rect.height() - namePara->getBoundBox().height()) / 2 ) - namePara->getBoundBox().top();	
	}
	// we can always render namePara
	rc->renderPara(*namePara, ePoint( rect.left() + nameXOffs, rect.top() + nameYOffs ) );

	if ( service.isLocked() && locked && eConfig::getInstance()->getParentalPin() )
	{
		int ypos = (rect.height() - locked->y) / 2;
		rc->blit( *locked, ePoint(nameXOffs+namePara->getBoundBox().width()+10, rect.top()+ypos ), eRect(), gPixmap::blitAlphaTest);
	}

	if ( listbox->getColumns() == 1 )
	{
		if ( !descrPara &&  
					service.type == eServiceReference::idDVB &&
					(!(service.flags & eServiceReference::isDirectory)) &&
					(!service.path.size()) )  // recorded dvb streams
		{
			eString sdescr;
			if (pservice && service.type == eServiceReference::idDVB && !(service.flags & eServiceReference::isDirectory) )
			{
				EITEvent *e=eEPGCache::getInstance()->lookupEvent((const eServiceReferenceDVB&)service);
				if (e)
				{
					for (ePtrList<Descriptor>::iterator d(e->descriptor); d != e->descriptor.end(); ++d)
					{
						if (d->Tag()==DESCR_SHORT_EVENT)
						{
							ShortEventDescriptor *ss=(ShortEventDescriptor*)*d;
							if ( ss->event_name )
								sdescr='('+ss->event_name+')';
							break;
						}
					}
					if (sdescr.length())
					{
						curEventId = e->event_id;
						descrPara = new eTextPara( eRect( 0, 0, rect.width(), rect.height() ) );
						descrPara->setFont( descrFont );
						descrPara->renderString( sdescr );
						descrXOffs = nameXOffs+namePara->getBoundBox().width();
						if ( service.isLocked() && locked && eConfig::getInstance()->getParentalPin() )
							descrXOffs = descrXOffs + locked->x;
						if (numPara)
							descrXOffs += numPara->getBoundBox().height();
						descrYOffs = ((rect.height() - descrPara->getBoundBox().height()) / 2 ) - descrPara->getBoundBox().top();
					}
					delete e;
				}
			}
		}
		if (descrPara)  // only render descr Para, when avail...
			rc->renderPara(*descrPara, ePoint( rect.left()+descrXOffs, rect.top() + descrYOffs ) );
	}

	if (b)
		drawEntryBorder(rc, rect, coActiveB, coActiveF, coNormalB, coNormalF );

	if ( pservice )
		eServiceInterface::getInstance()->removeRef(service);

	return sort;
}

void eServiceSelector::setKeyDescriptions( bool editMode )
{
	if (!(key[0] && key[1] && key[2] && key[3]))
		return;

	if ( editMode )
	{
		key[0]->setText(_("delete"));
		key[1]->setText(_("mark"));
		key[2]->setText(_("rename"));
		key[3]->setText(_("marker"));
		return;
	}

	switch (eZapMain::getInstance()->getMode())
	{
		case eZapMain::modeTV:
		case eZapMain::modeRadio:
			key[0]->setText(_("All Services"));
			if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
				key[1]->setText(_("Satellites"));
			else
				key[1]->setText("");
			key[2]->setText(_("Providers"));
			key[3]->setText(_("Bouquets"));
			break;
#ifndef DISABLE_FILE
		case eZapMain::modeFile:
			key[0]->setText(_("Root"));
			key[1]->setText(_("Movies"));
			key[2]->setText(_("Playlist"));
			key[3]->setText(_("Root"));
			break;
#endif
	}
}

void eServiceSelector::addService(const eServiceReference &ref)
{
	if ( eZap::getInstance()->getServiceSelector() == this )
	{
#ifndef DISABLE_FILE
		if ( eDVB::getInstance()->recorder && eZapMain::getInstance()->getMode() != eZapMain::modeFile )
		{
			eServiceReferenceDVB &Ref = (eServiceReferenceDVB&) ref;
			eServiceReferenceDVB &rec = eDVB::getInstance()->recorder->recRef;
			if ( rec.getTransportStreamID() != Ref.getTransportStreamID() ||
					 rec.getOriginalNetworkID() != Ref.getOriginalNetworkID() ||
					 rec.getDVBNamespace() != Ref.getDVBNamespace() )
				return;
		}
#endif
		if ( ref.isLocked() && (eConfig::getInstance()->pLockActive() & 2) )
			return;
	}

	int flags=serviceentryflags;

	if ( ref.flags & eServiceReference::isDirectory)
		flags &= ~ eListBoxEntryService::flagShowNumber;

	new eListBoxEntryService(services, ref, flags);
}

void eServiceSelector::addBouquet(const eServiceReference &ref)
{
	new eListBoxEntryService(bouquets, ref, serviceentryflags);
}

struct renumber: public std::unary_function<const eListBoxEntryService&, void>
{
	int &num;
	bool invalidate;
	renumber(int &num, bool invalidate=false)
		:num(num), invalidate(invalidate)
	{
	}

	bool operator()(eListBoxEntryService& s)
	{
		if (!s.service || s.service.flags == eServiceReference::isMarker )
			return 0;
		if ( !(s.service.flags & (eServiceReference::isDirectory)) )
		{
			s.num = ++num;
			s.invalidate();
		}

		return 0;
	}
};

void eServiceSelector::fillServiceList(const eServiceReference &_ref)
{
	eString windowDescr;
	eServicePath p = path;

	eServiceReference ref;

	// build complete path ... for window titlebar..
	do
	{
		eServiceReference b=p.current();
		const eService *pservice=eServiceInterface::getInstance()->addRef(b);
	  if (pservice && pservice->service_name.length() )
			windowDescr=pservice->service_name+" > "+windowDescr;
		ref = p.current();
		p.up();
		eServiceInterface::getInstance()->removeRef(b);
	}
	while ( ref != p.current() );
	if (windowDescr.rfind(">") != eString::npos)
		windowDescr.erase( windowDescr.rfind(">") );
	setText( windowDescr );

	ref=_ref;

	services->beginAtomic();
	services->clearList();

	if ( !movemode )
	{
		if ( path.size() > 1 && style != styleCombiColumn )
		{
			goUpEntry = new eListBoxEntryService(services, eServiceReference(), eListBoxEntryService::flagIsReturn);
		}
		else
			goUpEntry = 0;
	}

	eServiceInterface *iface=eServiceInterface::getInstance();
	ASSERT(iface);

	Signal1<void,const eServiceReference&> signal;
	CONNECT(signal, eServiceSelector::addService);

	serviceentryflags=eListBoxEntryService::flagShowNumber;

	if (ref.type == eServicePlaylistHandler::ID) // playlists have own numbers
		serviceentryflags|=eListBoxEntryService::flagOwnNumber;

	if ( eZap::getInstance()->getServiceSelector() == this
		&& eDVB::getInstance()->recorder
		&& eZapMain::getInstance()->getMode() != eZapMain::modeFile )
	{
		int mask = eZapMain::getInstance()->getMode() == eZapMain::modeTV ? (1<<4)|(1<<1) : ( 1<<2 );
		eServiceReference bla(eServiceReference::idDVB,
				eServiceReference::flagDirectory|eServiceReference::shouldSort,
				-2, mask, 0xFFFFFFFF );
		iface->enterDirectory(bla, signal);
		iface->leaveDirectory(bla);
	}
	else
	{
		iface->enterDirectory(ref, signal);
		iface->leaveDirectory(ref);	// we have a copy.
	}

	if (ref.flags & eServiceReference::shouldSort)
		services->sort();

	if (serviceentryflags & eListBoxEntryService::flagOwnNumber)
	{
		int num=0;
		services->forEachEntry( renumber(num) );
	}

/*	// now we calc the x size of the biggest number we have;
	if (num)
	{
		eTextPara  *tmp = new eTextPara( eRect(0, 0, 100, 50) );
		tmp->setFont( eListBoxEntryService::numberFont );
		tmp->renderString( eString().setNum( num ) );
		eListBoxEntryService::maxNumSize = tmp->getBoundBox().width()+10;
		tmp->destroy();
	}
	else */
		eListBoxEntryService::maxNumSize=45;

	services->endAtomic();
}

void eServiceSelector::updateNumbers()
{
	int num=0;
	services->beginAtomic();
	services->forEachEntry( renumber(num, true) );
	services->invalidateContent();
	services->endAtomic();
}

void eServiceSelector::fillBouquetList( const eServiceReference& _ref)
{
	bouquets->beginAtomic();
	bouquets->clearList();

	eServiceInterface *iface=eServiceInterface::getInstance();
	ASSERT(iface);
	
	Signal1<void,const eServiceReference&> signal;
	CONNECT(signal, eServiceSelector::addBouquet );
	
	eServiceReference ref=_ref;
	
	iface->enterDirectory(ref, signal);
	iface->leaveDirectory(ref);	// we have a copy.

	if (ref.flags & eServiceReference::shouldSort)
		bouquets->sort();

	bouquets->endAtomic();
}

struct moveFirstChar: public std::unary_function<const eListBoxEntryService&, void>
{
	char c;

	moveFirstChar(char c): c(c)
	{
	}

	bool operator()(const eListBoxEntryService& s)
	{
		if (s.sort[0] == c)
		{
			( (eListBox<eListBoxEntryService>*) s.listbox)->setCurrent(&s);
			return 1;
		}
		return 0;
	}
};

struct moveServiceNum: public std::unary_function<const eListBoxEntryService&, void>
{
	int num;

	moveServiceNum(int num): num(num)
	{
	}

	bool operator()(const eListBoxEntryService& s)
	{
		if (s.num == num)
		{
			( (eListBox<eListBoxEntryService>*) s.listbox)->setCurrent(&s);
			return 1;
		}
		return 0;
	}
};

bool eServiceSelector::selectService(int num)
{
	return services->forEachEntry( moveServiceNum( num ) ) == eListBoxBase::OK;
}

struct findServiceNum: public std::unary_function<const eListBoxEntryService&, void>
{
	int& num;
	const eServiceReference& service;

	findServiceNum(const eServiceReference& service, int& num): num(num), service(service)
	{
	}

	bool operator()(const eListBoxEntryService& s)
	{
		if (s.service == service)
		{
			num=s.getNum();
			return 1;
		}
		return 0;
	}
};

int eServiceSelector::getServiceNum( const eServiceReference &ref )
{
	int ret=-1;
	services->forEachEntry( findServiceNum(ref, ret ) );
	return ret;
}

void eServiceSelector::gotoChar(char c)
{
	eDebug("gotoChar %d", c);
	switch(c)
	{
		case 2:// A,B,C
			if (BrowseChar >= 'A' && BrowseChar < 'C')
				BrowseChar++;
			else
				BrowseChar = 'A';
			break;

		case 3:// D,E,F
			if (BrowseChar >= 'D' && BrowseChar < 'F')
				BrowseChar++;
			else
				BrowseChar = 'D';
			break;

		case 4:// G,H,I
			if (BrowseChar >= 'G' && BrowseChar < 'I')
				BrowseChar++;
			else
				BrowseChar = 'G';
		break;

		case 5:// J,K,L
			if (BrowseChar >= 'J' && BrowseChar < 'L')
				BrowseChar++;
			else
				BrowseChar = 'J';
			break;

		case 6:// M,N,O
			if (BrowseChar >= 'M' && BrowseChar < 'O')
				BrowseChar++;
			else
				BrowseChar = 'M';
			break;

		case 7:// P,Q,R,S
			if (BrowseChar >= 'P' && BrowseChar < 'S')
				BrowseChar++;
			else
				BrowseChar = 'P';
			break;

		case 8:// T,U,V
			if (BrowseChar >= 'T' && BrowseChar < 'V')
				BrowseChar++;
			else
				BrowseChar = 'T';
			break;

		case 9:// W,X,Y,Z
			if (BrowseChar >= 'W' && BrowseChar < 'Z')
				BrowseChar++;
			else
				BrowseChar = 'W';
			break;
	}
	if (BrowseChar != 0)
	{
		BrowseTimer.start(5000);
		services->beginAtomic();
		services->forEachEntry(moveFirstChar(BrowseChar));
		services->endAtomic();
	}
}

struct updateEPGChangedService: public std::unary_function<eListBoxEntryService&, void>
{
	int cnt;
	eEPGCache* epg;
	const tmpMap *updatedEntrys;
	bool redrawOnly;
	updateEPGChangedService( const tmpMap *u, bool redrawOnly=false ):
		cnt(0), epg(eEPGCache::getInstance()), updatedEntrys(u), redrawOnly(redrawOnly)
	{
	}

	bool operator()(eListBoxEntryService& l)
	{
		if ( l.service.type == eServiceReference::idDVB && !( l.service.flags & eServiceReference::isDirectory) )
		{
			tmpMap::const_iterator it;
			if (updatedEntrys)
			 it = updatedEntrys->find( (const eServiceReferenceDVB&)l.service );
			if ( (updatedEntrys && it != updatedEntrys->end()) )  // entry is updated
			{
				EITEvent *e=eEPGCache::getInstance()->lookupEvent((const eServiceReferenceDVB&)l.service );
				if (e)
				{
					if ( e->event_id != l.curEventId )
					{
						if ( redrawOnly )
							((eListBox<eListBoxEntryService>*) l.listbox)->invalidateEntry(cnt);
						else
							l.invalidateDescr();
					}
					delete e;
				}
			}
			cnt++;
		}
		return 0;
	}
};

void eServiceSelector::EPGUpdated( const tmpMap *m)
{
	services->forEachEntry( updateEPGChangedService( m ) );
	services->forEachVisibleEntry( updateEPGChangedService( m, true ) );
}

void eServiceSelector::pathUp()
{
	if (!movemode)
	{
		if (path.size() > ( (style == styleCombiColumn) ? focus==bouquets? 2 : 3 : 1) )
		{
			services->beginAtomic();
			eServiceReference last=path.current();
			path.up();
			actualize();
			selectService( last );
			services->endAtomic();
		} else if ( style == styleCombiColumn && bouquets->isVisible() )
			setFocus(bouquets);
	}
}

void eServiceSelector::serviceSelected(eListBoxEntryService *entry)
{
	if (entry)
	{
		if (entry->flags & eListBoxEntryService::flagIsReturn)
		{
			pathUp();
			return;
		}
		eServiceReference ref=entry->service;

		if (plockmode)
		{
			int doit=1;
			if ( ref.flags & eServiceReference::isDirectory )
			{
				hide();
				eMessageBox mb(_("Select No or press Abort to lock/unlock the complete directory"), _("Enter Directory"),  eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion, eMessageBox::btNo );
				mb.show();
				if ( mb.exec() == eMessageBox::btYes )
					doit=0;
				mb.hide();
				show();
			}
			if ( doit )
			{
				if ( ref.isLocked() )
					ref.unlock();
				else
					ref.lock();
				invalidateCurrent();
			}
			else
				enterDirectory(ref);
			return;
		}

		if (movemode)
		{
			const std::set<eString> &styles =
				eActionMapList::getInstance()->getCurrentStyles();

			if (eListBoxEntryService::selectedToMove)
			{
				eListBoxEntryService *next=services->getNext();
				/*emit*/moveEntry(path.current(), ref, next ? next->service : eServiceReference());
				services->setMoveMode(0);
				eListBoxEntryService::selectedToMove=0;
				services->beginAtomic();
				actualize();
				selectService(ref);
				services->endAtomic();
				if ( styles.find("sselect_edit") != styles.end() )
					eZapMain::getInstance()->toggleMoveMode(this);
				return;
			}
			else if ( ref )
			{
				services->setMoveMode(1);
				eListBoxEntryService::selectedToMove=entry;
				services->invalidateCurrent();
				return;
			}
		}

		if (entry->service.flags & eServiceReference::isMarker )
			return;

		if (ref.flags & eServiceReference::isDirectory)
			enterDirectory(ref);
		else if (editMode) // edit user bouquet mode
		{
			eServiceReference &ref = services->getCurrent()->service;
			std::set<eServiceReference>::iterator it = eListBoxEntryService::hilitedEntrys.find( ref );
			if ( it == eListBoxEntryService::hilitedEntrys.end() )
			{
				/*emit*/ addServiceToUserBouquet(&selected, 1);
				eListBoxEntryService::hilitedEntrys.insert(ref);
			}
			else
			{
				/*emit*/ removeServiceFromUserBouquet( this );
				eListBoxEntryService::hilitedEntrys.erase(ref);
			}
			services->invalidateCurrent();
			return;
		}
		else
		{
			result=&entry->service;
			close(0);
		}
	}
}

void eServiceSelector::bouquetSelected(eListBoxEntryService*)
{
	if ( services->isVisible() )
		setFocus(services);
}

void eServiceSelector::serviceSelChanged(eListBoxEntryService *entry)
{
	if (entry)
	{
		selected = (((eListBoxEntryService*)entry)->service);
		ci->clear();

		if ( selected.type == eServiceReference::idDVB &&
					(!(selected.flags & eServiceReference::flagDirectory)))
			ciDelay.start(selected.path.size() ? 100 : 500, true );
	}
}

void eServiceSelector::updateCi()
{
	ci->update((const eServiceReferenceDVB&)selected);
}

void eServiceSelector::forEachServiceRef( Signal1<void,const eServiceReference&> callback, bool fromBeg )
{
	eListBoxEntryService *safe = services->getCurrent(),
											 *p, *beg;
	if ( fromBeg )
	{
		services->moveSelection( eListBoxBase::dirFirst );
		beg = services->getCurrent();
	}
	else
		beg = safe;
	p = beg;
	do
	{
		if (!p)
			break;
		if ( !(p->flags & eListBoxEntryService::flagIsReturn) )
			callback(p->service);
		p=services->goNext();
	}
	while ( p && p != beg );

	if ( fromBeg )
		services->setCurrent(safe);
}

int eServiceSelector::eventHandler(const eWidgetEvent &event)
{
	int num=0;
	eServicePath enterPath;
	switch (event.type)
	{
		case eWidgetEvent::evtAction:
			if (event.action == &i_numberActions->key1 && !movemode && !editMode)
			{
				if ( serviceentryflags&eListBoxEntryService::flagOwnNumber )
					num=1;
			}
			else if (event.action == &i_numberActions->key2 && !movemode && !editMode)
			{
				if ( serviceentryflags&eListBoxEntryService::flagOwnNumber )
					num=2;
				else
					gotoChar(2);
			}
			else if (event.action == &i_numberActions->key3 && !movemode && !editMode)
			{
				if ( serviceentryflags&eListBoxEntryService::flagOwnNumber )
					num=3;
				else
					gotoChar(3);
			}
			else if (event.action == &i_numberActions->key4 && !movemode && !editMode)
			{
				if ( serviceentryflags&eListBoxEntryService::flagOwnNumber )
					num=4;
				else
					gotoChar(4);
			}
			else if (event.action == &i_numberActions->key5 && !movemode && !editMode)
			{
				if ( serviceentryflags&eListBoxEntryService::flagOwnNumber )
					num=5;
				else
					gotoChar(5);
			}
			else if (event.action == &i_numberActions->key6 && !movemode && !editMode)
			{
				if ( serviceentryflags&eListBoxEntryService::flagOwnNumber )
					num=6;
				else
					gotoChar(6);
			}
			else if (event.action == &i_numberActions->key7 && !movemode && !editMode)
			{
				if ( serviceentryflags&eListBoxEntryService::flagOwnNumber )
					num=7;
				else
					gotoChar(7);
			}
			else if (event.action == &i_numberActions->key8 && !movemode && !editMode)
			{
				if ( serviceentryflags&eListBoxEntryService::flagOwnNumber )
					num=8;
				else
					gotoChar(8);
			}
			else if (event.action == &i_numberActions->key9 && !movemode && !editMode)
			{
				if ( serviceentryflags&eListBoxEntryService::flagOwnNumber )
					num=9;
				else
					gotoChar(9);
			}
			else if (event.action == &i_serviceSelectorActions->prevBouquet && !movemode && path.size() > 1)
			{
				ci->clear();
				services->beginAtomic();
				if (style == styleCombiColumn)
				{
					if (bouquets->goPrev()->flags & eListBoxEntryService::flagIsReturn)
						bouquets->goPrev();
				} else
				{
					eServiceReference last=path.current();
					path.up();
					fillServiceList(path.current());
					selectService( last );
					eListBoxEntryService* p = services->goPrev();
					if (p && (p->flags&eListBoxEntryService::flagIsReturn))
						p=services->goPrev();
					if (p)
					{
						path.down( p->service );
						fillServiceList( p->service );
						selectService( eServiceInterface::getInstance()->service );
					}
				}
				services->endAtomic();
			}
			else if (event.action == &i_serviceSelectorActions->nextBouquet && !movemode && path.size()>1 )
			{
				ci->clear();
				services->beginAtomic();
				if (style == styleCombiColumn)
				{
					if (bouquets->goNext()->flags & eListBoxEntryService::flagIsReturn)
						bouquets->goNext();
				} else
				{
					eServiceReference last=path.current();
					path.up();
					fillServiceList(path.current());
					selectService( last );
					eListBoxEntryService* p = services->goNext();
					if (p && (p->flags&eListBoxEntryService::flagIsReturn))
						p=services->goNext();
					if (p)
					{
						path.down( p->service );
						fillServiceList( p->service );
						selectService( eServiceInterface::getInstance()->service );
					}
				}
				services->endAtomic();
			}
			else if (event.action == &i_serviceSelectorActions->showEPGSelector
				&& !movemode && !editMode && this == eZap::getInstance()->getServiceSelector() )
			{
				hide();
				eEPGStyleSelector e;
#ifndef DISABLE_LCD
				e.setLCD( LCDTitle, LCDElement );
#endif
				e.show();
				int ret = e.exec();
				e.hide();
				switch ( ret )
				{
					case 1:
						/*emit*/ showEPGList((eServiceReferenceDVB&)selected);
						show();
						break;
					case 2:
						showMultiEPG();
						break;
					default:
						show();
						break;
				}
			}
			else if (event.action == &i_cursorActions->help
				&& this != eZap::getInstance()->getServiceSelector() )
				;  // dont show help when this is not the main service selector
			else if (event.action == &i_serviceSelectorActions->pathUp)
				pathUp();
			else if (event.action == &i_serviceSelectorActions->toggleStyle && !movemode && !editMode)
			{
				int newStyle = lastSelectedStyle;
				if (newStyle == styleMultiColumn)
					newStyle = styleCombiColumn;
				else
					newStyle++;
				setStyle(lastSelectedStyle=newStyle);
			}
			else if (event.action == &i_serviceSelectorActions->toggleFocus && !movemode && path.size() > 1)
			{
				if ( style == styleCombiColumn )
					if (focus == services)
						setFocus( bouquets );
					else
						setFocus( services );
			}
			else if (event.action == &i_serviceSelectorActions->showMenu && focus != bouquets && !plockmode )
			{
				hide();
				/*emit*/ showMenu(this);
				show();
			}
			else if (event.action == &i_serviceSelectorActions->addService && !movemode && !editMode)
				/*emit*/ addServiceToPlaylist(selected);
			else if (event.action == &i_serviceSelectorActions->addServiceToUserBouquet && !movemode && !editMode)
			{
				hide();
				/*emit*/ addServiceToUserBouquet(&selected, 0);
				show();
			}
			else if (event.action == &i_serviceSelectorActions->modeTV && !movemode && !editMode)
				/*emit*/ setMode(eZapMain::modeTV);
			else if (event.action == &i_serviceSelectorActions->modeRadio && !movemode && !editMode)
				/*emit*/ setMode(eZapMain::modeRadio);
#ifndef DISABLE_FILE
			else if (event.action == &i_serviceSelectorActions->modeFile && !movemode && !editMode)
				/*emit*/ setMode(eZapMain::modeFile);
#endif
			else if (event.action == &i_serviceSelectorActions->gotoPrevMarker)
			{
				ePlaylist *p = 0;
				if ( path.current().type == eServicePlaylistHandler::ID )
					p = (ePlaylist*) eServicePlaylistHandler::getInstance()->addRef(path.current());
				if ( p )
				{
					std::list<ePlaylistEntry>::const_iterator it =
						std::find( p->getConstList().begin(), p->getConstList().end(), selected );
					if ( it != p->getConstList().end() )
					{
						for (--it ; it != p->getConstList().end(); --it )
							if ( it->service.flags & eServiceReference::isMarker )
							{
								selectService( it->service );
								break;
							}
					}
					if ( it == p->getConstList().end() )
						services->moveSelection(services->dirFirst);
				}
				else
					services->moveSelection(services->dirFirst);
			}
			else if (event.action == &i_serviceSelectorActions->gotoNextMarker)
			{
				ePlaylist *p = 0;
				if ( path.current().type == eServicePlaylistHandler::ID )
					p = (ePlaylist*) eServicePlaylistHandler::getInstance()->addRef(path.current());
				if ( p )
				{
					std::list<ePlaylistEntry>::const_iterator it;
					eListBoxEntryService *cur = services->getCurrent();
					if ( cur && cur->flags & eListBoxEntryService::flagIsReturn )
						it = p->getConstList().begin();
					else
						it = std::find( p->getConstList().begin(), p->getConstList().end(), selected );
					if ( it != p->getConstList().end() )
					{
						for ( ++it ; it != p->getConstList().end(); ++it )
							if ( it->service.flags & eServiceReference::isMarker )
							{
								selectService( it->service );
								break;
							}
					}
					if ( it == p->getConstList().end() )
						services->moveSelection(services->dirLast);
				}
				else
					services->moveSelection(services->dirLast);
			}
			else if (event.action == &i_serviceSelectorActions->showAll && !movemode)
			{
				enterPath = /*emit*/ getRoot(listAll);
				if ( style == styleCombiColumn && eZapMain::getInstance()->getMode() == eZapMain::modeFile )
					enterPath.down(eServiceReference());
			}
			else if (event.action == &i_serviceSelectorActions->showSatellites && !movemode)
			{
				if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
				{
					enterPath = /*emit*/ getRoot(listSatellites);
					if ( style == styleCombiColumn && eZapMain::getInstance()->getMode() != eZapMain::modeFile )
						enterPath.down(eServiceReference());
				}
			}
			else if (event.action == &i_serviceSelectorActions->showProvider && !movemode)
			{
				enterPath = /*emit*/ getRoot(listProvider);
				if ( style == styleCombiColumn && eZapMain::getInstance()->getMode() != eZapMain::modeFile )
					enterPath.down(eServiceReference());
			}
			else if (event.action == &i_serviceSelectorActions->showBouquets && !movemode)
			{
				enterPath = /*emit*/ getRoot(listBouquets);
				if ( style == styleCombiColumn )
					enterPath.down(eServiceReference());
			}
			else if (event.action == &i_cursorActions->cancel)
			{
				if (movemode)
					eZapMain::getInstance()->toggleMoveMode(this);
				if (editMode)
					eZapMain::getInstance()->toggleEditMode(this);
				break;
			}
			else if ( event.action == &i_serviceSelectorActions->markPressed )
			{
				if ( services->getCurrent() && !(services->getCurrent()->flags & eListBoxEntryService::flagIsReturn) )
				{
					if (path.current().type == eServicePlaylistHandler::ID)
					{
						if ( movemode )
							serviceSelected( services->getCurrent() );
						else
						{
							eZapMain::getInstance()->toggleMoveMode(this);
							serviceSelected( services->getCurrent() );
						}
					}
				}
			}
			else if ( event.action == &i_serviceSelectorActions->deletePressed )
				/*emit*/ deletePressed( this );
			else if ( event.action == &i_serviceSelectorActions->renamePressed )
			{
				hide();
				if ( selected.type == eServicePlaylistHandler::ID )
					/*emit*/ renameBouquet( this );
				else
					/*emit*/ renameService( this );
				show();
			}
			else if ( event.action == &i_serviceSelectorActions->newMarkerPressed )
			{
				if ( path.current().type == eServicePlaylistHandler::ID )
					/*emit*/ newMarkerPressed( this );
			}
			else
				break;
			if (enterPath.size())
			{
				ci->clear();
				if ( path.bottom() == enterPath.bottom() )
					pathUp();
				else
					setPath(enterPath);
			}
			else if (num)
			{
				hide();
				eServiceNumberWidget s(num);
				s.show();
				num = s.exec();
				s.hide();
				if (num != -1)
				{
					if (selectService( num ))
					{
						result=&services->getCurrent()->service;
						close(0);
					}
					else
						show();
				}
				else
					show();
			}
			return 1;
		default:
			break;
	}
	return eWindow::eventHandler(event);
}

struct _selectService: public std::unary_function<const eListBoxEntryService&, void>
{
	eServiceReference service;

	_selectService(const eServiceReference& e): service(e)
	{
	}

	bool operator()(const eListBoxEntryService& s)
	{
		if (service == s.service)
		{
			((eListBox<eListBoxEntryService>*) s.listbox)->setCurrent(&s);
			return 1;
		}
		return 0;
	}
};

struct copyEntry: public std::unary_function<const eListBoxEntryService&, void>
{
	std::list<eServiceReference> &dest;

	copyEntry(std::list<eServiceReference> &dest)
		:dest(dest)
	{
	}

	bool operator()(const eListBoxEntryService& s)
	{
		dest.push_back(s.service);
		return 0;
	}
};

bool eServiceSelector::selectServiceRecursive( eServiceReference &ref )
{
	services->beginAtomic();
	bool b = selServiceRec( ref );
	services->endAtomic();
	return b;
}

bool eServiceSelector::selServiceRec( eServiceReference &ref )
{
	std::list<eServiceReference> tmp;

	// copy all entrys to temp list
	services->forEachEntry( copyEntry( tmp ) );

	for ( std::list<eServiceReference>::iterator it( tmp.begin() ); it != tmp.end(); it++ )
	{
		if ( it->flags & eServiceReference::isDirectory )
		{
			path.down(*it);
			actualize();
			if ( selServiceRec( ref ) )
				return true;
			else
			{
				path.up();
				actualize();
			}
		}
		else if ( selectService(ref) )
			return true;
	}
	return false;
}

bool eServiceSelector::selectService(const eServiceReference &ref)
{
	if ( services->forEachEntry( _selectService(ref) ) )
	{
//		services->moveSelection( eListBox<eListBoxEntryService>::dirFirst );
		// ersten service NICHT selecten (warum auch - evtl. ist ja der aktuelle sinnvoller,
		// und bei einem entsprechenden returncode kann ja jeder sehen was er will)
		return false;
	}
	else
		return true;
}

void eServiceSelector::setStyle(int newStyle, bool force)
{
	eServicePath p = path;
	eServiceReference currentService;
	if (style != newStyle || force )
	{
		ci->hide();
		if ( services )
		{
			// save currentSelected Service
			if ( services->getCount() )
				currentService = services->getCurrent()->service;

			services->hide();
			delete services;
		}
		if ( bouquets )
		{
			bouquets->hide();
			delete bouquets;
		}

		if (key[0])
			delete key[0];
		if (key[1])
			delete key[1];
		if (key[2])
			delete key[2];
		if (key[3])
			delete key[3];

		if (newStyle == styleSingleColumn)
		{
			eListBoxEntryService::folder = eSkin::getActive()->queryImage("sselect_folder");
			eListBoxEntryService::marker = eSkin::getActive()->queryImage("sselect_marker");
			eListBoxEntryService::locked = eSkin::getActive()->queryImage("sselect_locked");
			eListBoxEntryService::numberFont = eSkin::getActive()->queryFont("eServiceSelector.singleColumn.Entry.Number");
			eListBoxEntryService::serviceFont = eSkin::getActive()->queryFont("eServiceSelector.singleColumn.Entry.Name");
		}
		else if (newStyle == styleMultiColumn)
		{
			eListBoxEntryService::folder = 0;
			eListBoxEntryService::marker = eSkin::getActive()->queryImage("sselect_marker_small");
			eListBoxEntryService::locked = eSkin::getActive()->queryImage("sselect_locked_small");
			eListBoxEntryService::numberFont = eSkin::getActive()->queryFont("eServiceSelector.multiColumn.Entry.Number");
			eListBoxEntryService::serviceFont = eSkin::getActive()->queryFont("eServiceSelector.multiColumn.Entry.Name");
		}
		else
		{
			eListBoxEntryService::folder = 0;
			eListBoxEntryService::marker = eSkin::getActive()->queryImage("sselect_marker_small");
			eListBoxEntryService::locked = eSkin::getActive()->queryImage("sselect_locked_small");
			eListBoxEntryService::numberFont = eSkin::getActive()->queryFont("eServiceSelector.combiColumn.Entry.Number");
			eListBoxEntryService::serviceFont = eSkin::getActive()->queryFont("eServiceSelector.combiColumn.Entry.Name");
		}
		services = new eListBox<eListBoxEntryService>(this);
		services->setName("services");
		services->setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight.background"), eSkin::getActive()->queryScheme("eServiceSelector.highlight.foreground"));
		services->hide();

		bouquets = new eListBox<eListBoxEntryService>(this);
		bouquets->setName("bouquets");
		bouquets->setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight.background"), eSkin::getActive()->queryScheme("eServiceSelector.highlight.foreground"));
		bouquets->hide();

		eString styleName="eServiceSelector_";
		if ( newStyle == styleSingleColumn )
			styleName+="singleColumn";
		else if ( newStyle == styleMultiColumn )
			styleName+="multiColumn";
		else
		{
			styleName+="combiColumn";
			CONNECT( bouquets->selchanged, eServiceSelector::bouquetSelChanged );
			CONNECT( bouquets->selected, eServiceSelector::bouquetSelected );
			bouquets->show();
		}
		int showButtons=0;
		eConfig::getInstance()->getKey("/ezap/serviceselector/showButtons", showButtons );
		if ( showButtons )
		{
			styleName+="_buttons";
			key[0] = new eLabel(this);
			key[0]->setName("key_red");
			key[1] = new eLabel(this);
			key[1]->setName("key_green");
			key[2] = new eLabel(this);
			key[2]->setName("key_yellow");
			key[3] = new eLabel(this);
			key[3]->setName("key_blue");
			for (int i=0; i < 4; i++)
				key[i]->show();
		}
		else
			key[0] = key[1] = key[2] = key[3] = 0;

		if (eSkin::getActive()->build(this, styleName.c_str()))
			eFatal("Service selector widget \"%s\" build failed!",styleName.c_str());

		style = newStyle;
		CONNECT(services->selected, eServiceSelector::serviceSelected);
		CONNECT(services->selchanged, eServiceSelector::serviceSelChanged);
		actualize();
		selectService( currentService );  // select the old service
		services->show();
		if ( services->isVisible() )
			setFocus(services);
		setKeyDescriptions();
 }
	ci->show();
}

void eServiceSelector::bouquetSelChanged( eListBoxEntryService *entry)
{
	if ( entry && entry->service )
	{
		ci->clear();
		services->beginAtomic();
		path.up();
		path.down(entry->service);
		fillServiceList( entry->service );
		if (!selectService( eServiceInterface::getInstance()->service ))
			services->moveSelection( eListBox<eListBoxEntryService>::dirFirst );
		services->endAtomic();
	}
}

void eServiceSelector::actualize()
{
	if (style == styleCombiColumn)
	{
		bouquets->beginAtomic();
		if ( path.size() > 1 )
		{
			eServiceReference currentBouquet = path.current();
			path.up();
			eServiceReference allBouquets = path.current();
			path.down( currentBouquet );
			fillBouquetList( allBouquets );

			if ( bouquets->forEachEntry( _selectService( currentBouquet ) ) )
				bouquets->moveSelection( eListBox<eListBoxEntryService>::dirFirst );
		}
		else
		{
			bouquets->clearList();
			fillServiceList(path.current());
		}
		bouquets->endAtomic();
	}
	else
		fillServiceList(path.current());
}

eServiceSelector::eServiceSelector()
	:eWindow(0), result(0), services(0), bouquets(0)
	,style(styleInvalid), lastSelectedStyle(styleSingleColumn)
	,BrowseChar(0), BrowseTimer(eApp), ciDelay(eApp), movemode(0)
	,editMode(0), plockmode(0)
{
	ci = new eChannelInfo(this);
	ci->setName("channelinfo");

	CONNECT(eDVB::getInstance()->bouquetListChanged, eServiceSelector::actualize);
	CONNECT(BrowseTimer.timeout, eServiceSelector::ResetBrowseChar);
	CONNECT(ciDelay.timeout, eServiceSelector::updateCi );
	CONNECT(eEPGCache::getInstance()->EPGUpdated, eServiceSelector::EPGUpdated);

	addActionMap(&i_serviceSelectorActions->map);
	addActionMap(&i_numberActions->map);

	setHelpID(33);
	addActionToHelpList(&i_serviceSelectorActions->deletePressed);
	addActionToHelpList(&i_serviceSelectorActions->markPressed);
	addActionToHelpList(&i_serviceSelectorActions->renamePressed);
	addActionToHelpList(&i_serviceSelectorActions->newMarkerPressed);
	addActionToHelpList(&i_serviceSelectorActions->showAll);
	if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
		addActionToHelpList(&i_serviceSelectorActions->showSatellites);
	addActionToHelpList(&i_serviceSelectorActions->showProvider);
	addActionToHelpList(&i_serviceSelectorActions->showBouquets);
	addActionToHelpList(&i_serviceSelectorActions->showMenu);
	addActionToHelpList(&i_serviceSelectorActions->toggleStyle);
	addActionToHelpList(&i_serviceSelectorActions->toggleFocus);
	addActionToHelpList(&i_serviceSelectorActions->gotoPrevMarker);
	addActionToHelpList(&i_serviceSelectorActions->gotoNextMarker);
	addActionToHelpList(&i_serviceSelectorActions->showEPGSelector);
	addActionToHelpList(&i_serviceSelectorActions->pathUp);
	addActionToHelpList(&i_serviceSelectorActions->modeTV);
	addActionToHelpList(&i_serviceSelectorActions->modeRadio);
#ifndef DISABLE_FILE
	addActionToHelpList(&i_serviceSelectorActions->modeFile);
#endif
	
	key[0] = key[1] = key[2] = key[3] = 0;

	eActionMapList::getInstance()->activateStyle("sselect_default");
}

eServiceSelector::~eServiceSelector()
{
}

void eServiceSelector::enterDirectory(const eServiceReference &ref)
{
	path.down(ref);
	doSPFlags(ref);
	services->beginAtomic();
	actualize();
	if (!selectService( eServiceInterface::getInstance()->service ))
		services->moveSelection( eListBox<eListBoxEntryService>::dirFirst );

	// we have a problem when selection not changes..
	// the listbox don't emit "selected"... then our current
	// selected entry is not valid.. to prevent this we set
	// it manual...
	eListBoxEntryService *cur = services->getCurrent();
	if ( cur )
		selected = cur->service;
	else
		selected = eServiceReference();

	services->endAtomic();
}

void eServiceSelector::showMultiEPG()
{
	eZapEPG epg;

	epg.move(ePoint(50, 50));
	epg.resize(eSize(620, 470));

	int direction = 2;
	epg.show();
	do
	{
		epg.buildPage(direction);
		direction = epg.exec();
	}
	while ( direction > 0 );
	epg.hide();
	if ( !direction ) // switch to service requested...
	{
		selectService( epg.getCurSelected()->service );
		result=&selected;
		close(0);
	}
	else
		show();
}

void eServiceSelector::doSPFlags(const eServiceReference &ref)
{
#if 0
	const eService *pservice=eServiceInterface::getInstance()->addRef(ref);
	if (pservice)
	{
		switch (pservice->spflags & eService::spfColMask)
		{
		case eService::spfColSingle:
			setStyle(styleSingleColumn);
			break;
		case eService::spfColMulti:
			setStyle(styleMultiColumn);
			break;
		case eService::spfColCombi:
			setStyle(styleCombiColumn);
			break;
		case eService::spfColDontChange:
			setStyle(lastSelectedStyle);
			break;
		}
	}
	eServiceInterface::getInstance()->removeRef(ref);
#endif
}

void eServiceSelector::ResetBrowseChar()
{
	BrowseChar=0;
}

const eServiceReference *eServiceSelector::choose(int irc)
{
	ASSERT(this);
	services->beginAtomic();
	result=0;

	switch (irc)
	{
	case dirUp:
		services->moveSelection(eListBox<eListBoxEntryService>::dirUp);
		break;
	case dirDown:
		services->moveSelection(eListBox<eListBoxEntryService>::dirDown);
		break;
	case dirFirst:
		services->moveSelection(eListBox<eListBoxEntryService>::dirFirst);
		break;
	case dirLast:
		services->moveSelection(eListBox<eListBoxEntryService>::dirLast);
		break;
	default:
		break;
	}
	services->endAtomic();

	if ( !services->getCount() )
		ci->clear();

	show();

	if (exec())
		result=0;

	hide();
	return result;
}

const eServiceReference *eServiceSelector::next()
{
	services->beginAtomic();
	selectService(eServiceInterface::getInstance()->service);

	eListBoxEntryService *s=0, *cur=services->getCurrent();
	do
		s=services->goNext();
	while ( s != cur && s &&
			( s->flags & eListBoxEntryService::flagIsReturn ||
				s->service.flags == eServiceReference::isMarker ) );

	services->endAtomic();
	if (s)
		return &s->service;
	else
		return 0;
}

const eServiceReference *eServiceSelector::prev()
{
	services->beginAtomic();
	selectService(eServiceInterface::getInstance()->service);
	eListBoxEntryService *s=0, *cur=services->getCurrent();
	do
		s=services->goPrev();
	while ( s != cur && s &&
			( s->flags & eListBoxEntryService::flagIsReturn ||
				s->service.flags & eServiceReference::isMarker ) );

	services->endAtomic();
	if (s)
		return &s->service;
	else
		return 0;
}

void eServiceSelector::setPath(const eServicePath &newpath, const eServiceReference &select)
{
	path=newpath;
	doSPFlags(path.current());
	if (services)
	{
		services->beginAtomic();
		actualize();
		selectService(select);
		services->endAtomic();
	}
}

void eServiceSelector::removeCurrent(bool selNext)
{
	services->beginAtomic();
	eListBoxEntryService *cur = services->getCurrent();
	if ( !cur )
		return;
	if ( selNext )
		services->goNext();
	else
		services->goPrev();
	services->remove( cur, true );
	services->endAtomic();
}

void eServiceSelector::invalidateCurrent( eServiceReference ref )
{
	eListBoxEntryService *cur = services->getCurrent();
	if ( !cur )
		return;
	if ( ref )
		cur->service = ref;
	cur->invalidate();
	services->invalidateCurrent();
}

int eServiceSelector::toggleMoveMode()
{
	services->beginAtomic();

	movemode^=1;
	if (!movemode)
	{
		eListBoxEntryService::selectedToMove=0;
		services->setMoveMode(0);
		if ( goUpEntry )
			services->append( goUpEntry, true, true );
	}
	else if ( goUpEntry )
		services->remove(goUpEntry, true);

	services->endAtomic();
	return movemode;
}

int eServiceSelector::toggleEditMode()
{
	editMode^=1;
	return editMode;
}

#include <algorithm>
#include <list>

#include <enigma.h>
#include <enigma_main.h>
#include <sselect.h>
#include <epgwindow.h>

#include <lib/base/i18n.h>
#include <lib/gdi/font.h>
#include <lib/gui/actions.h>
#include <lib/gui/eskin.h>
#include <lib/gui/elabel.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/epgcache.h>
#include <lib/driver/rc.h>
#include <lib/system/init.h>
#include <lib/dvb/service.h>
#include <lib/gui/numberactions.h>

gFont eListBoxEntryService::serviceFont;
gFont eListBoxEntryService::descrFont;
gFont eListBoxEntryService::numberFont;
gPixmap *eListBoxEntryService::folder=0;
int eListBoxEntryService::maxNumSize=0;
std::map< eServiceReference, int> eListBoxEntryService::favourites;
eListBoxEntryService *eListBoxEntryService::selectedToMove=0;

struct serviceSelectorActions
{
	eActionMap map;
	eAction nextBouquet, prevBouquet, pathUp, showEPGSelector, showMenu, showFavourite, addService, addServiceToFavourite, modeTV, modeRadio, modeFile, toggleStyle, toggleFocus;
	serviceSelectorActions():
		map("serviceSelector", _("service selector")),
		nextBouquet(map, "nextBouquet", _("switch to next bouquet"), eAction::prioDialogHi),
		prevBouquet(map, "prevBouquet", _("switch to previous bouquet"), eAction::prioDialogHi),
		pathUp(map, "pathUp", _("go one dir path up"), eAction::prioDialog),
		showEPGSelector(map, "showEPGSelector", _("shows the EPG selector for the highlighted channel"), eAction::prioDialog),
		showMenu(map, "showMenu", _("show service selector menu"), eAction::prioDialog),
		showFavourite(map, "showFavourite", _("showFavourite"), eAction::prioDialog),
		addService(map, "addService", _("add Service"), eAction::prioDialog),
		addServiceToFavourite(map, "addServiceToFavourite", _("add Service to Favourite"), eAction::prioDialog),
		modeTV(map, "modeTV", _("switch to TV mode"), eAction::prioDialog),
		modeRadio(map, "modeRadio", _("switch to Radio mode"), eAction::prioDialog),
		modeFile(map, "modeFile", _("switch to File mode"), eAction::prioDialog),
		toggleStyle(map, "toggleStyle", _("toggle between classic and multi column style"), eAction::prioDialog),
		toggleFocus(map, "toggleFocus", _("toggle focus between service and bouquet list"), eAction::prioDialog)
	{
	}
};
eAutoInitP0<serviceSelectorActions> i_serviceSelectorActions(5, "service selector actions");

eListBoxEntryService::eListBoxEntryService(eListBox<eListBoxEntryService> *lb, const eServiceReference &service)
	:eListBoxEntry((eListBox<eListBoxEntry>*)lb),	numPara(0), namePara(0), descrPara(0), nameXOffs(0), service(service)
{
#if 0
	sort=eString().sprintf("%06d", service->service_number);
#else
	const eService *pservice=eServiceInterface::getInstance()->addRef(service);
	sort=pservice?pservice->service_name:"";
	sort.upper();
	eServiceInterface::getInstance()->removeRef(service);
#endif
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

eString eListBoxEntryService::redraw(gPainter *rc, const eRect &rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int hilited)
{
	bool b;

	if ( (b = (hilited == 2)) )
		hilited = 0;

	if (this == selectedToMove)
		drawEntryRect(rc, rect, eSkin::getActive()->queryColor("eServiceSelector.entrySelectedToMove"), coActiveF, coNormalB, coNormalF, hilited );
	else  
		drawEntryRect(rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, hilited );

	eString sname;
	eString sdescr;
	const eService *pservice=eServiceInterface::getInstance()->addRef(service);

	if (pservice)
		sname=pservice->service_name;
	else
		sname="(removed service)";

	std::map< eServiceReference, int>::iterator it = favourites.find( service );
	if ( it != favourites.end() )
		rc->setForegroundColor( eSkin::getActive()->queryColor("eServiceSelector.entryInFavourite") );

	if ( service.flags & eServiceReference::isDirectory && folder )  // we draw the folder pixmap
	{
		nameXOffs = folder->x + 20;
		int ypos = (rect.height() - folder->y) / 2;
		rc->blit( *folder, ePoint(10, rect.top()+ypos ), eRect(), gPixmap::blitAlphaTest);		
	}
	else if (!service.flags & eServiceReference::isDirectory)
	{
		if (!numPara)
		{
			numPara = new eTextPara( eRect( 0, 0, maxNumSize, rect.height() ) );
			numPara->setFont( numberFont );
			numPara->renderString( eString().setNum(num) );
			numPara->realign(eTextPara::dirRight);
			numYOffs = ((rect.height() - numPara->getBoundBox().height()) / 2 ) - numPara->getBoundBox().top();
			nameXOffs = maxNumSize+numPara->getBoundBox().height();
		}
		rc->renderPara(*numPara, ePoint( rect.left(), rect.top() + numYOffs ) );
	}

	if (!namePara)
	{
		namePara = new eTextPara( eRect( 0, 0, rect.width(), rect.height() ) );
		namePara->setFont( serviceFont );
		namePara->renderString( sname );
		nameYOffs = ((rect.height() - namePara->getBoundBox().height()) / 2 ) - namePara->getBoundBox().top();	
	}
	// we can always render namePara
	rc->renderPara(*namePara, ePoint( rect.left() + nameXOffs, rect.top() + nameYOffs ) );

	if ( listbox->getColumns() == 1 && 
				service.type == eServiceReference::idDVB &&
				(!(service.flags & eServiceReference::isDirectory)) &&
				(!service.path.size()) )
	{
		if (pservice && service.type == eServiceReference::idDVB && !(service.flags & eServiceReference::isDirectory) )
		{
			EITEvent *e=eEPGCache::getInstance()->lookupEvent((const eServiceReferenceDVB&)service);

			if (e)
			{
				for (ePtrList<Descriptor>::iterator d(e->descriptor); d != e->descriptor.end(); ++d)
				{
					Descriptor *descriptor=*d;

					if (descriptor->Tag()==DESCR_SHORT_EVENT)
					{
						ShortEventDescriptor *ss=(ShortEventDescriptor*)descriptor;
						sdescr=ss->event_name;
						break;
					}
				}
				delete e;
			}
			eServiceInterface::getInstance()->removeRef(service);
		}
		descrPara = new eTextPara( eRect( 0, 0, rect.width(), rect.height() ) );
		descrPara->setFont( descrFont );
		descrPara->renderString( sdescr );
		descrXOffs = nameXOffs+namePara->getBoundBox().width()+numPara->getBoundBox().height();
		descrYOffs = ((rect.height() - descrPara->getBoundBox().height()) / 2 ) - descrPara->getBoundBox().top();
	}
	if (descrPara)  // only render descr Para, when avail...
		rc->renderPara(*descrPara, ePoint( rect.left()+descrXOffs, rect.top() + descrYOffs ) );

	if (b)
	{
		rc->setForegroundColor(coActiveB);
		rc->line( ePoint(rect.left(), rect.bottom()-1), ePoint(rect.right()-1, rect.bottom()-1) );
		rc->line( ePoint(rect.left(), rect.top()), ePoint(rect.right()-1, rect.top()) );
		rc->line( ePoint(rect.left(), rect.top()), ePoint(rect.left(), rect.bottom()-1) );
		rc->line( ePoint(rect.right()-1, rect.top()), ePoint(rect.right()-1, rect.bottom()-1) );
		rc->line( ePoint(rect.left()+1, rect.bottom()-2), ePoint(rect.right()-2, rect.bottom()-2) );
		rc->line( ePoint(rect.left()+1, rect.top()+1), ePoint(rect.right()-2, rect.top()+1) );
		rc->line( ePoint(rect.left()+1, rect.top()+2), ePoint(rect.left()+1, rect.bottom()-3) );
		rc->line( ePoint(rect.right()-2, rect.top()+2), ePoint(rect.right()-2, rect.bottom()-3) );
		rc->line( ePoint(rect.left()+2, rect.bottom()-3), ePoint(rect.right()-3, rect.bottom()-3) );
		rc->line( ePoint(rect.left()+2, rect.top()+2), ePoint(rect.right()-3, rect.top()+2) );
		rc->line( ePoint(rect.left()+2, rect.top()+3), ePoint(rect.left()+2, rect.bottom()-4) );
		rc->line( ePoint(rect.right()-3, rect.top()+3), ePoint(rect.right()-3, rect.bottom()-4) );
	}

	return sort;
}

void eServiceSelector::addService(const eServiceReference &ref)
{
	new eListBoxEntryService(services, ref);
}

void eServiceSelector::addBouquet(const eServiceReference &ref)
{
	new eListBoxEntryService(bouquets, ref);
}

struct renumber: public std::unary_function<const eListBoxEntryService&, void>
{
	int &num;
	renumber(int &num):num(num)
	{
	}

	bool operator()(eListBoxEntryService& s)
	{
		if ( !(s.service.flags & eServiceReference::isDirectory) )
	 		s.num = ++num;
		return 0;
	}
};

void eServiceSelector::fillServiceList(const eServiceReference &_ref)
{
	eString windowDescr;
	eServicePath p = path;

	eServiceReference ref;
	do
	{
		const eService *pservice=eServiceInterface::getInstance()->addRef(p.current());
	  if (pservice && pservice->service_name.length() )
			windowDescr=pservice->service_name+" > "+windowDescr;
		ref = p.current();
		p.up();
		eServiceInterface::getInstance()->removeRef	(p.current());
	}
	while ( ref != p.current() );
	windowDescr.erase( windowDescr.rfind(">") );
	setText( windowDescr );	

	services->beginAtomic();
	services->clearList();

	eServiceInterface *iface=eServiceInterface::getInstance();
	ASSERT(iface);
	
	Signal1<void,const eServiceReference&> signal;
	CONNECT(signal, eServiceSelector::addService);
	
	ref=_ref;
	
	iface->enterDirectory(ref, signal);
	iface->leaveDirectory(ref);	// we have a copy.

	if (ref.flags & eServiceReference::shouldSort)
		services->sort();

	int num=0;
	services->forEachEntry( renumber(num) );

	// now we calc the x size of the biggest number we have;
	if (num)
	{
		eTextPara  *tmp = new eTextPara( eRect(0, 0, 100, 50) );
		tmp->setFont( eListBoxEntryService::numberFont );
		tmp->renderString( eString().setNum( num ) );
		eListBoxEntryService::maxNumSize = tmp->getBoundBox().width()+10;
		tmp->destroy();
	}
	else
		eListBoxEntryService::maxNumSize=10;

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
	switch(c)
	{
		case 2:	// A,B,C
			if (BrowseChar == 'A' || BrowseChar == 'B')
				BrowseChar++;
			else
				BrowseChar = 'A';
		break;

		case 3:	// D,E,F
			if (BrowseChar == 'D' || BrowseChar == 'E')
				BrowseChar++;
			else
				BrowseChar = 'D';
		break;

		case 4:	// G,H,I
			if (BrowseChar == 'G' || BrowseChar == 'H')
				BrowseChar++;
			else
				BrowseChar = 'G';
		break;
		case 5:	// J,K,L
			if (BrowseChar == 'J' || BrowseChar == 'M')
				BrowseChar++;
			else
				BrowseChar = 'J';
		break;

		case 6:	// M,N,O
			if (BrowseChar == 'M' || BrowseChar == 'N')
				BrowseChar++;
			else
				BrowseChar = 'M';
		break;

		case 7:	// P,Q,R,S
			if (BrowseChar >= 'P' && BrowseChar <= 'R')
				BrowseChar++;
			else
				BrowseChar = 'P';
		break;

		case 8:	// T,U,V
			if (BrowseChar == 'T' || BrowseChar == 'U')
				BrowseChar++;
			else
				BrowseChar = 'T';
		break;

		case 9:	// W,X,Y,Z
			if (BrowseChar >= 'W' && BrowseChar <= 'Y')
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
	updateEPGChangedService( const tmpMap *u ): cnt(0), epg(eEPGCache::getInstance()), updatedEntrys(u)
	{
	}

	bool operator()(eListBoxEntryService& l)
	{
		if ( l.service.type == eServiceReference::idDVB && !( l.service.flags & eServiceReference::isDirectory) )
		{
			uniqueEPGKey key( ((const eServiceReferenceDVB&)l.service).getServiceID().get(), ((const eServiceReferenceDVB&)l.service).getOriginalNetworkID().get() );
			tmpMap::const_iterator it;
			if (updatedEntrys)
			 it = updatedEntrys->find( key );
			if ( (updatedEntrys && it != updatedEntrys->end()) )  // entry is updated
			{
				l.invalidateDescr();
				((eListBox<eListBoxEntryService>*) l.listbox)->invalidateEntry(cnt);
			}
			cnt++;
		}
		return 0;
	}
};

void eServiceSelector::EPGUpdated( const tmpMap *m)
{
	services->forEachEntry( updateEPGChangedService( m ) );
}

void eServiceSelector::serviceSelected(eListBoxEntryService *entry)
{
	if (entry && entry->service)
	{
		const eServiceReference &ref=entry->service;

		if (movemode)
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
				return;
			}
			else
			{
				services->setMoveMode(1);
				eListBoxEntryService::selectedToMove=entry;
				services->invalidateCurrent();
				return;
			}

		if (ref.flags & eServiceReference::isDirectory)
			enterDirectory(ref);
		else if (!FavouriteMode)
		{
			result=&entry->service;
			close(0);
		}
	}
}

void eServiceSelector::bouquetSelected(eListBoxEntryService*)
{
	setFocus(services);
}

void eServiceSelector::serviceSelChanged(eListBoxEntryService *entry)
{
	if (entry)
	{
		selected = (((eListBoxEntryService*)entry)->service);
		if (ci->isVisible())				
		{
			ci->clear();
			if ( selected.type == eServiceReference::idDVB &&
						(!(selected.flags & eServiceReference::isDirectory)))
  			ciDelay.start(selected.path.size() ? 100 : 500, true );
		}
	}
}

void eServiceSelector::updateCi()
{
	ci->update((const eServiceReferenceDVB&)selected);
}

int eServiceSelector::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::evtAction:
			if (event.action == &i_numberActions->key2 && !movemode)
				gotoChar(2);
			else if (event.action == &i_numberActions->key3 && !movemode)
				gotoChar(3);
			else if (event.action == &i_numberActions->key4 && !movemode)
				gotoChar(4);
			else if (event.action == &i_numberActions->key5 && !movemode)
				gotoChar(5);
			else if (event.action == &i_numberActions->key6 && !movemode)
				gotoChar(6);
			else if (event.action == &i_numberActions->key7 && !movemode)
				gotoChar(7);
			else if (event.action == &i_numberActions->key8 && !movemode)
				gotoChar(8);
			else if (event.action == &i_numberActions->key9 && !movemode)
				gotoChar(9);
			else if (event.action == &i_serviceSelectorActions->prevBouquet && !movemode)
			{
				services->beginAtomic();
				if (style == styleCombiColumn)
					bouquets->goPrev();
				else
				{
					eServiceReference last=path.current();
					path.up();
					fillServiceList(path.current());
					selectService( last );
					eListBoxEntryService* p = services->goPrev();
					if (p)
					{
						path.down( p->service );
						fillServiceList( p->service );
						selectService( eServiceInterface::getInstance()->service );
					}
				}
				services->endAtomic();
			}
			else if (event.action == &i_serviceSelectorActions->nextBouquet && !movemode)
			{
				services->beginAtomic();
				if (style == styleCombiColumn)
					bouquets->goNext();
				else
				{
					eServiceReference last=path.current();
					path.up();
					fillServiceList(path.current());
					selectService( last );
					eListBoxEntryService* p = services->goNext();
					if (p)
					{
						path.down( p->service );
						fillServiceList( p->service );
						selectService( eServiceInterface::getInstance()->service );
					}
				}
				services->endAtomic();
			}
			else if (event.action == &i_serviceSelectorActions->showEPGSelector && !movemode)
			{
				const eventMap* e=0;
				if (selected.type == eServiceReference::idDVB)
				 	e = eEPGCache::getInstance()->getEventMap((eServiceReferenceDVB&)selected);
				if (e && !e->empty())
				{
					eEPGSelector wnd((eServiceReferenceDVB&)selected);

					if (LCDElement && LCDTitle)
						wnd.setLCD(LCDTitle, LCDElement);

					hide();
					wnd.show();
					wnd.exec();
					wnd.hide();
					show();
				}
			}
			else if (event.action == &i_serviceSelectorActions->pathUp && !movemode)
			{
				if (path.size() > ( (style == styleCombiColumn) ? 2 : 1) )
				{
					services->beginAtomic();
					eServiceReference last=path.current();
					path.up();
					actualize();
					selectService( last );
					services->endAtomic();
				} else if (style == styleCombiColumn)
					setFocus(bouquets);
			}
			else if (event.action == &i_serviceSelectorActions->toggleStyle && !movemode)
			{
				int newStyle = style;
				if (newStyle == styleMultiColumn)
					newStyle = styleCombiColumn;
				else
					newStyle++;
				setStyle(newStyle);			
			}
			else if (event.action == &i_serviceSelectorActions->toggleFocus && !movemode)
			{
				if ( style == styleCombiColumn )
					if (focus == services)
						setFocus( bouquets );
					else
						setFocus( services );
			}
			else if (event.action == &i_serviceSelectorActions->showMenu/* && !movemode*/)
				/*emit*/ showMenu(this);
			else if (event.action == &i_serviceSelectorActions->showFavourite && !movemode && !FavouriteMode)
				/*emit*/ showFavourite(this);
			else if (event.action == &i_serviceSelectorActions->addService && !movemode)
			{
				if (FavouriteMode)
				{
					eServiceReference &ref = services->getCurrent()->service;
					std::map<eServiceReference, int>::iterator it = eListBoxEntryService::favourites.find( ref );
					if ( it == eListBoxEntryService::favourites.end() )
					{
						/*emit*/ addServiceToFavourite(this, 1);
						eListBoxEntryService::favourites[ref] = 1;
					}
					else
					{
						/*emit*/ removeServiceFromFavourite( ref );
						eListBoxEntryService::favourites.erase( ref );
					}
					services->invalidateCurrent();

					break;
				}
				/*emit*/ addServiceToList(selected);
			}
			else if (event.action == &i_serviceSelectorActions->addServiceToFavourite && !movemode)
				/*emit*/ addServiceToFavourite(this, 0);
			else if (event.action == &i_serviceSelectorActions->modeTV && !movemode)
				/*emit*/ setMode(eZapMain::modeTV);
			else if (event.action == &i_serviceSelectorActions->modeRadio && !movemode)
				/*emit*/ setMode(eZapMain::modeRadio);
			else if (event.action == &i_serviceSelectorActions->modeFile && !movemode)
				/*emit*/ setMode(eZapMain::modeFile);
			else if (event.action == &i_cursorActions->cancel)
			{	
				if (movemode)
				{
					toggleMoveMode();
					services->beginAtomic();
					actualize();
					selectService(selected);
					services->endAtomic();
				}
				if (FavouriteMode)
				{
					FavouriteMode=0;
					if (eListBoxEntryService::favourites.size())
						eListBoxEntryService::favourites.clear();
				}
				break;
			}
			else
				break;
        
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
		services->moveSelection( eListBox<eListBoxEntryService>::dirFirst );
		return false;
	}
	else
		return true;
}

void eServiceSelector::setStyle(int newStyle)
{
	ci->hide();
	eServicePath p = path;
	eServiceReference currentService;
	if (style != newStyle)
	{
			if ( services )
			{
				// safe currentSelected Service
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
			if (newStyle == styleSingleColumn)
			{
				eListBoxEntryService::folder = eSkin::getActive()->queryImage("sselect_folder");
				eListBoxEntryService::numberFont = eSkin::getActive()->queryFont("eServiceSelector.singleColumn.Entry.Number");
				eListBoxEntryService::serviceFont = eSkin::getActive()->queryFont("eServiceSelector.singleColumn.Entry.Name");
			}
			else if (style == styleMultiColumn)
			{
				eListBoxEntryService::folder = 0;	
				eListBoxEntryService::numberFont = eSkin::getActive()->queryFont("eServiceSelector.multiColumn.Entry.Number");
				eListBoxEntryService::serviceFont = eSkin::getActive()->queryFont("eServiceSelector.multiColumn.Entry.Name");
			}
			else
			{
				eListBoxEntryService::folder = 0;	
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

			if ( newStyle == styleSingleColumn )
			{
				if (eSkin::getActive()->build(this, "eServiceSelector_singleColumn"))
					eFatal("Service selector widget build failed!");
			}
			else if ( newStyle == styleMultiColumn )
			{
				if (eSkin::getActive()->build(this, "eServiceSelector_multiColumn"))
					eFatal("Service selector widget build failed!");
			}
			else
			{
				if (eSkin::getActive()->build(this, "eServiceSelector_combiColumn"))
					eFatal("Service selector widget build failed!");
				CONNECT( bouquets->selchanged, eServiceSelector::bouquetSelChanged );
				CONNECT( bouquets->selected, eServiceSelector::bouquetSelected );
				bouquets->show();
			}
			style = newStyle;
			actualize();
			selectService( currentService );  // select the old service
			CONNECT(services->selected, eServiceSelector::serviceSelected);
			CONNECT(services->selchanged, eServiceSelector::serviceSelChanged);
			services->show();
			setFocus(services);
	}
	ci->show();
}

void eServiceSelector::bouquetSelChanged( eListBoxEntryService *entry)
{
	if ( entry && entry->service != eServiceReference() )
	{
		ci->clear();
		services->beginAtomic();
		path.up();
		path.down(entry->service);
		fillServiceList( entry->service );
		selectService( eServiceInterface::getInstance()->service );
		services->endAtomic();
	}
}

void eServiceSelector::actualize()
{
	if (style == styleCombiColumn)
	{
		eServiceReference currentBouquet = path.current();
		path.up();
		eServiceReference allBouquets = path.current();
		path.down( currentBouquet );
		bouquets->beginAtomic();
		fillBouquetList( allBouquets );

		if ( bouquets->forEachEntry( _selectService( currentBouquet ) ) )
			bouquets->moveSelection( eListBox<eListBoxEntryService>::dirFirst );

		bouquets->endAtomic();
	}
	else
		fillServiceList(path.current());
}

eServiceSelector::eServiceSelector()
	:eWindow(0), result(0), services(0), bouquets(0), style(styleInvalid), BrowseChar(0), BrowseTimer(eApp), ciDelay(eApp), movemode(0), FavouriteMode(0)
{
	ci = new eChannelInfo(this);
	ci->setName("channelinfo");

	CONNECT(eDVB::getInstance()->bouquetListChanged, eServiceSelector::actualize);
	CONNECT(BrowseTimer.timeout, eServiceSelector::ResetBrowseChar);
	CONNECT(ciDelay.timeout, eServiceSelector::updateCi );
	CONNECT(eEPGCache::getInstance()->EPGUpdated, eServiceSelector::EPGUpdated);

	addActionMap(&i_serviceSelectorActions->map);
	addActionMap(&i_numberActions->map);
}

eServiceSelector::~eServiceSelector()
{
}

void eServiceSelector::enterDirectory(const eServiceReference &ref)
{
	services->beginAtomic();
	path.down(ref);
	actualize();
	selectService( eServiceInterface::getInstance()->service );
	services->endAtomic();
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
	default:
		break;
	}
	services->endAtomic();

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

	eListBoxEntryService *s=services->goNext();
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

	eListBoxEntryService *s=services->goPrev();
	services->endAtomic();
	if (s)
		return &s->service;
	else
		return 0;
}

void eServiceSelector::setPath(const eServicePath &newpath, const eServiceReference &select)
{
	path=newpath;
	if (services)
	{
		services->beginAtomic();
		actualize();
		selectService(select);
		services->endAtomic();
	}
}

void eServiceSelector::toggleMoveMode()
{
	movemode^=1;
	if (!movemode)
	{
		eListBoxEntryService::selectedToMove=0;
		services->setMoveMode(0);
	}
}

void eServiceSelector::toggleFavouriteMode()
{
	FavouriteMode^=1;
}

#include <algorithm>
#include <list>

#include <apps/enigma/enigma.h>
#include <apps/enigma/enigma_main.h>
#include <apps/enigma/sselect.h>
#include <apps/enigma/epgwindow.h>

#include <core/base/i18n.h>
#include <core/gdi/font.h>
#include <core/gui/actions.h>
#include <core/gui/eskin.h>
#include <core/gui/elabel.h>
#include <core/dvb/edvb.h>
#include <core/dvb/dvb.h>
#include <core/dvb/dvbservice.h>
#include <core/dvb/epgcache.h>
#include <core/driver/rc.h>
#include <core/system/init.h>
#include <core/dvb/service.h>
#include <core/gui/numberactions.h>

gFont eListBoxEntryService::serviceFont;
gFont eListBoxEntryService::descrFont;
gFont eListBoxEntryService::numberFont;
gPixmap *eListBoxEntryService::folder=0;
int eListBoxEntryService::maxNumSize=0;

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
	:eListBoxEntry((eListBox<eListBoxEntry>*)lb), service(service)
{
#if 0
	sort=eString().sprintf("%06d", service->service_number);
#else
	const eService *pservice=eServiceInterface::getInstance()->addRef(service);
	sort=pservice?pservice->service_name:"";
	sort.upper();
	eServiceInterface::getInstance()->removeRef(service);
#endif
	numPara=namePara=descrPara=0;
}

eListBoxEntryService::~eListBoxEntryService()
{
	invalidate();
}

int eListBoxEntryService::getEntryHeight()
{
	if (!serviceFont.pointSize)
	{
		numberFont = eSkin::getActive()->queryFont("eServiceSelector.singleColumn.Entry.Number");
		serviceFont = eSkin::getActive()->queryFont("eServiceSelector.singleColumn.Entry.Name");
		descrFont = eSkin::getActive()->queryFont("eServiceSelector.Entry.Description");
		folder = eSkin::getActive()->queryImage("sselect_folder");
	}
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
	drawEntryRect(rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, hilited );

	eString sname;
	eString sdescr;
	const eService *pservice=eServiceInterface::getInstance()->addRef(service);

	if (pservice)
		sname=pservice->service_name;

	if (/*service.type == eServiceReference::idDVB &&*/ !(service.flags & eServiceReference::isDirectory) )
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
	else  // we draw the folder pixmap
	{
		nameXOffs = folder->x + 20;
  	int ypos = (rect.height() - folder->y) / 2;
		rc->blit( *folder, ePoint(10, rect.top()+ypos ), eRect(), gPixmap::blitAlphaTest);		
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

	if ( listbox->getColumns() == 1 && /*service.type == eServiceReference::idDVB && */!(service.flags & eServiceReference::isDirectory) )
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

	return sort;
}

void eServiceSelector::addService(const eServiceReference &ref)
{
	new eListBoxEntryService(services, ref);
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
	{
		const eService *pservice=eServiceInterface::getInstance()->addRef(_ref);
	  if (pservice)
			setText( pservice->service_name );
		else
			setText("");
		eServiceInterface::getInstance()->removeRef(_ref);
	}

	services->beginAtomic();
	services->clearList();

	eServiceInterface *iface=eServiceInterface::getInstance();
	ASSERT(iface);
	
	Signal1<void,const eServiceReference&> signal;
	CONNECT(signal, eServiceSelector::addService);
	
	eServiceReference ref=_ref;
	
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

void eServiceSelector::entrySelected(eListBoxEntryService *entry)
{
	if (entry && entry->service)
	{
		const eServiceReference &ref=entry->service;

		if (ref.flags & eServiceReference::isDirectory)
			enterDirectory(ref);
		else
		{
			result=&entry->service;
			close(0);
		}
	}
}

void eServiceSelector::selchanged(eListBoxEntryService *entry)
{
	if (entry)
	{
		selected = (((eListBoxEntryService*)entry)->service);
		if (ci->isVisible())				
		{
			ci->clear();
//			eDebug("tsid = %i, type = %i, isDirectory=%i", ((const eServiceReferenceDVB&)selected).getTransportStreamID().get(), selected.type, selected.flags & eServiceReference::isDirectory  );
			if ( selected.type == eServiceReference::idDVB &&
						!(selected.flags & eServiceReference::isDirectory) &&
							(((const eServiceReferenceDVB&)selected).getTransportStreamID().get() > 0))
  			ciDelay.start( 500, true );
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
			if (event.action == &i_numberActions->key2)
				gotoChar(2);
			else if (event.action == &i_numberActions->key3)
				gotoChar(3);
			else if (event.action == &i_numberActions->key4)
				gotoChar(4);
			else if (event.action == &i_numberActions->key5)
				gotoChar(5);
			else if (event.action == &i_numberActions->key6)
				gotoChar(6);
			else if (event.action == &i_numberActions->key7)
				gotoChar(7);
			else if (event.action == &i_numberActions->key8)
				gotoChar(8);
			else if (event.action == &i_numberActions->key9)
				gotoChar(9);
			else if (event.action == &i_serviceSelectorActions->prevBouquet)
			{
				services->beginAtomic();
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
				services->endAtomic();
			}
			else if (event.action == &i_serviceSelectorActions->nextBouquet)
			{
				services->beginAtomic();
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
				services->endAtomic();
			}
			else if (event.action == &i_serviceSelectorActions->showEPGSelector)
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
			else if (event.action == &i_serviceSelectorActions->pathUp)
			{
				eServiceReference last=path.current();
				path.up();
				if (last != path.current())
				{
					services->beginAtomic();
					fillServiceList(path.current());
					selectService(last);
					services->endAtomic();
				}
			}
			else if (event.action == &i_serviceSelectorActions->toggleStyle)
				setStyle(-1);
			else if (event.action == &i_serviceSelectorActions->showMenu)
				showMenu(this);
			else if (event.action == &i_serviceSelectorActions->showFavourite)
				showFavourite(this);
			else if (event.action == &i_serviceSelectorActions->addService)
				addServiceToList(selected);
			else if (event.action == &i_serviceSelectorActions->addServiceToFavourite)
				addServiceToFavourite(this);
			else if (event.action == &i_serviceSelectorActions->modeTV)
				setMode(eZapMain::modeTV);
			else if (event.action == &i_serviceSelectorActions->modeRadio)
				setMode(eZapMain::modeRadio);
			else if (event.action == &i_serviceSelectorActions->modeFile)
				setMode(eZapMain::modeFile);
			else
				break;
			return 1;
		default:
			break;
	}
	return eWindow::eventHandler(event);
}

void eServiceSelector::setStyle(int newstyle)
{
/*	int safedStyle=-1;
	if ( eZapMain::getInstance()->getRealMode() == eZapMain::modeTV || eZapMain::getInstance()->getRealMode() == eZapMain::modeRadio )
	{
		if (newstyle == -1)  //the we toggle current Style
		{
			if (style == styleClassic)
				style=styleMultiColumn;
			else if (style == styleMultiColumn)
				style=styleClassic;
		}
		else
			style = newstyle;
	}
	else
	{
		safedStyle=style;
		style = styleClassic;
	}
	services->setColumns( style==styleClassic ? 1 : eSkin::getActive()->queryValue( "eServiceSelector.columns", 2 ) );
	switch (style)
	{
		case styleClassic:
			if ( bouquets )
			{
				delete bouquets;
				bouquets=0;
				services->beginAtomic();
				services->resize( eSize( clientrect.size().width(), services->getSize().height()+10 ) );
				services->move( ePoint(0,0) );
				services->loadDeco();
				services->endAtomic();
			}
		break;
		case styleMultiColumn:
			if (!bouquets)
			{
				bouquets=new eListBox<eListBoxEntryService>( this );
				bouquets->move( ePoint( 5, 5 ) );
				bouquets->resize( eSize( 200, services->getSize().height()-10 ) );
				bouquets->loadDeco();
				bouquets->show();
				services->beginAtomic();
				services->move( ePoint(210, 5) );
				services->resize( eSize( clientrect.size().width()-215, services->getSize().height()-10 ) );
				services->endAtomic();
			}
		break;
	}


	if (safedStyle != -1)
		style = safedStyle;
*/
}

void eServiceSelector::actualize()
{
	fillServiceList(path.current());
}

eServiceSelector::eServiceSelector()
								:eWindow(0), result(0), BrowseChar(0), bouquets(0), BrowseTimer(eApp), ciDelay(eApp)
{
	services = new eListBox<eListBoxEntryService>(this);
	services->setName("services");
	services->setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight.background"), eSkin::getActive()->queryScheme("eServiceSelector.highlight.foreground"));

	bouquets = new eListBox<eListBoxEntryService>(this);
	bouquets->setName("bouquets");
	bouquets->hide();

	ci = new eChannelInfo(this);
	ci->setName("channelinfo");

	CONNECT(eDVB::getInstance()->bouquetListChanged, eServiceSelector::actualize);
	CONNECT(services->selected, eServiceSelector::entrySelected);
	CONNECT(services->selchanged, eServiceSelector::selchanged);
	CONNECT(BrowseTimer.timeout, eServiceSelector::ResetBrowseChar);
	CONNECT(ciDelay.timeout, eServiceSelector::updateCi );
	CONNECT(eEPGCache::getInstance()->EPGUpdated, eServiceSelector::EPGUpdated);

	if (eSkin::getActive()->build(this, "eServiceSelector_singleColumn"))
		eFatal("Service selector widget build failed!");

	addActionMap(&i_serviceSelectorActions->map);
	addActionMap(&i_numberActions->map);
}

eServiceSelector::~eServiceSelector()
{
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

void eServiceSelector::selectService(const eServiceReference &ref)
{
	if ( services->forEachEntry( _selectService(ref) ) )
		services->moveSelection( eListBox<eListBoxEntryService>::dirFirst );
}

void eServiceSelector::enterDirectory(const eServiceReference &ref)
{
	services->beginAtomic();
	path.down(ref);
	fillServiceList( ref );
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
//	actualize();
	selectService( eServiceInterface::getInstance()->service );
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
	services->beginAtomic();
	path=newpath;
	fillServiceList(path.current());
	selectService(select);
	services->endAtomic();
}

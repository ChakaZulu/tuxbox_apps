#include <algorithm>
#include <list>

#include <apps/enigma/enigma.h>
#include <apps/enigma/sselect.h>
#include <apps/enigma/bselect.h>
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

gFont eListBoxEntryService::serviceFont;
gFont eListBoxEntryService::descrFont;
gFont eListBoxEntryService::numberFont;

struct serviceSelectorActions
{
  eActionMap map;
	eAction nextBouquet, prevBouquet, showBouquetSelector, showEPGSelector, showAllServices;
	serviceSelectorActions():
		map("serviceSelector", _("service selector")),
		nextBouquet(map, "nextBouquet", _("switch to next bouquet"), eAction::prioDialogHi),
		prevBouquet(map, "prevBouquet", _("switch to previous bouquet"), eAction::prioDialogHi),
		showBouquetSelector(map, "showBouquetSelector", _("shows the bouquet selector"), eAction::prioDialog),
		showEPGSelector(map, "showEPGSelector", _("shows the EPG selector for the highlighted channel"), eAction::prioDialog),
		showAllServices(map, "showAllServices", _("switch to all services"), eAction::prioDialog)
	{
	}
};

struct numberActions
{
	eActionMap map;
	eAction key0, key1, key2, key3, key4, key5, key6, key7, key8, key9;
	numberActions():
		map("numbers", _("number actions")),
		key0(map, "0", _("key 0"), eAction::prioDialog),
		key1(map, "1", _("key 1"), eAction::prioDialog),
		key2(map, "2", _("key 2"), eAction::prioDialog),
		key3(map, "3", _("key 3"), eAction::prioDialog),
		key4(map, "4", _("key 4"), eAction::prioDialog),
		key5(map, "5", _("key 5"), eAction::prioDialog),
		key6(map, "6", _("key 6"), eAction::prioDialog),
		key7(map, "7", _("key 7"), eAction::prioDialog),
		key8(map, "8", _("key 8"), eAction::prioDialog),
		key9(map, "9", _("key 9"), eAction::prioDialog)
	{
	}
};

eAutoInitP0<serviceSelectorActions> i_serviceSelectorActions(5, "service selector actions");
eAutoInitP0<numberActions> i_numberActions(5, "number actions");

eListBoxEntryService::eListBoxEntryService(eListBox<eListBoxEntryService> *lb, const eServiceReference &service)
	:eListBoxEntry((eListBox<eListBoxEntry>*)lb), service(service)
{
	if (!serviceFont.pointSize)
	{
		serviceFont = eSkin::getActive()->queryFont("eServiceSelector.Entry.Name");
		descrFont = eSkin::getActive()->queryFont("eServiceSelector.Entry.Description");
		numberFont = eSkin::getActive()->queryFont("eServiceSelector.Entry.Number");
	}
#if 0
	sort=eString().sprintf("%06d", service->service_number);
#else
	const eService *pservice=eServiceInterface::getInstance()->lookupService(service);
	sort=pservice?pservice->service_name:"";
	sort.upper();
#endif
}

eListBoxEntryService::~eListBoxEntryService()
{
}

int eListBoxEntryService::getHeight()
{
	eTextPara *para;
	para = new eTextPara( eRect(0,0,100,50) );
	para->setFont( serviceFont );
	para->renderString("Mjdyl");
	int i = para->getBoundBox().height();
	para->destroy();
	return i+4;
}


void eListBoxEntryService::redraw(gPainter *rc, const eRect &rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int hilited)
{
	eString sname;
	if (service.type == eServiceReference::idDVB)
	{
		const eService *pservice=eServiceInterface::getInstance()->lookupService(service);
		if (pservice)
		{
			sname=pservice->service_name;
			EITEvent *e=eEPGCache::getInstance()->lookupCurrentEvent((const eServiceReferenceDVB&)service);

			eWidget* p = listbox->getParent();			
			if (hilited && p && p->LCDElement)
					p->LCDElement->setText(sort);

			if (e)
			{
				for (ePtrList<Descriptor>::iterator d(e->descriptor); d != e->descriptor.end(); ++d)
				{
					Descriptor *descriptor=*d;
					if (descriptor->Tag()==DESCR_SHORT_EVENT)
					{
						ShortEventDescriptor *ss=(ShortEventDescriptor*)descriptor;
						sname+=" (";
						sname+=ss->event_name;
						sname+=")";
						break;
					}
				}
				delete e;
			}
		}
		else
			return;
	} else
		sname="non-DVB";
	
	rc->setFont( serviceFont );

	if ((coNormalB != -1 && !hilited) || (hilited && coActiveB != -1))
	{
		rc->setForegroundColor(hilited?coActiveB:coNormalB);
		rc->fill(rect);
		rc->setBackgroundColor(hilited?coActiveB:coNormalB);
	} else
	{
		eWidget *w=listbox->getNonTransparentBackground();
		rc->setForegroundColor(w->getBackgroundColor());
		rc->fill(rect);
		rc->setBackgroundColor(w->getBackgroundColor());
	}

	rc->setForegroundColor(hilited?coActiveF:coNormalF);

	rc->renderText(rect, sname);
}

struct eServiceSelector_addService: public std::unary_function<eServiceReference&,void>
{
	eListBox<eListBoxEntryService> &list;

	int mode;

	eServiceSelector_addService(eListBox<eListBoxEntryService> &list, int mode)
		:list(list), mode(mode)
	{
	}

	void operator()(const eServiceReference& c)
	{
		int useable=0;
		
		if (c.type == eServiceReference::idDVB)
		{
			const eServiceReferenceDVB &d=(const eServiceReferenceDVB&)c;
			if ( mode == eZap::TV)
			{
				if ( d.getServiceType() == 1 || d.getServiceType() == 4)
					useable++;
			}
			else
				if (d.getServiceType() == 2)
					useable++;
		}
		if (useable)
			new eListBoxEntryService(&list, c);
	}
};

void eServiceSelector::fillServiceList()
{
	services->clearList();

	if (eDVB::getInstance()->settings->getTransponders())
		eDVB::getInstance()->settings->getTransponders()->forEachServiceReference(eServiceSelector_addService(*services, eZap::getInstance()->getMode() ));
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

void eServiceSelector::gotoChar(char c)
{
	if (pbs->current()->bouquet_id < 0)   // user defined bouquets... no key choosing...
		return;

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
		services->forEachEntry(moveFirstChar(BrowseChar));
	}
}

void eServiceSelector::entrySelected(eListBoxEntryService *entry)
{
	if (!entry)
	{
		result=0;
		hide();			// a little tricky..
		resetBouquet();
		close(1);
	}
	else if (entry->service)
	{
		if (eZap::getInstance()->getMode() == eZap::TV)
			lastTvBouquet = pbs->current()->bouquet_id;
		else
			lastRadioBouquet = pbs->current()->bouquet_id;

		result=&entry->service;
		close(0);
	}
}

void eServiceSelector::selchanged(eListBoxEntryService *entry)
{
	selected = (((eListBoxEntryService*)entry)->service);
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
				eBouquet *b;
				b=pbs->prev();
				if (b)
					useBouquet(b);
			}
			else if (event.action == &i_serviceSelectorActions->nextBouquet)
			{
					eBouquet *b;
					b=pbs->next();
					if (b)
						useBouquet(b);
			}
			else if (event.action == &i_serviceSelectorActions->showBouquetSelector)
			{
					eBouquet *b;
					hide();
					pbs->setLCD(LCDTitle, LCDElement);
					b=pbs->choose();
					if (b)
						useBouquet(b);

					show();
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
			else if (event.action == &i_serviceSelectorActions->showAllServices)
			{
				pbs->moveTo( 9999 );
				useBouquet( pbs->current() );
				services->sort();
				services->invalidate();
			}
			else
				break;
		return 1;

		default:

		break;
	}
	return eWindow::eventHandler(event);
}

void eServiceSelector::actualize()
{
		if (pbs->fillBouquetList())  // Bouquets added ?
			if (eZap::getInstance()->getMode() == eZap::TV)
			{
			 	if ( pbs->moveTo(lastTvBouquet) )
					useBouquet( pbs->current() );
			}
			else
			 	if ( pbs->moveTo(lastRadioBouquet) )
					useBouquet( pbs->current() );			
}

eServiceSelector::eServiceSelector()
								:eWindow(0), result(0), BrowseChar(0), BrowseTimer(eApp)
{
	services = new eListBox<eListBoxEntryService>(this);
	services->setName("services");
	services->setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight.background"), eSkin::getActive()->queryScheme("eServiceSelector.highlight.foreground"));
	
	pbs = new eBouquetSelector();

	if (eConfig::getInstance()->getKey("/ezap/ui/lastTvBouquet", lastTvBouquet))
		lastTvBouquet = 9999;

	if (eConfig::getInstance()->getKey("/ezap/ui/lastRadioBouquet", lastRadioBouquet))
		lastRadioBouquet = 9999;

	CONNECT(eDVB::getInstance()->bouquetListChanged, eServiceSelector::actualize);
	CONNECT(services->selected, eServiceSelector::entrySelected);
	CONNECT(services->selchanged, eServiceSelector::selchanged);
	CONNECT(eDVB::getInstance()->serviceListChanged, eServiceSelector::actualize);
	CONNECT(BrowseTimer.timeout, eServiceSelector::ResetBrowseChar);
	CONNECT(pbs->cancel, eServiceSelector::resetBouquet);

	if (eSkin::getActive()->build(this, "eServiceSelector"))
		eWarning("Service selector widget build failed!");

	actualize();

	addActionMap(&i_serviceSelectorActions->map);
	addActionMap(&i_numberActions->map);
}

void eServiceSelector::resetBouquet()
{
		int id = eZap::getInstance()->getMode()?lastRadioBouquet:lastTvBouquet;

		if ( pbs->current() )
		{
			pbs->moveTo( id );
			useBouquet( pbs->current() );
		}
		selectCurrentService();
}

eServiceSelector::~eServiceSelector()
{
	eConfig::getInstance()->setKey("/ezap/ui/lastTvBouquet", lastTvBouquet);
	eConfig::getInstance()->setKey("/ezap/ui/lastRadioBouquet", lastRadioBouquet);		

	if (pbs)
		delete pbs;
}

struct selectService: public std::unary_function<const eListBoxEntryService&, void>
{
	const eServiceReference& service;

	selectService(const eServiceReference& e): service(e)
	{
	}

	bool operator()(const eListBoxEntryService& s)
	{
		if (service == s.service)
		{
	 		( (eListBox<eListBoxEntryService>*) s.listbox)->setCurrent(&s);
			return 1;
		}
		return 0;
	}
};

void eServiceSelector::selectCurrentService()
{
	if (selected != eDVB::getInstance()->getServiceAPI()->service)
		services->forEachEntry( selectService( eDVB::getInstance()->getServiceAPI()->service ) );
}

void eServiceSelector::useBouquet(const eBouquet *bouquet)
{
	services->clearList();

	if (bouquet)
	{
		setText(bouquet->bouquet_name);
		
		if (bouquet->bouquet_id != 9999) // all Services
			for (std::list<eServiceReferenceDVB>::const_iterator i( bouquet->list.begin() ); i != bouquet->list.end(); i++)
			{
				int addToList=0;
		
				if (eZap::getInstance()->getMode() == eZap::TV)
				{
					if (i->getServiceType() == 1 || i->getServiceType() == 4) // TV or Nvod
						addToList++;
				}
				else
					if (i->getServiceType() == 2) //Radio
						addToList++;

				if (addToList)
					new eListBoxEntryService(services, *i);
			}
		else
			fillServiceList();

		if (bouquet->bouquet_id >= 0)
			services->sort();
	}
	services->invalidate();
}

void eServiceSelector::ResetBrowseChar()
{
	BrowseChar=0;
}

const eServiceReference *eServiceSelector::choose(int irc)
{
	selectCurrentService();

	result=0;

	show();

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
	if (exec())
		result=0;

	hide();
	return result;
}

const eServiceReference *eServiceSelector::next()
{
	selectCurrentService();

	eListBoxEntryService *s=services->goNext();
	if (s)
		return &s->service;
	else
		return 0;
}

const eServiceReference *eServiceSelector::prev()
{
	selectCurrentService();

	eListBoxEntryService *s=services->goPrev();
	if (s)
		return &s->service;
	else
		return 0;
}

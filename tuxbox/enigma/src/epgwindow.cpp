#include <epgwindow.h>
#include <enigma_event.h>

#include <algorithm>

#include <timer.h>
#include <enigma_main.h>
#include <enigma_lcd.h>
#include <lib/gui/eskin.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/dvbservice.h>
#include <lib/gdi/font.h>
#include <lib/system/init_num.h>
#include "epgactions.h"

gFont eListBoxEntryEPG::TimeFont;
gFont eListBoxEntryEPG::DescrFont;
gPixmap *eListBoxEntryEPG::inTimer=0;
gPixmap *eListBoxEntryEPG::inTimerRec=0;
int eListBoxEntryEPG::timeXSize=0;
int eListBoxEntryEPG::dateXSize=0;

eString LocalEventData::country;
eString LocalEventData::primary_language;
eString LocalEventData::secondary_language;
eString now_text;
int myEPGSearch;


eAutoInitP0<epgSelectorActions> i_epgSelectorActions(eAutoInitNumbers::actions, "epg selector actions");

eListBoxEntryEPG::~eListBoxEntryEPG()
{
	if (paraTime)
		paraTime->destroy();

	if (paraDate)
		paraDate->destroy();

	if (paraDescr)
		paraDescr->destroy();
}

int eListBoxEntryEPG::getEntryHeight()
{
	if (!DescrFont.pointSize && !TimeFont.pointSize)
	{
		DescrFont = eSkin::getActive()->queryFont("eEPGSelector.Entry.Description");
		TimeFont = eSkin::getActive()->queryFont("eEPGSelector.Entry.DateTime");
		inTimer = eSkin::getActive()->queryImage("timer_symbol");
		inTimerRec = eSkin::getActive()->queryImage("timer_rec_symbol");
		eTextPara* tmp = new eTextPara( eRect(0, 0, 200, 30) );
		tmp->setFont( TimeFont );
		tmp->renderString( "00:00" );
		timeXSize = tmp->getBoundBox().width();
		tmp->destroy();
		tmp = new eTextPara( eRect(0, 0, 200, 30) );
		tmp->setFont( TimeFont );
		tmp->renderString( "Tue 09.09," );
		dateXSize = tmp->getBoundBox().width();
		tmp->destroy();
	}
	return calcFontHeight(DescrFont)+4;	
}

void eListBoxEntryEPG::build()
{
	start_time = *localtime(&event.start_time);
	if (myEPGSearch) // EPGSearch
	{
		descr = now_text;
		return;
	}
	LocalEventData led;
	led.getLocalData(&event, &descr);

	if (descr)
		return;
	for (ePtrList<Descriptor>::iterator d(event.descriptor); d != event.descriptor.end(); ++d)
	{
		Descriptor *descriptor=*d;
		if (descriptor->Tag()==DESCR_TIME_SHIFTED_EVENT)
		{
			// build parent Service Reference
			eServiceReferenceDVB nvodService(
					((eServiceReferenceDVB&)service).getDVBNamespace(),
					service.data[2], service.data[3], 
					((TimeShiftedEventDescriptor*)descriptor)->reference_service_id, service.data[0] );
			// get EITEvent from Parent...
			EITEvent* evt = eEPGCache::getInstance()->lookupEvent(nvodService, ((TimeShiftedEventDescriptor*)descriptor)->reference_event_id );
			if (evt)
			{
				led.getLocalData(evt, &descr);
				delete evt;
				return;
			}
		}
	}
	descr = "no event data available";
}

eListBoxEntryEPG::eListBoxEntryEPG(const eit_event_struct* evt, eListBox<eListBoxEntryEPG> *listbox, eServiceReference &ref)
		:eListBoxEntry((eListBox<eListBoxEntry>*)listbox), paraDate(0), paraTime(0), paraDescr(0), event(evt,(((eServiceReferenceDVB&)ref).getTransportStreamID().get()<<16)|((eServiceReferenceDVB&)ref).getOriginalNetworkID().get()), service(ref)
{
	build();
}

eListBoxEntryEPG::eListBoxEntryEPG(EITEvent& evt, eListBox<eListBoxEntryEPG> *listbox, eServiceReference &ref)
	:eListBoxEntry((eListBox<eListBoxEntry>*)listbox), paraDate(0), paraTime(0), paraDescr(0), event(evt), service(ref)
{
	build();
}

/* extern const char *dayStrShort[]; bug fix - at localization, 
   macro the type _ ("xxxxx") for a constant does not work, 
   if it is declared outside of the function */

const eString &eListBoxEntryEPG::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int hilited)
{
	const char *dayStrShort[7] = { _("Sun"), _("Mon"), _("Tue"), _("Wed"), _("Thu"), _("Fri"), _("Sat") };

	drawEntryRect(rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, hilited);

	int xpos=rect.left()+10;
	if (!paraDate)
	{
		paraDate = new eTextPara( eRect( 0, 0, dateXSize, rect.height()) );
		paraDate->setFont( TimeFont );
		hlp.sprintf("%02d.%02d,", start_time.tm_mday, start_time.tm_mon+1);
		paraDate->renderString( eString(dayStrShort[start_time.tm_wday])+' '+hlp );
		paraDate->realign( eTextPara::dirRight );
		TimeYOffs = ((rect.height() - paraDate->getBoundBox().height()) / 2 ) - paraDate->getBoundBox().top();
	}
	rc->renderPara(*paraDate, ePoint( xpos, rect.top() + TimeYOffs ) );
	xpos+=dateXSize+paraDate->getBoundBox().height();

	if (!paraTime)
	{
		paraTime = new eTextPara( eRect( 0, 0, timeXSize, rect.height()) );
		paraTime->setFont( TimeFont );         		
		eString tmp;
		tmp.sprintf("%02d:%02d", start_time.tm_hour, start_time.tm_min);
		paraTime->renderString( tmp );
		paraTime->realign( eTextPara::dirRight );
		hlp+=tmp;
	}
	rc->renderPara(*paraTime, ePoint( xpos, rect.top() + TimeYOffs ) );
	xpos+=timeXSize+paraTime->getBoundBox().height();

	ePlaylistEntry* p=0;
	if ( (p = eTimerManager::getInstance()->findEvent( &service, &event )) )
		if ( p->type & ePlaylistEntry::SwitchTimerEntry )
		{
			int ypos = (rect.height() - inTimer->y) / 2;
			rc->blit( *inTimer, ePoint( xpos, rect.top()+ypos ), eRect(), gPixmap::blitAlphaTest);		
			xpos+=paraTime->getBoundBox().height()+inTimer->x;
		}
		else if ( p->type & ePlaylistEntry::RecTimerEntry )
		{
			int ypos = (rect.height() - inTimerRec->y) / 2;
			rc->blit( *inTimerRec, ePoint( xpos, rect.top()+ypos ), eRect(), gPixmap::blitAlphaTest);		
			xpos+=paraTime->getBoundBox().height()+inTimerRec->x;
		}
	
	if (!paraDescr)
	{
		paraDescr = new eTextPara( eRect( 0 ,0, rect.width(), rect.height()) );
		paraDescr->setFont( DescrFont );
		paraDescr->renderString(descr);
		DescrYOffs = 0; // ((rect.height() - paraDescr->getBoundBox().height()) / 2 ) - paraDescr->getBoundBox().top();
		hlp=hlp+' '+descr;
	}
	rc->renderPara(*paraDescr, ePoint( xpos, rect.top() + DescrYOffs ) );

	return hlp;
}

void eEPGSelector::fillEPGList()
{
	eService *service=eDVB::getInstance()->settings->getTransponders()->searchService(current);
	if (service)
		setText(eString(_("EPG - "))+service->service_name);
	eDebug("get EventMap for onid: %02x, sid: %02x", current.getOriginalNetworkID().get(), current.getServiceID().get());

	eEPGCache::getInstance()->Lock();
	const timeMap* evt = eEPGCache::getInstance()->getTimeMap(current);
	timeMap::const_iterator It;
	if (evt)
		It = evt->begin();

	int tsidonid = (current.getTransportStreamID().get()<<16)|current.getOriginalNetworkID().get();
	if (current.data[0] == 5 ) // NVOD REF ENTRY
	{
		for (; It != evt->end(); It++)
		{
			EITEvent evt(*It->second,tsidonid);   // NVOD Service Event
			for (ePtrList<Descriptor>::iterator d(evt.descriptor); d != evt.descriptor.end(); ++d)
			{
				Descriptor *descr=*d;
				// dereference only TimeShiftedEventDescriptor specific data when the Tag is okay...
				if (descr->Tag()==DESCR_TIME_SHIFTED_EVENT)
				{
					TimeShiftedEventDescriptor *descriptor = (TimeShiftedEventDescriptor*) descr;
//					eServiceReferenceDVB ref( current.getTransportStreamID().get(), current.getOriginalNetworkID().get(), descriptor->reference_event_id, 4 );
					const timeMap *parent = eEPGCache::getInstance()->getTimeMap(eZapMain::getInstance()->refservice);
					timeMap::const_iterator pIt;
					if ( parent )
					{
						pIt = parent->find( descriptor->reference_event_id );
						if ( pIt != parent->end() )   // event found..
						{
							// build EITEvent with short and ext description )
							EITEvent e(*pIt->second,tsidonid);
							// do not delete ePtrListEntrys..
							e.descriptor.setAutoDelete(false);
							e.start_time = evt.start_time;
							e.duration = evt.duration;
							e.event_id = evt.event_id;
							e.free_CA_mode = evt.free_CA_mode;
							e.running_status = evt.running_status;
							new eListBoxEntryEPG(e, events, current );
							break;
						}
					}
				}
			}
		}
	}
#if 0
	else if (current.data[0] == 4 ) //NVOD
	{
		for (; It != evt->end(); It++)
		{
			EITEvent evt(*It->second);   // NVOD Service Event

			const std::list<NVODReferenceEntry> *RefList = eEPGCache::getInstance()->getNVODRefList( (eServiceReferenceDVB&)current );
			if (RefList)
			{
				for (std::list<NVODReferenceEntry>::const_iterator it( RefList->begin() ); it != RefList->end(); it++ )
				{
					eServiceReferenceDVB ref( ((eServiceReferenceDVB&)current).getDVBNamespace(), it->transport_stream_id, it->original_network_id, it->service_id, 5 );
					const timeMap *eMap = eEPGCache::getInstance()->getTimeMap( ref );
					if (eMap)
					{
						for ( timeMap::const_iterator refIt( eMap->begin() ); refIt != eMap->end(); refIt++)
						{
							EITEvent refEvt(*refIt->second);
							for (ePtrList<Descriptor>::iterator d(refEvt.descriptor); d != refEvt.descriptor.end(); ++d)
							{
								Descriptor *descriptor=*d;
								if (descriptor->Tag()==DESCR_TIME_SHIFTED_EVENT)
								{
									if ( ((TimeShiftedEventDescriptor*)descriptor)->reference_event_id == evt.event_id)
									{
										// make copy of ref Event  ( then we habe begintime and duration )
										EITEvent e(*It->second);
										// do not delete ePtrListEntrys..
										e.descriptor.setAutoDelete(false);
										e.start_time = refEvt.start_time;
										e.duration = refEvt.duration;
										e.event_id = refEvt.event_id;
										e.free_CA_mode = refEvt.free_CA_mode;
										e.running_status = refEvt.running_status;
										new eListBoxEntryEPG(e, events, ref);
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
#endif
	else for (It = evt->begin(); It != evt->end(); It++)
		new eListBoxEntryEPG(*It->second, events, current);
	eEPGCache::getInstance()->Unlock();
}

void eEPGSelector::entrySelected(eListBoxEntryEPG *entry)
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();

	if (!entry || !sapi)
		close(0);
	else
	{	
		int ret;
		hide();
		eService *service=eDVB::getInstance()->settings->getTransponders()->searchService(current);
		eEventDisplay ei(service ? service->service_name.c_str() : "", current, 0, &entry->event);
		if (myEPGSearch)
			ei.setEPGSearchEvent((eServiceReferenceDVB&)entry->service, &entry->event, service ? service->service_name.c_str() : "");
#ifndef DISABLE_LCD
		ei.setLCD(LCDTitle, LCDElement);
#endif
		ei.show();
		while((ret = ei.exec()))
		{
			eListBoxEntryEPG* tmp;

			if (ret == 1)
				tmp=events->goPrev();
			else if (ret == 2)
				tmp=events->goNext();
			else
				break; // close EventDisplay

			if (tmp)
			{
				if (myEPGSearch)
				{
					eService *ServiceName=eDVB::getInstance()->settings->getTransponders()->searchService((eServiceReferenceDVB&)tmp->service);
					ei.setEPGSearchEvent((eServiceReferenceDVB&)tmp->service, &tmp->event, ServiceName ? ServiceName->service_name.c_str() : "");
				}
				else
					ei.setEvent(&tmp->event);
			}
		}
		ei.hide();
		invalidate();
		show();
	}
}

eEPGSelector::eEPGSelector(const eServiceReferenceDVB &service)
	:eWindow(0), current(service)
{
	init_eEPGSelector(NULL);
}
void eEPGSelector::init_eEPGSelector(eString* pSearchString)
{
	events = new eListBox<eListBoxEntryEPG>(this);
	events->setName("events");
	events->setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight.background"), eSkin::getActive()->queryScheme("eServiceSelector.highlight.foreground"));

	BuildSkin("eEPGSelector");

	CONNECT(events->selected, eEPGSelector::entrySelected);
	myEPGSearch = 0;
	if (pSearchString)
	{
		myEPGSearch = 1;
		fillEPGSearchList();
		setText(eString(_("EPG Search")) +": " + *pSearchString);
	}
	else
		fillEPGList();
	addActionMap( &i_epgSelectorActions->map );
#ifndef DISABLE_FILE
	addActionToHelpList( &i_epgSelectorActions->addDVRTimerEvent );
#endif
#ifndef DISABLE_NETWORK
	addActionToHelpList( &i_epgSelectorActions->addNGRABTimerEvent );
#endif
	addActionToHelpList( &i_epgSelectorActions->addSwitchTimerEvent );
	addActionToHelpList( &i_epgSelectorActions->removeTimerEvent );
	setHelpID(12);
}

eEPGSelector::eEPGSelector(eString SearchString):eWindow(0)
{
	init_eEPGSelector(&SearchString);
}
void eEPGSelector::fillEPGSearchList()
{
	SearchEPGDATA SearchEPGDATA1;
	SearchEPGDATA1 = eEPGSearchDATA::getInstance()->getSearchData();
	for (SearchEPGDATA::iterator a = SearchEPGDATA1.begin(); a != SearchEPGDATA1.end(); a++)
	{
		EITEvent e;
		e.start_time = a->start_time;
		e.duration = a->duration;
		e.event_id = -1;
		eServiceReference Ref;
		Ref = a->ref;
		eService *s = eTransponderList::getInstance()->searchService( a->ref );
		if (s)
		{
			Ref.descr = s->service_name + "/" + a->title;
			now_text = s->service_name + ": " + a->title;
			new eListBoxEntryEPG(e, events, Ref);
		}
	}
	eEPGSearchDATA::getInstance()->clearList();
}

int eEPGSelector::eventHandler(const eWidgetEvent &event)
{
	int addtype=-1;
	switch (event.type)
	{
		case eWidgetEvent::evtAction:
			if (event.action == &i_epgSelectorActions->searchEPG)
			{
				if (!myEPGSearch)
				{
					hide();
					eEPGSearch *dd = new eEPGSearch(current,NULL, &events->getCurrent()->event);
					dd->show();
					int back = 2;
					do
					{
						back = dd->exec();
						EPGSearchName = dd->getSearchName();
						if (back == 2)
						{
							dd->hide();
							eMessageBox::ShowBox(EPGSearchName + eString(_(" was not found!")) , _("EPG Search"), eMessageBox::iconInfo|eMessageBox::btOK);
							dd->show();
						}
					}
					while (back == 2);
					dd->hide();
					delete dd;
					if (!back)
					{
						close(2);
					}
					else
						show();
					break;
				}
			}
			if ( (addtype = i_epgSelectorActions->checkTimerActions( event.action )) != -1 )
				;
			else if (event.action == &i_epgSelectorActions->removeTimerEvent)
			{
				if ( eTimerManager::getInstance()->removeEventFromTimerList( this, &current, &events->getCurrent()->event ) )
					events->invalidateCurrent();
			}
			else if (event.action == &i_epgSelectorActions->showExtendedInfo)
				entrySelected(events->getCurrent());
			else
				break;
			if (addtype != -1)
			{
				if ( !eTimerManager::getInstance()->eventAlreadyInList( this, events->getCurrent()->event, events->getCurrent()->service) )
				{
					hide();
					eTimerEditView v( events->getCurrent()->event, addtype, events->getCurrent()->service);
					v.show();
					v.exec();
					v.hide();
					invalidate();
					show();
				}
			}
			return 1;
		default:
			break;
	}
	return eWindow::eventHandler(event);
}
    
/* search for the presence of language from given EIT event descriptors*/
bool LocalEventData::language_exists(EITEvent *event, eString lang)
{
	ShortEventName=ExtendedEventText=ShortEventText="";
	bool retval=0;
	for (ePtrList<Descriptor>::iterator descriptor(event->descriptor); descriptor != event->descriptor.end(); ++descriptor)
	{
		if (descriptor->Tag() == DESCR_SHORT_EVENT)
		{
			ShortEventDescriptor *ss = (ShortEventDescriptor*)*descriptor;
			if (!lang || !strncmp(lang.c_str(), ss->language_code, 3) )
			{
				ShortEventName=ss->event_name;
				ShortEventText=ss->text;
				retval=1;
			}
		}
		else if (descriptor->Tag() == DESCR_EXTENDED_EVENT)
		{
			ExtendedEventDescriptor *ss = (ExtendedEventDescriptor*)*descriptor;
			if (!lang || !strncmp(lang.c_str(), ss->language_code, 3) )
			{
				ExtendedEventText += ss->text;
				retval=1;
			}
		}
	}
	return retval;
}

const char MAX_LANG = 37;
/* OSD language (see /share/locales/locales) to iso639 conversion table */    
eString ISOtbl[MAX_LANG][2] =
{
	{"ar_AE","ara"},
	{"C","eng"},
	{"cs_CZ","ces"},     /* or 'cze' */
	{"cs_CZ","cze"},
	{"da_DK","dan"},
	{"de_DE","deu"},     /* also 'ger' is valid iso639 code!! */
	{"de_DE","ger"},
	{"el_GR","gre"},     /* also 'ell' is valid */
	{"el_GR","ell"},
	{"es_ES","esl"},     /* also 'spa' is ok */
	{"es_ES","spa"},
	{"et_EE","est"},
	{"fi_FI","fin"},
	{"fr_FR","fra"},
	{"hr_HR","hrv"},     /* or 'scr' */
	{"hr_HR","scr"},
	{"hu_HU","hun"},
	{"is_IS","isl"},     /* or 'ice' */
	{"is_IS","ice"},
	{"it_IT","ita"},
	{"lt_LT","lit"},
	{"nl_NL","nld"},     /* or 'dut' */
	{"nl_NL","dut"},
	{"no_NO","nor"},
	{"pl_PL","pol"},
	{"pt_PT","por"},
	{"ro_RO","ron"},     /* or 'rum' */
	{"ro_RO","rum"},
	{"ru_RU","rus"},
	{"sk_SK","slk"},     /* or 'slo' */
	{"sk_SK","slo"},
	{"sl_SI","slv"},
	{"sr_YU","srp"},     /* or 'scc' */
	{"sr_YU","scc"},
	{"sv_SE","swe"},
	{"tr_TR","tur"},
	{"ur_IN","urd"}
};

LocalEventData::LocalEventData()
{
	if ( !country )
	{
		char *str=0;
		eConfig::getInstance()->getKey("/elitedvb/language", str); // fetch selected OSD country
		if (!str)
			eDebug("No OSD-language found!");
		else
		{
			country=str;
			free(str);
			for (int i=0; i < MAX_LANG; i++)
				if (country==ISOtbl[i][0])
					if (!primary_language)
						primary_language=ISOtbl[i][1];
					else
						secondary_language=ISOtbl[i][1];
		}
#if 0
		if ( country )
			eDebug("Country = %s",country.c_str());
		if ( primary_language )
			eDebug("Primary Language = %s",primary_language.c_str());
		if ( secondary_language )
			eDebug("Secondary Language = %s",secondary_language.c_str());
#endif
	}
}

/* short event name, short event text and extended event text */
void LocalEventData::getLocalData(EITEvent *event, eString *name, eString *desc, eString *text)
{
	if (!language_exists(event,primary_language))
		if (!language_exists(event,secondary_language))
			if (!language_exists(event,"eng"))
				language_exists(event,0);
	if ( name )
		*name=ShortEventName;
	if ( desc )
		*desc=ShortEventText;
	if ( text )
		*text=ExtendedEventText;
}

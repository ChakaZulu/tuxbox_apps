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
	for (ePtrList<Descriptor>::iterator d(event.descriptor); d != event.descriptor.end(); ++d)
	{
		Descriptor *descriptor=*d;
		if (descriptor->Tag()==DESCR_SHORT_EVENT)
		{
			descr = ((ShortEventDescriptor*)descriptor)->event_name;
			return;
		}
		else if (descriptor->Tag()==DESCR_TIME_SHIFTED_EVENT)
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
				for (ePtrList<Descriptor>::iterator d(evt->descriptor); d != evt->descriptor.end(); ++d)
				{
					if (d->Tag()==DESCR_SHORT_EVENT)
					{
						descr = ((ShortEventDescriptor*)descriptor)->event_name;
						break;
					}
				}
				delete evt;
				return;
			}
		}
	}
	descr = "no event data avail";
}

eListBoxEntryEPG::eListBoxEntryEPG(const eit_event_struct* evt, eListBox<eListBoxEntryEPG> *listbox, eServiceReference &ref)
		:eListBoxEntry((eListBox<eListBoxEntry>*)listbox), paraDate(0), paraTime(0), paraDescr(0), event(evt), service(ref)
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

	const timeMap* evt = eEPGCache::getInstance()->getTimeMap(current);
	timeMap::const_iterator It;
	if (evt)
		It = evt->begin();

	if (current.data[0] == 5 ) // NVOD REF ENTRY
	{
		for (; It != evt->end(); It++)
		{
			EITEvent evt(*It->second);   // NVOD Service Event
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
							EITEvent e(*pIt->second);
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
	else for (It = evt->begin(); It != evt->end(); It++)
		new eListBoxEntryEPG(*It->second, events, current);
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
				ei.setEvent(&tmp->event);
		}
		ei.hide();
		show();
	}
}

eEPGSelector::eEPGSelector(const eServiceReferenceDVB &service)
	:eWindow(0), current(service)
{
	events = new eListBox<eListBoxEntryEPG>(this);
	events->setName("events");
	events->setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight.background"), eSkin::getActive()->queryScheme("eServiceSelector.highlight.foreground"));

	if (eSkin::getActive()->build(this, "eEPGSelector"))
		eWarning("EPG selector widget build failed!");

	CONNECT(events->selected, eEPGSelector::entrySelected);
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

int eEPGSelector::eventHandler(const eWidgetEvent &event)
{
	int addtype=-1;
	switch (event.type)
	{
		case eWidgetEvent::evtAction:
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

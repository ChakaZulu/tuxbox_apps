#include "epgwindow.h"
#include "enigma_event.h"

#include <algorithm>
#include <iomanip>

#include <apps/enigma/enigma_lcd.h>
#include <core/driver/rc.h>
#include <core/gui/eskin.h>
#include <core/dvb/edvb.h>
#include <core/dvb/dvbservice.h>
#include <core/gdi/font.h>

gFont eListBoxEntryEPG::TimeFont;
gFont eListBoxEntryEPG::DescrFont;

eListBoxEntryEPG::~eListBoxEntryEPG()
{
	if (paraTime)
		paraTime->destroy();

	if (paraDescr)
		paraDescr->destroy();
}

int eListBoxEntryEPG::getEntryHeight()
{
	if (!DescrFont.pointSize && !TimeFont.pointSize)
	{
		DescrFont = eSkin::getActive()->queryFont("eEPGSelector.Entry.Description");
		TimeFont = eSkin::getActive()->queryFont("eEPGSelector.Entry.DateTime");
	}
	return calcFontHeight(DescrFont)+4;	
}

eListBoxEntryEPG::eListBoxEntryEPG(const eit_event_struct* evt, eListBox<eListBoxEntryEPG> *listbox)
		:eListBoxEntry((eListBox<eListBoxEntry>*)listbox), paraTime(0), paraDescr(0), event(evt)	
{	

		for (ePtrList<Descriptor>::iterator d(event.descriptor); d != event.descriptor.end(); ++d)
		{
			Descriptor *descriptor=*d;
			if (descriptor->Tag()==DESCR_SHORT_EVENT)
			{
				tm* t = localtime(&event.start_time);
 		
				time 	<< std::setw(2) << t->tm_mday << '.' << t->tm_mon+1 << ", "
							<< std::setw(2) << std::setfill('0') << t->tm_hour << ':' << std::setw(2) << t->tm_min;
				
				descr = ((ShortEventDescriptor*)descriptor)->event_name;

				return;
			}
		}
		time << "no event data avail";

}

eString eListBoxEntryEPG::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int hilited)
{
	drawEntryRect(rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, hilited);

	if (!paraTime && !paraDescr)
	{
		paraTime = new eTextPara( eRect( rect.left(), 0, rect.width(), rect.height()) );
		paraTime->setFont( TimeFont );
		paraTime->renderString(time.str());
		DescrXOffs = paraTime->getBoundBox().width()+paraTime->getBoundBox().height();

		paraDescr = new eTextPara( eRect( rect.left() ,0, rect.width(), rect.height()) );
		paraDescr->setFont( DescrFont );
		paraDescr->renderString(descr);

		DescrYOffs = ((rect.height() - paraDescr->getBoundBox().height()) / 2 ) - paraDescr->getBoundBox().top();
		TimeYOffs = ((rect.height() - paraTime->getBoundBox().height()) / 2 ) - paraTime->getBoundBox().top();
	}

	rc->renderPara(*paraTime, ePoint( rect.left(), rect.top() + TimeYOffs ) );
	rc->renderPara(*paraDescr, ePoint( rect.left()+DescrXOffs, rect.top() + DescrYOffs ) );

	return time.str()+" "+descr;
}

void eEPGSelector::fillEPGList()
{
  eService *service=eDVB::getInstance()->settings->getTransponders()->searchService(current);
  if (service)
		setText(eString("EPG - ")+service->service_name);
 	eDebug("get EventMap for onid: %02x, sid: %02x", current.getOriginalNetworkID().get(), current.getServiceID().get());
	const eventMap* evt = eEPGCache::getInstance()->getEventMap(current);
	eventMap::const_iterator It;
	for (It = evt->begin(); It != evt->end(); It++)
		new eListBoxEntryEPG(*It->second, events);
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
		eZapLCD* pLCD = eZapLCD::getInstance();
		pLCD->lcdMain->hide();
		pLCD->lcdMenu->show();
	  eService *service=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
		eEventDisplay ei(service ? service->service_name.c_str() : "", 0, &entry->event);
		ei.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
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
		pLCD->lcdMenu->hide();
		pLCD->lcdMain->show();
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
}

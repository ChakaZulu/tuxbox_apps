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
int eListBoxEntryEPG::TimeYOffs(-1);
int eListBoxEntryEPG::TimeFontHeight(-1);

eListBoxEntryEPG::~eListBoxEntryEPG()
{
	if (paraTime)
	{
		paraTime->destroy();
		paraTime = 0;
	}
	if (paraDescr)
	{
		paraDescr->destroy();
		paraDescr = 0;
	}

}

eListBoxEntryEPG::eListBoxEntryEPG(const eit_event_struct* evt, eListBox<eListBoxEntryEPG> *listbox)
		:eListBoxEntry((eListBox<eListBoxEntry>*)listbox), paraTime(0), paraDescr(0), event(evt)	
{	
		if (!DescrFont.pointSize && !TimeFont.pointSize)
		{
			DescrFont = eSkin::getActive()->queryFont("eEPGSelector.Entry.Description");
			TimeFont = eSkin::getActive()->queryFont("eEPGSelector.Entry.DateTime");
		}

		if ( TimeYOffs == -1 && TimeFontHeight == -1)
		{
			eRect rect(0,0,400,50);
			eTextPara* tmp = new eTextPara(rect);
			tmp->setFont( TimeFont );
			tmp->renderString("0123456789");			
			eSize s1 = tmp->getExtend();
			tmp->destroy();

			tmp = new eTextPara(rect);
			tmp->setFont( DescrFont );
			tmp->renderString("ABCefg");
			eSize s2 = tmp->getExtend();
			tmp->destroy();

			TimeFontHeight = s1.height();
			TimeYOffs = (s2.height() - TimeFontHeight) / 2;
		}


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

void eListBoxEntryEPG::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int hilited)
{
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

	if (!paraTime && !paraDescr)
	{
		eRect r(rect);
		r.setTop( r.top() + TimeYOffs );
		paraTime = new eTextPara(r);
		paraTime->setFont( TimeFont );
		paraTime->renderString(time.str());
		
		int DescrXOffs = paraTime->getExtend().width() - rect.left();
		r = rect;
		r.setLeft( r.left() + DescrXOffs + TimeFontHeight );
		paraDescr = new eTextPara(r);
		paraDescr->setFont( DescrFont );
		paraDescr->renderString(descr);
	}
	rc->renderPara(*paraTime);
	rc->renderPara(*paraDescr);

	eWidget* p = listbox->getParent();			
	if (hilited && p && p->LCDElement)
		p->LCDElement->setText(time.str()+" "+descr);
}

void eEPGSelector::fillEPGList()
{
  eService *service=eDVB::getInstance()->settings->getTransponders()->searchService(current);
  if (service)
		setText(eString("EPG - ")+service->service_name);
 	eDebug("get EventMap for onid: %02x, sid: %02x", current.original_network_id.get(), current.service_id.get());
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

eEPGSelector::eEPGSelector(const eServiceReference &service)
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

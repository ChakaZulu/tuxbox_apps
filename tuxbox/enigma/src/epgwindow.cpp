#include "epgwindow.h"
#include "enigma_event.h"

#include <algorithm>
#include <iomanip>

#include <apps/enigma/enigma_lcd.h>
#include <core/driver/rc.h>
#include <core/gui/eskin.h>
#include <core/dvb/edvb.h>
#include <core/dvb/dvbservice.h>

eListBoxEntryEPG::eListBoxEntryEPG(const eit_event_struct* evt, eListBox<eListBoxEntryEPG> *listbox)
		:eListBoxEntryTextStream((eListBox<eListBoxEntryTextStream>*)listbox), event(evt)	
{	
		for (ePtrList<Descriptor>::iterator d(event.descriptor); d != event.descriptor.end(); ++d)
		{
			Descriptor *descriptor=*d;
			if (descriptor->Tag()==DESCR_SHORT_EVENT)
			{
				tm* t = localtime(&event.start_time);
 		
				text 	<< std::setw(2) << t->tm_mday << '.' << t->tm_mon+1 << ", "
							<< std::setw(2) << std::setfill('0') << t->tm_hour << ':' << std::setw(2) << t->tm_min << "  "
 							<< ((ShortEventDescriptor*)descriptor)->event_name;

				return;
			}
		}
		text << "no_name";
}

void eEPGWindow::fillEPGList()
{
  eService *service=eDVB::getInstance()->settings->getTransponders()->searchService(current);
  if (service)
		setText(eString("EPG - ")+service->service_name);
 	eDebug("get EventMap for onid: %02x, sid: %02x\n", current.original_network_id.get(), current.service_id.get());
	const eventMap* evt = eEPGCache::getInstance()->getEventMap(current);
	eventMap::const_iterator It;
	for (It = evt->begin(); It != evt->end(); It++)
		new eListBoxEntryEPG(*It->second, &list);
}

void eEPGWindow::entrySelected(eListBoxEntryEPG *entry)
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
				tmp=list.goPrev();
			else if (ret == 2)
				tmp=list.goNext();
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

eEPGWindow::eEPGWindow(const eServiceReference &service)
	:eListBoxWindow<eListBoxEntryEPG>("Select Service...", 16, 600), current(service)
{
	move(ePoint(50, 50));
	list.setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight"));
	CONNECT(list.selected, eEPGWindow::entrySelected);
	fillEPGList();
}

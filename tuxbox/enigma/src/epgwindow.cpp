#include "epgwindow.h"
#include "enigma_event.h"

#include <algorithm>

#include <core/driver/rc.h>
#include <core/gui/eskin.h>
#include <core/dvb/edvb.h>


eString eListboxEntryEPG::getText(int col) const
{
		tm* t = localtime(&event->start_time);
		eString DateTime;
		DateTime.sprintf("%2d.%d, %02d:%02d   ", t->tm_mday, t->tm_mon+1, t->tm_hour, t->tm_min);
		for (ePtrList<Descriptor>::iterator d(event->descriptor); d != event->descriptor.end(); ++d)
		{
			Descriptor *descriptor=*d;
			if (descriptor->Tag()==DESCR_SHORT_EVENT)
			{
				ShortEventDescriptor *ss=(ShortEventDescriptor*)descriptor;
				return DateTime+ss->event_name;
			}
		}
		return eString("no_name");
}

void eEPGWindow::fillEPGList()
{
	setText(eString("EPG - ")+current->service_name.c_str());
	eDebug("get EventMap for onid: %02x, sid: %02x\n", current->original_network_id, current->service_id);
	const eventMap* evt = eEPGCache::getInstance()->getEventMap(current->original_network_id, current->service_id);
	eventMap::const_iterator It;
	for (It = evt->begin(); It != evt->end(); It++)
		new eListboxEntryEPG(new EITEvent(*It->second) , &list);
}

void eEPGWindow::entrySelected(eListboxEntry *entry)
{
	if (!entry)
		close(0);
	else
	{	
		int ret;
		hide();
		eEventDisplay ei(eDVB::getInstance()->service->service_name.c_str(), 0, ((eListboxEntryEPG*)entry)->event);
		ei.show();
		while(ret = ei.exec())
		{
			eListboxEntryEPG* tmp;

			if (ret==-1)
				tmp=(eListboxEntryEPG*)list.goPrev();
			else if (ret == 1)
				tmp=(eListboxEntryEPG*)list.goNext();

			if (tmp)
				ei.setEvent(tmp->event);					
		}
		ei.hide();
		show();
	}
}

eEPGWindow::eEPGWindow(eService* service):current(service),
								eLBWindow("Select Service...", 16, eSkin::getActive()->queryValue("fontsize", 20), 600)
{
	move(ePoint(50, 50));
	list.setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight"));
	CONNECT(list.selected, eEPGWindow::entrySelected);
	fillEPGList();
}

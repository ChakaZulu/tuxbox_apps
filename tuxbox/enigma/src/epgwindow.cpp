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
	hide();
	eEventDisplay ei(eDVB::getInstance()->service->service_name.c_str(), 0, ((eListboxEntryEPG*)entry)->event);
	ei.show();
	ei.exec();
	ei.hide();
	show();
}

int eEPGWindow::eventFilter(const eWidgetEvent &event)
{
#if 0
	switch (event.type)
	{
		case eWidgetEvent::keyUp:
		{
			switch (event.parameter)
			{
				case eRCInput::RC_HELP:
				case eRCInput::RC_RED:
				case eRCInput::RC_HOME:
				case eRCInput::RC_DBOX:
					closeTimer.start(100,1);   // without Timer ... no return 1 :-(
				return 1;

				case eRCInput::RC_OK:
					return 0;
			}
			return 1;
		}

		case eWidgetEvent::keyDown:
		{
			switch (event.parameter)
			{	
					case eRCInput::RC_LEFT:
					case eRCInput::RC_RIGHT:					
					case eRCInput::RC_UP:
					case eRCInput::RC_DOWN:
						return 0;
			}
			return 1;	
		}
	}
	return 0;
#endif
}

void eEPGWindow::closeWnd()
{
	close(0);
}

eEPGWindow::eEPGWindow(eService* service):current(service), closeTimer(eApp),
								eLBWindow("Select Service...", 16, eSkin::getActive()->queryValue("fontsize", 20), 600)
{
	move(ePoint(50, 50));
	list.setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight"));
	CONNECT(closeTimer.timeout, eEPGWindow::closeWnd);
	CONNECT(list.selected, eEPGWindow::entrySelected);

	fillEPGList();
}

#ifndef __src_epgactions_h
#define __src_epgactions_h

struct epgSelectorActions
{
  eActionMap map;
	eAction addDVRTimerEvent, addNGRABTimerEvent, addSwitchTimerEvent,
		removeTimerEvent, showExtendedInfo;
	epgSelectorActions():
		map("epgSelector", _("EPG selector")),
		addDVRTimerEvent(map, "addDVRTimerEvent", _("add this event as DVR Event to timer list"), eAction::prioDialog ),
		addNGRABTimerEvent(map, "addNGRABTimerEvent", _("add this event as NGRAB Event to timer list"), eAction::prioDialog ),
		addSwitchTimerEvent(map, "addSwitchTimerEvent", _("add this event as simple Switch Event to timer list"), eAction::prioDialog ),
		removeTimerEvent(map, "removeTimerEvent", _("remove this event from timer list"), eAction::prioDialog ),
		showExtendedInfo(map, "showExtendedInfo", _("show extended event information"), eAction::prioDialog )
	{
	}
};

extern eAutoInitP0<epgSelectorActions> i_epgSelectorActions;

#endif

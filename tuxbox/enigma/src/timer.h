#ifndef __apps__enigma__timer_h
#define __apps__enigma__timer_h

#include <core/dvb/serviceplaylist.h>
#include <apps/enigma/epgwindow.h>

class eTimerManager: public Object
{
	static eTimerManager *instance;

// eTimerManager actionHandler stuff
	enum
	{
		zap, showMessage, startCountdown, setNextEvent,
		startEvent, pauseEvent, restartEvent, stopEvent,
		startRecording, restartRecording, pauseRecording, stopRecording
	};
	int nextAction;

	eTimer actionTimer;  // to start timer related actions
	void actionHandler(); // the action Handler
///////////////////////////

// for multiple use timer and connection objects..
	eTimer timer;					
	Connection conn, conn2;

// the timerlist self...
	ePlaylist *timerlist;
	eServiceReference timerlistref;

// nextStarting event, or the current running Event
	std::list<ePlaylistEntry>::iterator nextStartingEvent;

// both methods are NOT always connected to the eDVB Signals
	void serviceChanged( const eServiceReferenceDVB& );
	void leaveService( const eServiceReferenceDVB& );

	long getSecondsToBegin();
	long getSecondsToEnd();

// this Method is called multiple at the start of the eTimerManager....
	void waitClock();

// handle all eit related timer stuff ( for smart Timers)
	void EITready(int);
public:
	eTimerManager();
	~eTimerManager();
	static eTimerManager *getInstance() { return instance; }
  bool removeEventFromTimerList( eWidget *sel, const ePlaylistEntry& entry );
  bool removeEventFromTimerList( eWidget *sel, const eServiceReference *ref, const EITEvent *evt );
  bool addEventToTimerList( eEPGSelector *sel, const eServiceReference *ref, const EITEvent *evt );
	ePlaylistEntry* findEvent( eServiceReference *service, EITEvent *evt );
	template <class Z>
	void forEachEntry(Z ob)
	{
		if (timerlist)
			for (std::list<ePlaylistEntry>::iterator i(timerlist->list.begin()); i != timerlist->list.end(); ++i)
				ob(&*i);
	}
};

class eEPGContextMenu: public eListBoxWindow<eListBoxEntryText>
{
	void entrySelected(eListBoxEntryText *s);
public:
	eEPGContextMenu();
};

class eListBoxEntryTimer: public eListBoxEntry
{
	friend class eListBox<eListBoxEntryTimer>;
	friend class eTimerView;
	static gFont TimeFont, DescrFont;
	static gPixmap *ok, *failed;
	static int timeXSize, dateXSize;
	int TimeYOffs, DescrYOffs;
	eTextPara *paraDate, *paraTime, *paraDescr;
	const ePlaylistEntry *entry;
	eString redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int hilited);
	static int getEntryHeight();
public:
	eListBoxEntryTimer(eListBox<eListBoxEntryTimer> *listbox, const ePlaylistEntry *entry);
	~eListBoxEntryTimer();
};

class eTimerView: public eWindow
{
	eListBox<eListBoxEntryTimer>* events;
private:
	void fillTimerList();
	void entrySelected(eListBoxEntryTimer *entry);
	int eventHandler(const eWidgetEvent &event);
	void invalidateEntry( eListBoxEntryTimer* );
public:
	eTimerView();
	~eTimerView(){};
};
/*
class eTimerConfig: public eWindow
{
	eComboBox *type, // single, daily, weekly
						*weekday; // for weekly
	eCheckbox *smartTimerMode;
	eLabel *beginTime, *endTime;
	eNumber *advanceTime, *runAfterTime;
	time_t begTime, int duration;
public:
	eTimerConfig(time_t begTime, int duration, int advanceTime, int runAfterTime )
		:begTime(begTime), duration(duration)
	{
	}
};*/


#endif

#ifndef __apps__enigma__timer_h
#define __apps__enigma__timer_h

#include <lib/dvb/serviceplaylist.h>
#include <lib/gui/combobox.h>
#include <lib/gui/enumber.h>
#include <epgwindow.h>

class eTimerManager: public Object
{
	static eTimerManager *instance;

// eTimerManager actionHandler stuff
	enum
	{
		zap, showMessage, startCountdown, setNextEvent,
		startEvent, pauseEvent, restartEvent, stopEvent,
		toggleRecording, restartRecording, pauseRecording
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
	enum { erase, update };
	eTimerManager();
	~eTimerManager();
	static eTimerManager *getInstance() { return instance; }
  bool removeEventFromTimerList( eWidget *sel, const ePlaylistEntry& entry, int type=erase );
  bool removeEventFromTimerList( eWidget *sel, const eServiceReference *ref, const EITEvent *evt);
  bool addEventToTimerList( eWidget *sel, const eServiceReference *ref, const EITEvent *evt, int type = ePlaylistEntry::SwitchTimerEntry | ePlaylistEntry::stateWaiting );
	bool addEventToTimerList( eWidget *sel, const ePlaylistEntry& entry );
	ePlaylistEntry* findEvent( eServiceReference *service, EITEvent *evt );
	template <class Z>
	void forEachEntry(Z ob)
	{
		if (timerlist)
			for (std::list<ePlaylistEntry>::iterator i(timerlist->list.begin()); i != timerlist->list.end(); ++i)
				ob(&*i);
	}
};

class eListBoxEntryTimer: public eListBoxEntry
{
	friend class eListBox<eListBoxEntryTimer>;
	friend class eTimerView;
	friend struct _selectEvent;
	static gFont TimeFont, DescrFont;
	static gPixmap *ok, *failed;
	static int timeXSize, dateXSize;
	int TimeYOffs, DescrYOffs;
	eTextPara *paraDate, *paraTime, *paraDescr;
	ePlaylistEntry *entry;
	eString redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int hilited);
	static int getEntryHeight();
public:
	bool operator < ( const eListBoxEntryTimer& ref ) const
	{
		return entry->time_begin < ref.entry->time_begin;
	}
	eListBoxEntryTimer(eListBox<eListBoxEntryTimer> *listbox, ePlaylistEntry *entry);
	~eListBoxEntryTimer();
};

class eTimerView: public eWindow
{
	eListBox<eListBoxEntryTimer>* events;
	eComboBox *bday, *bmonth, *byear, *eday, *emonth, *eyear, *type, *services;
	eNumber *btime, *etime;
	eButton *add, *update, *erase, *bclose;
	tm beginTime, endTime;
	friend struct _selectEvent;
private:
	void selectServiceInCombo( const eServiceReference& ref );
	void comboBoxClosed( eComboBox*, eListBoxEntryText* );
	void selChanged( eListBoxEntryTimer* );
	void fillTimerList();
	void entrySelected(eListBoxEntryTimer *entry);
	int eventHandler(const eWidgetEvent &event);
	void invalidateEntry( eListBoxEntryTimer* );
	void updateDateTime( const tm& beginTime, const tm& endTime );
	void updateDay( eComboBox* dayCombo, int year, int month, int day );
	void updatePressed();
	void selectEvent( ePlaylistEntry* e );
	void addPressed();
	void erasePressed();
	void focusNext(int*)
	{
		eWidget::focusNext(eWidget::focusDirNext);
	}
	bool getData( time_t& beginTime, int& duration );
public:
	eTimerView(ePlaylistEntry* e=0);
	~eTimerView(){};
};

#endif

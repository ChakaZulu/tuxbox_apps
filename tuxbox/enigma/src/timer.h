#ifndef __apps__enigma__timer_h
#define __apps__enigma__timer_h

#include <core/dvb/serviceplaylist.h>
#include <apps/enigma/epgwindow.h>

class eTimerManager: public Object
{
	static eTimerManager *instance;
	eTimer timer;
	ePlaylist *timerlist;
	eServiceReference timerlistref;
	std::list<ePlaylistEntry>::iterator nextStartingEvent;
	Connection conn, conn2; // connection object for timer.. to disconnect...

	long getSecondsToBegin();
	long getSecondsToEnd();
//	eAUTable<EIT> tEIT;
	void waitClock();
	void setNextEvent();
	void viewTimerMessage();
	void zapToChannel();
	void startActiveTimerMode();
	void startEvent();
	void pauseEvent();
	void stopEvent( int state );
	void restartEvent();
	void serviceChanged( const eServiceReferenceDVB&, int );

	void EITready(int);  // handle all eit related timer stuff
	void restartRecording();

	void leaveService( const eServiceReferenceDVB& );
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

#ifndef __src_enigma_epg
#define __src_enigma_epg

#include <lib/gui/ewidget.h>
#include <lib/gui/ewindow.h>
#include <lib/gui/elabel.h>
#include <lib/dvb/dvb.h>

class EITEvent;
class eStatusBar;

class eZapEPG: public eWindow
{
	gFont timeFont, titleFont, descrFont;
	gColor entryColor, entryColorSelected;
	unsigned int offs, focusColumn, hours, numservices;
	eStatusBar *sbar;
	eWidget *eventWidget;
	struct serviceentry;
	struct entry: public eWidget
	{
		gFont &timeFont, &titleFont, &descrFont;
		gColor entryColor, entryColorSelected;
		eWidget *sbar;
		void redrawWidget(gPainter *target, const eRect &area);
		void gotFocus();
		void lostFocus();
	public:
		struct serviceentry *service;
		static gPixmap *inTimer;
		static gPixmap *inTimerRec;
		time_t start;
		int duration;
		int event_id;
		eString title, description;
		void setActive(int active);
		const EITEvent *event;
		entry(eWidget *parent, gFont &timeFont, gFont &titleFont, gFont &descrFont, gColor entryColor, gColor entryColorSelected, eWidget *sbar );
		~entry();
	};
	
	struct serviceentry
	{
		eRect pos;
		eLabel *header;
		eServiceReferenceDVB service;
		ePtrList<entry> entries;
		ePtrList<entry>::iterator current_entry;
		serviceentry() : header(0), current_entry(entries.end()) { }
		~serviceentry() { delete header; }
	};
	std::list<eServiceReferenceDVB> services;
	std::list<eServiceReferenceDVB>::iterator curS, curE;
	std::list<serviceentry> serviceentries;
	std::list<serviceentry>::iterator current_service;
	int eventHandler(const eWidgetEvent &event);
	void buildService(serviceentry &service, time_t start, time_t end);
	void selService(int dir);
	void selEntry(int dir);
public:
	std::list<serviceentry>::iterator& getCurSelected() { return current_service; }
	void addToList( const eServiceReference& ref );
	eZapEPG();
	void buildPage(int direction);
};

#endif

#ifndef __sselect_h
#define __sselect_h

#include <lib/gui/ewindow.h>
#include <lib/gui/listbox.h>
#include <lib/gui/statusbar.h>
#include <lib/dvb/epgcache.h>
#include <src/channelinfo.h>

#include <stack>
#include <lib/dvb/service.h>

class eService;
class eLabel;

class eListBoxEntryService: public eListBoxEntry
{
	friend class eServiceSelector;
	friend class eListBox<eListBoxEntryService>;
	friend struct moveFirstChar;
	friend struct moveServiceNum;
	friend struct _selectService;
	friend struct updateEPGChangedService;
	friend struct renumber;
	eString sort;
	eString short_name;
	static gFont serviceFont, descrFont, numberFont;
	static int maxNumSize;
	static gPixmap *folder;
	eTextPara *numPara, *namePara, *descrPara;
	int nameXOffs, descrXOffs, numYOffs, nameYOffs, descrYOffs;
	int flags;
	int num;
public:
	static eListBoxEntryService *selectedToMove;
	static std::map< eServiceReference, int> favourites;
	int getNum() const { return num; }
	void invalidate();
	void invalidateDescr();
	static int getEntryHeight();
	eServiceReference service;
	enum { flagShowNumber=1, flagOwnNumber=2 };
	eListBoxEntryService(eListBox<eListBoxEntryService> *lb, const eServiceReference &service, int flags, int num=-1);
	~eListBoxEntryService();

	bool operator<(const eListBoxEntryService &r) const
	{
		if (service.getSortKey() == r.service.getSortKey())
			return sort < r.sort;
		else
			return service.getSortKey() > r.service.getSortKey();		// sort andersrum
	}
protected:
	eString redraw(gPainter *rc, const eRect &rect, gColor, gColor, gColor, gColor, int hilited);
};

class eServiceSelector: public eWindow
{
	eServiceReference selected;
	eServiceReference *result;
	eListBox<eListBoxEntryService> *services, *bouquets;

	eChannelInfo* ci;

	eServicePath path;

	void addService(const eServiceReference &service);
	void addBouquet(const eServiceReference &service);
	int style;
	int serviceentryflags;

	char BrowseChar;
	eTimer BrowseTimer;
	eTimer ciDelay;

protected:
	int eventHandler(const eWidgetEvent &event);
private:
	void fillServiceList(const eServiceReference &ref);
	void fillBouquetList(const eServiceReference &ref);
	void serviceSelected(eListBoxEntryService *entry);
	void bouquetSelected(eListBoxEntryService *entry);
	void serviceSelChanged(eListBoxEntryService *entry);
	void bouquetSelChanged( eListBoxEntryService *entry);
	void ResetBrowseChar();
	void gotoChar(char c);
	void EPGUpdated( const tmpMap* );
	void updateCi();
public:
	int movemode;
	int FavouriteMode;
	enum { styleInvalid, styleCombiColumn, styleSingleColumn, styleMultiColumn };
	enum { dirNo, dirUp, dirDown };

	eServiceSelector();
	~eServiceSelector();

	Signal1<void,const eServiceReference &> addServiceToList, removeServiceFromFavourite;
	Signal2<void,eServiceSelector*,int> addServiceToFavourite;
	Signal1<void,eServiceSelector*> showFavourite, showMenu, toggleStyle;
	Signal1<void,int> setMode;
	Signal3<void,
		const eServiceReference &, 		// path
		const eServiceReference &, 		// service to move
		const eServiceReference &			// service AFTER moved service
		> moveEntry;

	const eServicePath &getPath()	{	return path; }
	void setPath(const eServicePath &path, const eServiceReference &select=eServiceReference());

	int getStyle()	{ return style; }
	void setStyle(int newStyle=-1);	
	void actualize();
	bool selectService(const eServiceReference &ref);
	bool selectService(int num);	
	bool selectServiceRecursive( eServiceReference &ref );
	bool selServiceRec( eServiceReference &ref );
	int getServiceNum( const eServiceReference &ref);
	void enterDirectory(const eServiceReference &ref);
	const eServiceReference &getSelected() { return selected; }
	const eServiceReference *choose(int irc=-1);
	const eServiceReference *next();
	const eServiceReference *prev();

	void toggleMoveMode();
	void toggleFavouriteMode();
};

#endif

#ifndef __epgsearch_h
#define __epgsearch_h 



#include <stdio.h>
#include <lib/gui/ewindow.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/emessage.h>
#include <lib/gui/listbox.h>
#include <lib/gui/combobox.h>

#include <lib/dvb/dvb.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/epgcache.h>
#include <lib/dvb/servicestructure.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/service.h>
#include <lib/dvb/record.h>
#include <lib/dvb/serviceplaylist.h>
#include <lib/gdi/fb.h>
#include <lib/gdi/glcddc.h>
#include <lib/gdi/gfbdc.h>
#include <lib/gdi/epng.h>
#include <lib/gui/emessage.h>
#include <lib/system/http_dyn.h>
#include <lib/system/econfig.h>
#include <lib/system/info.h>
#include <lib/system/dmfp.h>
#include <lib/system/file_eraser.h>
#include <lib/movieplayer/movieplayer.h>
#include <src/enigma_dyn.h>
#include <src/enigma_dyn_utils.h>
#include <src/enigma_dyn_mount.h>
#include <src/enigma_dyn_wap.h>
#include <src/enigma_dyn_conf.h>
#include <src/enigma_dyn_flash.h>
#include <src/enigma_dyn_rotor.h>
#include <src/enigma_dyn_xml.h>
#include <src/enigma_dyn_misc.h>
#include <src/enigma_dyn_epg.h>
#include <src/enigma_dyn_timer.h>
#include <src/enigma_dyn_pda.h>
#include <src/enigma_dyn_movieplayer.h>
#include <src/enigma_streamer.h>
#include <src/enigma_processutils.h>
#include <src/epgwindow.h>
#include <src/streaminfo.h>
#include <src/enigma_mount.h>
#include <lib/gui/textinput.h>
#include <lib/gui/echeckbox.h>

struct SEARCH_REFERENCES
{
	eServiceReference SearchRef;
};
typedef std::vector<SEARCH_REFERENCES> SearchReferences;

struct SEARCH_EPG_DATA
{
	eServiceReference ref;
	eString name;
	int start_time;
	int duration;
	eString title;
	
};

typedef std::vector<SEARCH_EPG_DATA> SearchEPGDATA;

struct EPGSEARCHDATA
{
	int EventStart;
	eString title;
	EITEvent *event;
	eServiceReference ref;
};
typedef std::vector<EPGSEARCHDATA> vecEPGSearch;

struct EPGSEARCHLIST
{
	eServiceReference ref;
	eString name;
	vecEPGSearch EPGSearchData;
};
typedef std::vector<EPGSEARCHLIST> EPGSearchList;

class eEPGSearch: public eWindow
{
	
	
	eTextInputField *InputName;
	eStatusBar *status;
	eCheckbox *chkAllServices, *chkCurrentService, *chkExactMatch, *chkCaseSensitive;
	eComboBox *cboGenre;
	
	eString sServiceReference;
	eString sServiceReferenceSearch;
	int canCheck;
	

	int intAllServices;
	int intExactMatch;
	int intCaseSensitive;
	eButton *bt_abort, *bt_seach;
	
	void Search();
	eString Titel;
	void chkAllServicesStateChanged(int state);
	void chkCurrentServiceStateChanged(int state);
	void cboGenreChanged(eListBoxEntryText*);
	int Searching(eString SearchName);
	eString SearchName;
	void init_eEPGSearch(eServiceReference ref);
public:
	eEPGSearch(eServiceReference ref, EITEvent e);
	eEPGSearch(eServiceReference ref, eString CurrentEventName);
	eEPGSearch();
	eString getSearchName();
	int EPGSearching(eString SearchName, eServiceReference SearchRef, int AllServices, int ExactMatch, int CaseSensitive, int genre);
	int EPGSearching(eString SearchName, SearchReferences SearchRefs, int ExactMatch, int CaseSensitive, int TimeSpanSearch, tm beginTime, tm endTime, int Days, int Max_Duration );
	int EPGSearching(eString SearchName, int ExactMatch, int CaseSensitive, int TimeSpanSearch, tm beginTime, tm endTime, int Days, int Max_Duration );
};

class eEPGSearchDATA
{
	static eEPGSearchDATA *instance;
	int numCounter;
	eServiceReference userTVBouquets;
	eServiceReference oldService;
public:
	static eEPGSearchDATA *getInstance() { return instance; }

	eEPGSearchDATA();
	SearchEPGDATA getSearchData();
	void clearList();
	eServiceReference getService();
	eServiceReference GetOldService() {return oldService;}
};

#endif /* __epgsearch_h */


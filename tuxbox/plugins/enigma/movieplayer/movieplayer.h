#include <lib/gui/emessage.h>
#include <lib/gui/eskin.h>
#include <lib/gui/listbox.h>
#include <lib/gui/ewindow.h>
#include <lib/system/econfig.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

class PLAYLIST
{
 public:
 	int Filetype, Extitem;
	eString Filename, Fullname;
};

class EXTLIST
{
 public:
 	int ITEM;
	eString NAME, EXT, VRATE, ARATE, VCODEC, VSIZE;
	bool VTRANS, ATRANS, AC3;
};

typedef std::vector<PLAYLIST> PlayList;
typedef std::vector<EXTLIST> ExtList;

class eSCGui: public eWindow
{
	enum{GOUP, DIRS, FILES};
	enum{DATA, VCD, SVCD, DVD};

	ExtList extList;
	PlayList playList;

	eString startdir, pathfull, cddrive;
	eString str_mpeg1, str_mpeg2, str_audio;
	int a_pit, v_pit, a_type;
	int MODE, next_val;
	bool menu;
	eListBox<eListBoxEntryText> *list;
	eTimer *timer;
	eMessageBox *bufferingBox;

	void loadList();
	void viewList();
	void setStatus(int val);
	bool loadXML(eString file);

	void listSelected(eListBoxEntryText *item);
	void listSelChanged(eListBoxEntryText *item);
	int eventHandler(const eWidgetEvent &);

	void parseSout(int val);
	void playerStop();
	void playerStart(int val);
	void showMenu();
public:
	eSCGui();
	~eSCGui();
};

class eSCGuiInfo: public eWindow
{
public:
	void info();
};


class VLCsend
{
	CURLcode sendGetRequest (const eString & url, eString & response);
public:
	struct server_value
	{
		eString IP, IF_PORT, STREAM_PORT, AUTH, RESPONSE;
	}send_parms;
	
	static VLCsend *getInstance();
	void send(eString val);
};


struct player_value
{
	int STAT, BUFFERTIME, JUMPMIN;
	bool ACT_AC3, BUFFERFILLED, AVPIDS_FOUND;
	unsigned short PIDA, PIDV;
	short AC3;
};

class eSCplay
{
	static int PlayStreamThread (void *mrl);
	static int VlcGetStreamTime();
	//static int VlcGetStreamLength();
	static int cnt;
public:
	enum{STOPPED, PREPARING, STREAMERROR, PLAY, PAUSE, FF, REW, RESYNC, JF, JB, SKIP, SOFTRESET = 99};

	static player_value play_parms;
	static void playnow();
};



#ifndef __enigma_main_h
#define __enigma_main_h

#include <epgwindow.h>
#include <enigma_lcd.h>
#include <lib/dvb/si.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/edvb.h>
#include <lib/gui/ewindow.h>
#include <lib/gui/listbox.h>
#include <lib/gui/multipage.h>
#include <lib/gui/emessage.h>
#include <lib/gui/numberactions.h>
#include <lib/base/message.h>
#include <lib/dvb/service.h>


class eLabel;
class eProgress;
class EIT;
class SDT;
class SDTEntry;
class PMT;
class PMTEntry;
class eNumber;
class gPainter;
class NVODReferenceEntry;
class eServiceSelector;
class eRecordingStatus;

class eZapMessage
{
	int type;
	eString caption, body;
	int timeout;
public:
	eZapMessage(int type, const eString &caption, const eString &body, int timeout=0)
		: type(type), caption(caption), body(body), timeout(timeout)
	{
	}

	eZapMessage::eZapMessage()
	{
	}

	eZapMessage::eZapMessage(int type)
		: type(type)
	{
	}

	const eString & getCaption() const { return caption; }
	const eString & getBody() const { return body; }
	void setTimeout(int _timeout)
	{
		timeout=_timeout;
	}
	int getTimeout() const
	{
		return timeout;
	}
	bool isSameType(const eZapMessage &msg) const
	{
		return type == msg.type;
	}
	bool isEmpty() const
	{
		return ! (caption.length() || body.length());
	}
};

class NVODStream: public eListBoxEntryTextStream
{
	friend class eListBox<NVODStream>;
	friend class eNVODSelector;
private:
	void EITready(int error);
public:
	NVODStream(eListBox<NVODStream> *listbox, const NVODReferenceEntry *ref, int type);
	eServiceReferenceDVB service;
	EIT eit;
};

class NVODReferenceEntry;

class eNVODSelector: public eListBoxWindow<NVODStream>
{
private:
	void selected(NVODStream *);
public:
	eNVODSelector();
	void clear();
	void add(NVODReferenceEntry *ref);
};

class AudioStream: public eListBoxEntryText
{
	friend class eListBox<AudioStream>;
	friend class eAudioSelector;
  int isAC3;
  int component_tag;
  void EITready(int error);
  void parseEIT(EIT* eit);
public:
	enum
  {
		audioMPEG, audioAC3
	};
	AudioStream(eListBox<AudioStream> *listbox, PMTEntry *stream);
	PMTEntry *stream;
	bool operator < ( const AudioStream& e) const	{	return 0;	}
};

class eAudioSelector: public eListBoxWindow<AudioStream>
{
	void selected(AudioStream *);
public:
	eAudioSelector();
	void clear();
	void add(PMTEntry *);
};

class LinkageDescriptor;

class SubService: public eListBoxEntryText
{
	friend class eListBox<SubService>;
	friend class eSubServiceSelector;
public:
	SubService(eListBox<SubService> *listbox, const LinkageDescriptor *descr);
	eServiceReferenceDVB service;
};

class eSubServiceSelector: public eListBoxWindow<SubService>
{
	void selected(SubService *);
public:
	eSubServiceSelector();
	void clear();
	void add(const LinkageDescriptor *ref);
};

class eServiceNumberWidget: public eWindow
{
	eNumber *number;
	int chnum;
	eTimer *timer;
private:
	void selected(int*);
	void timeout();
public:
	eServiceNumberWidget(int initial);
	~eServiceNumberWidget();
};

#define ENIGMA_NVOD		1	
#define ENIGMA_AUDIO	2
#define ENIGMA_SUBSERVICES 4

class eEventDisplay;
class eServiceEvent;

class ePlaylist;

class eZapMain: public eWidget
{
public:
	enum { modeTV, modeRadio, modeFile, modePlaylist, modeEnd };
	enum { stateNormal, stateSleeping, stateInTimerMode, stateRecording, 
		stateMask=7,
		recDVR=8, recVCR=16 };

private:
	eLabel 	*ChannelNumber, *ChannelName, *Clock, *EINow, *EINext,
		*EINowDuration, *EINextDuration, *EINowTime, *EINextTime,
		*Description,
		*ButtonRedEn, *ButtonRedDis, 
		*ButtonGreenEn, *ButtonGreenDis, 
		*ButtonYellowEn, *ButtonYellowDis,
		*ButtonBlueEn, *ButtonBlueDis;
	
	eLabel *DolbyOn, *DolbyOff, *CryptOn, *CryptOff, *WideOn, *WideOff;
	eLabel mute, volume;
	
	eWidget *dvrFunctions;
	int dvrfunctions;
	
	eRecordingStatus *recstatus;
	
	eProgress *Progress, VolumeBar;
	eMessageBox *pMsg;

	eLock messagelock;
	std::list<eZapMessage> messages;
	eFixedMessagePump<int> message_notifier;

	eTimer timeout, clocktimer, messagetimeout, progresstimer, volumeTimer;

	int cur_start, cur_duration, cur_event_id;
	eString cur_event_text;
	
	eNVODSelector nvodsel;
	eSubServiceSelector subservicesel;
	eAudioSelector audiosel;
	eEventDisplay *actual_eventDisplay;
	eServiceReferenceDVB refservice;
	
	ePlaylist *curlist;		// history / current playlist entries
	eServiceReference playlistref;

	ePlaylist *favourite[modeFile+1];
	eServiceReference favouriteref[modeFile+1];

	ePlaylist *recordings;
	eServiceReference recordingsref;

	int playlistmode; // curlist is a list controlled by the user (not just a history).
	int entered_playlistmode;
	
	int flags;
	int isVT;
	int isEPG;
	int showOSDOnEITUpdate;
	int serviceFlags;
	int isSeekable() const { return serviceFlags & eServiceHandler::flagIsSeekable; }
	eZapLCD lcdmain;
	
	void redrawWidget(gPainter *, const eRect &where);
	void eraseBackground(gPainter *, const eRect &where);
	void setEIT(EIT *);
	void handleNVODService(SDTEntry *sdtentry);
	
	// actions
	void showServiceSelector(int dir, int selcurrent);
	void nextService(int add=0);
	void prevService();
	void playlistNextService();
	void playlistPrevService();
	void volumeUp();
	void volumeDown();
	void hideVolumeSlider();
	void toggleMute();
	void showMainMenu();
	
	time_t standbyTime;

	void standbyPress();
	void standbyRepeat();
	void standbyRelease();
	void showSubserviceMenu();
	void showAudioMenu();
	void runVTXT();
	void showEPGList();
	void showEPG();
	void showInfobar();
	void hideInfobar();
	
	void play();
	void stop();
	void pause();
	enum { skipForward, skipReverse };
	int skipcounter;
	int skipping;
	void startSkip(int dir);
	void repeatSkip(int dir);
	void stopSkip(int dir);
	
	void showServiceMenu(eServiceSelector*);
	void showFavourite(eServiceSelector*);
	
	void addService(const eServiceReference &service);
	
	void doPlaylistAdd(const eServiceReference &service);
	void addServiceToFavourite(eServiceSelector *s);
	
	void showFavourites();
	void showBouquetList(int sellast);

	void showDVRFunctions(int show);

	static eZapMain *instance;
	
	eServicePath modeLast[modeEnd];
	int mode, last_mode, state, oldstate;
protected:
	int eventHandler(const eWidgetEvent &event);
private:
	void handleServiceEvent(const eServiceEvent &event);
	void startService(const eServiceReference &, int);
	void gotEIT();
	void gotSDT();
	void gotPMT();
	void timeOut();
	void leaveService();
	void clockUpdate();
	void updateVolume(int vol);
	void set16_9Logo(int aspect);
	void setSmartcardLogo(bool b);
	void setAC3Logo(bool b);
	void setVTButton(bool b);
	void setEPGButton(bool b);
	void updateProgress();
	void getPlaylistPosition();
	void setPlaylistPosition();
	bool handleState(int notimer);
public:
	void postMessage(const eZapMessage &message, int clear=0);
	void gotMessage(const int &);
	void startMessages();
	void stopMessages();
	void pauseMessages();
	void nextMessage();
	static eZapMain *getInstance() { return instance; }

// methods used from the timer
	enum {
		psAdd=1,	// just add, to not change current
		psRemove=2, // remove before add
		psActivate=4, // use existing entry if available
		psDontAdd=8, // just play
	};
	void playService(const eServiceReference &service, int flags);
	int recordDVR(int onoff, int user, eString name="");	// starts recording
//////////////////////////////

	void setMode(int mode, int user=0); // user made change?
	void setModeD(int mode);
	int getRealMode() { return last_mode==-1 ? mode : last_mode; }
	int getState() { return state; }
	void setState( int newState ) { state = newState; }
	
	void setServiceSelectorPath(eServicePath path);
	void getServiceSelectorPath(eServicePath &path);

	void moveService(const eServiceReference &path, const eServiceReference &ref, const eServiceReference &after);

	eZapMain();
	~eZapMain();
};

class eServiceContextMenu: public eListBoxWindow<eListBoxEntryText>
{
	eServiceReference ref;
	void entrySelected(eListBoxEntryText *s);
public:
	eServiceContextMenu(const eServiceReference &ref, const eServiceReference &path);
};

class eRecordingStatus: public eDecoWidget
{
public:
	eRecordingStatus();
};

#endif /* __enigma_main_h */

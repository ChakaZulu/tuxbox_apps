#ifndef __enigma_main_h
#define __enigma_main_h

#include <apps/enigma/enigma_lcd.h>
#include <core/dvb/si.h>
#include <core/dvb/dvb.h>
#include <core/dvb/edvb.h>
#include <core/gui/ewindow.h>
#include <core/gui/listbox.h>
#include <core/gui/multipage.h>
#include <core/gui/emessage.h>
#include <core/base/message.h>

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

	const eString & getCaption() { return caption; }
	const eString & getBody() { return body; }
	void setTimeout(int _timeout)
	{
		timeout=_timeout;
	}
	int getTimeout()
	{
		return timeout;
	}
	bool isSameType(const eZapMessage &msg) const
	{
		return type == msg.type;
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

class eZapMain: public eWidget
{
	eLabel 	*ChannelNumber, *ChannelName, *Clock, *EINow, *EINext,
		*EINowDuration, *EINextDuration, *EINowTime, *EINextTime,
		*Description,
		*ButtonRedEn, *ButtonRedDis, 
		*ButtonGreenEn, *ButtonGreenDis, 
		*ButtonYellowEn, *ButtonYellowDis,
		*ButtonBlueEn, *ButtonBlueDis;
	
	eLabel *DolbyOn, *DolbyOff, *CryptOn, *CryptOff, *WideOn, *WideOff;
	
	eProgress *Progress, *VolumeBar;
	eMessageBox *pMsg;

	eLock messagelock;
	std::list<eZapMessage> messages;
	eFixedMessagePump<int> message_notifier;

	eTimer timeout, clocktimer, messagetimeout;

	int cur_start, cur_duration;
	
	eNVODSelector nvodsel;
	eSubServiceSelector subservicesel;
	eAudioSelector audiosel;
	eEventDisplay *actual_eventDisplay;
	eServiceReferenceDVB refservice;
	int flags;
	int isVT;
	int isEPG;
	int showOSDOnEITUpdate;
	eZapLCD lcdmain;
	
	void redrawWidget(gPainter *, const eRect &where);
	void eraseBackground(gPainter *, const eRect &where);
	void setEIT(EIT *);
	void handleNVODService(SDTEntry *sdtentry);
	
	// actions
	void showServiceSelector(int dir);
	void nextService();
	void prevService();
	void volumeUp();
	void volumeDown();
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
public:
	void postMessage(const eZapMessage &message, int clear=0);
	void gotMessage(const int &);
	void startMessages();
	void stopMessages();
	void pauseMessages();
	void nextMessage();

	eZapMain();
	~eZapMain();
};

#endif /* __enigma_main_h */

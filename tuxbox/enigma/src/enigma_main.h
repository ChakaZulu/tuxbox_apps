#ifndef __enigma_main_h
#define __enigma_main_h

#include <apps/enigma/enigma_lcd.h>
#include <core/dvb/si.h>
#include <core/dvb/dvb.h>
#include <core/dvb/edvb.h>
#include <core/gui/ewindow.h>
#include <core/gui/listbox.h>
#include <core/gui/multipage.h>

class eLabel;
class eProgress;
class EIT;
class SDT;
class SDTEntry;
class PMT;
class PMTEntry;
class eNumber;
class gPainter;

class NVODStream: public eListBoxEntryTextStream
{
	friend class eListBox<NVODStream>;
	friend class eNVODSelector;
private:
	void EITready(int error);
public:
	NVODStream(eListBox<NVODStream> *listbox, int transport_stream_id, int original_network_id, int service_id, int type);
	int transport_stream_id, original_network_id, service_id;
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

class AudioStream: public eListBoxEntry
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

protected:
	void redraw(gPainter *rc, const eRect& rect, const gColor& coActive, const gColor& coNormal, bool highlited) const;
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
	int transport_stream_id, original_network_id, service_id;
public:
	SubService(eListBox<SubService> *listbox, LinkageDescriptor *descr);
};

class eSubServiceSelector: public eListBoxWindow<SubService>
{
	void selected(SubService *);
public:
	eSubServiceSelector();
	void clear();
	void add(LinkageDescriptor *ref);
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

	eTimer timeout, clocktimer;

	int cur_start, cur_duration;
	
	eNVODSelector nvodsel;
	eSubServiceSelector subservicesel;
	eAudioSelector audiosel;
	eEventDisplay *actual_eventDisplay;
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
	void standby();
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
	void serviceChanged(eService *, int);
	void gotEIT(EIT *, int);
	void gotSDT(SDT *);
	void gotPMT(PMT *);
	void timeOut();
	void leaveService(eService *);
	void clockUpdate();
	void updateVolume(int vol);
	void set16_9Logo(int aspect);
	void setSmartcardLogo(bool b);
	void setAC3Logo(bool b);
	void setVTButton(bool b);
	void setEPGButton(bool b);
public:
	eZapMain();
	~eZapMain();
};

#endif /* __enigma_main_h */

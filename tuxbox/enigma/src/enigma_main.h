#ifndef __enigma_main_h
#define __enigma_main_h

#include "elistbox.h"
#include "ewindow.h"
#include "si.h"
#include "dvb.h"
#include "enigma_lcd.h"
#include "multipage.h"

class eListbox;
class eLabel;
class eListboxEntry;
class eProgress;
class EIT;
class SDT;
class SDTEntry;
class PMT;
class PMTEntry;
class eNumber;
class gPainter;

class NVODStream: public eListboxEntry
{
private:
	void EITready(int error);
public:
	NVODStream(eListbox *listbox, int transport_stream_id, int original_network_id, int service_id);
	eString getText(int col=0) const;
	int transport_stream_id, original_network_id, service_id;
	EIT eit;
};

class NVODReferenceEntry;

class eNVODSelector: public eWindow
{
	eListbox *list;
private:
	void selected(eListboxEntry *);
public:
	eNVODSelector();
	void clear();
	void add(NVODReferenceEntry *ref);
};

class AudioStream: public eListboxEntry
{
public:
	enum
	{
		audioMPEG, audioAC3
	};
	AudioStream(eListbox *listbox, PMTEntry *stream);
	eString getText(int col=0) const;
	PMTEntry *stream;
};

class eAudioSelector: public eWindow
{
	eListbox *list;
private:
	void selected(eListboxEntry *);
public:
	eAudioSelector();
	void clear();
	void add(PMTEntry *);
};

class LinkageDescriptor;

class SubService: public eListboxEntry
{
public:
	SubService(eListbox *listbox, LinkageDescriptor *descr);
	eString getText(int col=0) const;

	int transport_stream_id, original_network_id, service_id;
	eString name;
};

class eSubServiceSelector: public eWindow
{
	eListbox *list;
private:
	void selected(eListboxEntry *);
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
#define ENIGMA_SUBSERVICES	4

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
	eZapLCD lcdmain;
	
	void redrawWidget(gPainter *, const eRect &where);
	void eraseBackground(gPainter *, const eRect &where);
	void setEIT(EIT *);
	void handleNVODService(SDTEntry *sdtentry);
protected:
	void keyDown(int);
	void keyUp(int);
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

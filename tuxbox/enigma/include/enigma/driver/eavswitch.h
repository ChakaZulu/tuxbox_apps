#ifndef __eavswitch_h
#define __eavswitch_h

enum eAVAspectRatio
{
	rUnknown, r43, r169
};

enum eAVColorFormat
{
	cfNull,
	cfCVBS, 	// "FBAS"
	cfRGB,
	cfYC			// "SVideo"
};

class eAVSwitch
{
	static eAVSwitch *instance;
public:
	static eAVSwitch *getInstance();
	eAVSwitch();
	virtual int setVolume(int vol)=0;	// 0..65535
	virtual int setColorFormat(eAVColorFormat cf)=0;
	virtual int setAspectRatio(eAVAspectRatio as)=0;
	virtual int isVCRActive()=0;
	virtual int setActive(int active)=0;
	virtual int setInput(int v)=0;	// 0: dbox, 1: vcr
	virtual ~eAVSwitch();
	
	void reloadSettings();
};

class eAVSwitchNokia: public eAVSwitch
{
	int fd, saafd;
	int setTVPin8(int vol);
	
	int active;
	eAVAspectRatio aspect;
	eAVColorFormat colorformat;
public:
	eAVSwitchNokia();
	~eAVSwitchNokia();

	int setVolume(int vol);	// 0..65535
	int setColorFormat(eAVColorFormat cf);
	int setAspectRatio(eAVAspectRatio as);
	int isVCRActive();
	int setActive(int active); 
	int setInput(int v);	// 0: dbox, 1: vcr
};

class eAVSwitchPhilips: public eAVSwitch
{
	int fd, saafd;
	int setTVPin8(int vol);
	
	int active;
	eAVAspectRatio aspect;
public:
	eAVSwitchPhilips();
	~eAVSwitchPhilips();

	int setVolume(int vol);	// 0..65535
	int setColorFormat(eAVColorFormat cf);
	int setAspectRatio(eAVAspectRatio as);
	int isVCRActive();
	int setActive(int active); 
	int setInput(int v);	// 0: dbox, 1: vcr
};

class eAVSwitchSagem: public eAVSwitch
{
	int fd, saafd;
	int setTVPin8(int vol);
	
	int active;
	eAVAspectRatio aspect;
public:
	eAVSwitchSagem();
	~eAVSwitchSagem();

	int setVolume(int vol);	// 0..65535
	int setColorFormat(eAVColorFormat cf);
	int setAspectRatio(eAVAspectRatio as);
	int isVCRActive();
	int setActive(int active); 
	int setInput(int v);	// 0: dbox, 1: vcr
};

#endif

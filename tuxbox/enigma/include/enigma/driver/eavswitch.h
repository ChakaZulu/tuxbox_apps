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
	enum {NOKIA, SAGEM, PHILIPS} Type;
	int scart[6];
	int dvb[6];
	int active;
	int fd, saafd;
	eAVAspectRatio aspect;
	eAVColorFormat colorformat;
	static eAVSwitch *getInstance();
	eAVSwitch();
	virtual int setTVPin8(int vol);
	virtual int setVolume(int vol);	// 0..65535
	virtual int setColorFormat(eAVColorFormat cf);
	virtual int setAspectRatio(eAVAspectRatio as);
	virtual int setActive(int active);
	virtual int isVCRActive();
	virtual int setInput(int v);	// 0: dbox, 1: vcr
	virtual ~eAVSwitch();
	void reloadSettings();
	bool loadScartConfig();
};

class eAVSwitchNokia: public eAVSwitch
{
public:
	eAVSwitchNokia()
	{
		Type = NOKIA;
		scart[0] = 3;
		scart[1] = 2;
		scart[2] = 1;
		scart[3] = 0;
		scart[4] = 1;
		scart[5] = 1;
		dvb[0] = 5;
		dvb[1] = 1;
		dvb[2] = 1;
		dvb[3] = 0;
		dvb[4] = 1;
		dvb[5] = 1;
		loadScartConfig();
	}
};

class eAVSwitchPhilips: public eAVSwitch
{
public:
	eAVSwitchPhilips()
	{
		Type = PHILIPS;
		scart[0] = 2;
		scart[1] = 2;
		scart[2] = 3;
		scart[3] = 0;
		scart[4] = 3;
		scart[5] = 0;
		dvb[0] = 5;
		dvb[1] = 1;
		dvb[2] = 1;
		dvb[3] = 0;
		dvb[4] = 1;
		dvb[5] = 0;
		loadScartConfig();
	}
};

class eAVSwitchSagem: public eAVSwitch
{
public:
	eAVSwitchSagem()
	{
		Type = SAGEM;
		scart[0] = 2;
		scart[1] = 1;
		scart[2] = 0;
		scart[3] = 0;
		scart[4] = 0;
		scart[5] = 0;
		dvb[0] = 0;
		dvb[1] = 0;
		dvb[2] = 0;
		dvb[3] = 0;
		dvb[4] = 0;
		dvb[5] = 0;
		loadScartConfig();
	}
};

#endif

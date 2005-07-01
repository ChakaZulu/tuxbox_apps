#ifndef __enigma_dyn_h
#define __enigma_dyn_h

#if ENABLE_DYN_MOUNT && ENABLE_DYN_CONF && ENABLE_DYN_FLASH && ENABLE_DYN_ROTOR
#define WEBIFVERSION "4.1.4.Expert"
#else
#define WEBIFVERSION "4.1.4"
#endif

#define ZAPMODETV 0
#define ZAPMODERADIO 1
#define ZAPMODEDATA 2
#define ZAPMODERECORDINGS 3
#define ZAPMODEROOT 4

#define ZAPSUBMODENAME 0
#define ZAPSUBMODECATEGORY 1
#define ZAPSUBMODESATELLITES 2
#define ZAPSUBMODEPROVIDERS 3
#define ZAPSUBMODEBOUQUETS 4

class eHTTPDynPathResolver;

void ezapInitializeDyn(eHTTPDynPathResolver *dyn_resolver);

class myTimerEntry
{
public:
	int start;
	eString timerData;
	myTimerEntry(int pStart, eString pTimerData)
	{
		start = pStart;
		timerData = pTimerData;
	};
	~myTimerEntry() {};
	bool operator < (const myTimerEntry &a) const {return start < a.start;}
};

#endif /* __enigma_dyn_h */

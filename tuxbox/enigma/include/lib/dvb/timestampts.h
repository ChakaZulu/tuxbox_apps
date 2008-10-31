#ifndef __include_lib_dvb_timestampts_h
#define __include_lib_dvb_timestampts_h

#include <map>
#include <set>

#include <time.h>
#include <lib/dvb/service.h>

class eTimeStampParserTS
{
	typedef unsigned long long Timestamp;
	unsigned char pkt[188];
	int pktptr;
	int processPacket(const unsigned char *pkt);
	inline int wantPacket(const unsigned char *hdr) const;
	int pid;
	int needNextPacket;
	int skip;
	tm movie_begin;
	tm movie_end;	
	tm movie_current;
	int MovieCurrentTime;
	int MovieBeginTime;
	int MovieEndTime;
	int MovieDuration;
	eString currentTime;
	eString beginTime;
	eString endTime;
	eString durationTime;
	eString remainTime;
	int type;
public:
	eTimeStampParserTS(eString _filename);
	void parseData(const void *data, unsigned int len);
	eString getCurrentTime() {return currentTime;}
	eString getBeginTime() {return beginTime;}
	eString getEndTime() {return endTime;}
	eString getDurationTime() {return durationTime;}
	eString getRemainTime() {return remainTime;}
};

#endif

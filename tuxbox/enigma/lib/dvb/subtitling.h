#ifndef __subtitling_h
#define __subtitling_h

#include <lib/gui/ewidget.h>
#include <lib/base/ebase.h>
#include <lib/dvb/subtitle.h>
#include <queue>

class eSubtitleWidget: public eWidget
{
	std::set<int> pageids;
	void gotData(int);
	eSocketNotifier *sn;
	int fd;
	
	subtitle_ctx *subtitle; // the subtitle context
	
	struct pes_packet_s
	{
		unsigned long long pts;
		unsigned char *pkt;
		int len;
	};
	
	std::queue<pes_packet_s> queue;
	
	eTimer timer;
	void processPESPacket(unsigned char *pkt, int len);
	void processNext();
	
	unsigned char pesbuffer[65536];
	int pos;
	int peslen;
public:
	void start(int pid, const std::set<int> &pageids);
	void stop();
	eSubtitleWidget();
	~eSubtitleWidget();
};

#endif

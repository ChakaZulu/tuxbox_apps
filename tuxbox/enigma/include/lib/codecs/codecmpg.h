#ifndef __lib_codecs_codecmpg_h
#define __lib_codecs_codecmpg_h


#include <lib/base/buffer.h>
#include <lib/codecs/codec.h>

// mpeg-2 ps demuxer.
class eMPEGDemux: public eAudioDecoder
{
	eIOBuffer &input, &video, &audio;
	unsigned long last, remaining;
	unsigned long getLong();
	void refill();
	unsigned long getBits(unsigned int num);
	void syncBits();
public:
	eMPEGDemux(eIOBuffer &input, eIOBuffer &video, eIOBuffer &audio);
	virtual int decodeMore(int last, int maxsamples); // returns number of samples(!) written to IOBuffer (out)
	virtual void resync(); // clear (secondary) decoder buffers
	virtual int getMinimumFramelength();
	virtual int getAverageBitrate();
};

#endif

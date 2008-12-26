#ifndef DISABLE_FILE

#ifndef __lib_codecs_codecogg_h
#define __lib_codecs_codecogg_h

#include <tremor/ivorbiscodec.h>
#include <tremor/ivorbisfile.h>
#include <lib/codecs/codec.h>

struct eOggFileInfo
{
	eIOBuffer* input;
	int sourcefd;
};

class eAudioDecoderOgg: public eAudioDecoder
{
	enum { INPUT_BUFFER_SIZE=8192 };
	unsigned char input_buffer[INPUT_BUFFER_SIZE];
	int avgbr; // average bitrate
	OggVorbis_File vf;
	eIOBuffer &input, &output;
	bool first;
	eOggFileInfo fileinfo;
	int sourcefd;
public:
	eAudioDecoderOgg(eIOBuffer &input, eIOBuffer &output, const char *filename, int sourcefd);
	~eAudioDecoderOgg();
	
	void resync();
	int getMinimumFramelength();
	int decodeMore(int last, int maxsamples, Signal1<void, unsigned int>*cb=0);
	int getAverageBitrate();
	void Init();
};

#endif

#endif //DISABLE_FILE

#include <lib/codecs/codec.h>

eAudioDecoder::eAudioDecoder(eIOBuffer &input, eIOBuffer &output): input(input), output(output), speed(1) 
{ 
}

eAudioDecoder::~eAudioDecoder()
{
}

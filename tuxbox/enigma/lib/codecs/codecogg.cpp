#ifndef DISABLE_FILE

#include <lib/codecs/codecogg.h>
#include <lib/base/eerror.h>
#include <lib/base/buffer.h>
#include <linux/soundcard.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>


size_t ogg_read(void *buf, size_t size, size_t nmemb, void *data)
{
	if (((eOggFileInfo *)data)->sourcefd < 0) // not seekable (we are streaming)
	{
//eDebug ("[OGG]reading:%d/%d,%d",size,nmemb,((eOggFileInfo *)data)->input->size());
		return ((eOggFileInfo *)data)->input->read(buf, size*nmemb);
	}
	else
	{
		((eOggFileInfo *)data)->input->read(buf, size*nmemb);
		return ::read(((eOggFileInfo *)data)->sourcefd , buf, size*nmemb);
	}
}

int ogg_seek(void *data, ogg_int64_t offset, int whence)
{
//eDebug ("[OGG]seeking:%d/%d",offset,whence);
	if (((eOggFileInfo *)data)->sourcefd < 0) // not seekable (we are streaming)
		return -1;
	return ::lseek64(((eOggFileInfo *)data)->sourcefd,offset,whence);
}

int ogg_close(void *data)
{
//eDebug ("[OGG]close");
	if (((eOggFileInfo *)data)->sourcefd >= 0)
		::close(((eOggFileInfo *)data)->sourcefd);
	((eOggFileInfo *)data)->sourcefd = -1;
	return 0;
}

long ogg_tell(void *data)
{
	if (((eOggFileInfo *)data)->sourcefd < 0) // not seekable (we are streaming)
		return -1;
//eDebug ("[OGG]tell");
	return ::lseek64(((eOggFileInfo *)data)->sourcefd,0,SEEK_CUR);
}

eAudioDecoderOgg::eAudioDecoderOgg(eIOBuffer &input, eIOBuffer &output, const char *filename, int sourcefd):
		avgbr(-1), input(input), output(output),first(true),sourcefd(sourcefd)
{
	fileinfo.input = &input;
	fileinfo.sourcefd = filename ? ::open(filename, O_RDONLY|O_LARGEFILE) : -1;
}

eAudioDecoderOgg::~eAudioDecoderOgg()
{
	ov_clear(&vf);
}

int eAudioDecoderOgg::decodeMore(int last, int maxsamples, Signal1<void, unsigned int> *)
{
//eDebug ("[OGG]decodeMore:%d,%d,%d",last,maxsamples,input.size());
	int written = 0;
	int bitstream;
	while (last || (written < maxsamples))
	{
		if (!input.size())
			break;
		if (first) // fill ogg structures
			Init();
		const int OUTPUT_BUFFER_SIZE = 4096;
		char outbuffer[OUTPUT_BUFFER_SIZE];
		long ret=ov_read(&vf,outbuffer,sizeof(outbuffer),&bitstream);
		if (ret <= 0)
			break;
		output.write(outbuffer, ret);
		written+=ret;
	}
	return written;
}

int eAudioDecoderOgg::getMinimumFramelength()
{
	return 4096;
}
void eAudioDecoderOgg::resync()
{
//eDebug ("[OGG]resync");
	if (fileinfo.sourcefd >= 0)
		ov_raw_seek(&vf,::lseek64(sourcefd,0,SEEK_CUR));
}

int eAudioDecoderOgg::getAverageBitrate()
{
	return avgbr;
}
void eAudioDecoderOgg::Init()
{ 
	ov_callbacks cb;
	cb.read_func  = ogg_read;
	cb.seek_func  = ogg_seek;
	cb.close_func = ogg_close;
	cb.tell_func  = ogg_tell;

//eDebug ("[OGG]opening");
	if (ov_open_callbacks(&fileinfo, &vf, NULL, 0, cb) >= 0)
	{
		vorbis_info *vi=ov_info(&vf,-1);
		pcmsettings.samplerate=vi->rate;
		pcmsettings.channels=vi->channels;
		pcmsettings.format=AFMT_S16_BE;
		pcmsettings.reconfigure=0;
		if (vi->bitrate_nominal  > 0)
			avgbr = vi->bitrate_nominal;
//eDebug ("[OGG]opened:%d",avgbr);
	}
	first = false;
//eDebug ("[OGG]done opening");
}
#endif //DISABLE_FILE

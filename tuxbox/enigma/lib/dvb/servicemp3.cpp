#include <core/dvb/servicemp3.h>
#include <core/dvb/servicefile.h>
#include <core/system/init.h>
#include <core/base/i18n.h>

#include <unistd.h>
#include <fcntl.h>
#include <id3tag.h>
#include <linux/soundcard.h>
#include <sys/ioctl.h>

/*
	note: mp3 decoding is done in ONE seperate thread with multiplexed input/
	decoding and output. The only problem arises when the ::read-call,
	encapsulated in eIOBuffer::fromfile, blocks. althought we only call
	it when data is "ready" to read (according to ::poll), this doesn't help
	since linux/posix/unix/whatever doesn't support proper read-ahead with
	::poll-notification. bad luck for us.
	
	the only way to address this problem (except using ::aio_*) is to
	use another thread. i don't like threads so if you really have a slow
	network/harddisk, it's your problem. sorry.
	
	another problem is too slow decoding, but we only decode one frame
	per loop, so if t(DECODE_ONE_FRAME) > t(DMA_BUFFER_EMPTY) we're out
	of luck. shouldn't happen.
*/

eMP3Decoder::eMP3Decoder(const char *filename): input(32*1024), output(64*1024), messages(this)
{
	state=stateInit;

	mad_stream_init(&stream);
	mad_frame_init(&frame);
	mad_synth_init(&synth);
	mad_timer_reset(&timer);

	sourcefd=::open(filename, O_RDONLY);
	if (sourcefd<0)
	{
		eDebug("error opening %s", filename);
		state=stateError;
	}
	
	pcmsettings.reconfigure=1;
	
	dspfd=::open("/dev/sound/dsp", O_WRONLY|O_NONBLOCK);
	if (dspfd<0)
	{
		eDebug("output failed! (%m)");
		state=stateError;
	}
	
	outputsn=new eSocketNotifier(this, dspfd, eSocketNotifier::Write, 0);
	CONNECT(outputsn->activated, eMP3Decoder::outputReady);
	inputsn=new eSocketNotifier(this, sourcefd, eSocketNotifier::Read, 0);
	CONNECT(inputsn->activated, eMP3Decoder::decodeMore);
	
	CONNECT(messages.recv_msg, eMP3Decoder::gotMessage);
	
	maxOutputBufferSize=256*1024;

	if (state != stateError)
		run();
}

void eMP3Decoder::thread()
{
	messages.start();
	exec();
}

void eMP3Decoder::outputReady(int what)
{
	if ( ( pcmsettings.reconfigure 
			|| (pcmsettings.samplerate != synth.pcm.samplerate) 
			|| (pcmsettings.channels != synth.pcm.channels)))
	{
		pcmsettings.samplerate=synth.pcm.samplerate;
		pcmsettings.channels=synth.pcm.channels;
		pcmsettings.reconfigure=0;
		pcmsettings.format=AFMT_S16_BE;
		::ioctl(dspfd, SNDCTL_DSP_SPEED, &pcmsettings.samplerate);
		::ioctl(dspfd, SNDCTL_DSP_CHANNELS, &pcmsettings.channels);
		::ioctl(dspfd, SNDCTL_DSP_SETFMT, &pcmsettings.format);
		eDebug("reconfigured audio interface...");
	}

	output.tofile(dspfd, 65536);
	if ((state == stateBufferFull) && (output.size()<maxOutputBufferSize))
	{
		state=statePlaying;
		inputsn->start();
	}
	if (output.empty())
	{
		outputsn->stop();
		state=stateBuffering;
	}
}

static inline unsigned short MadFixedToUshort(mad_fixed_t Fixed)
{
		// CLIPPING !!!
	Fixed>>=MAD_F_FRACBITS-14;
	return Fixed;
}

void eMP3Decoder::decodeMore(int what)
{
	if ((state != statePlaying) && (state != stateBuffering))
	{
		eDebug("wrong state (%d)", state);
		return;
	}
	
	int flushbuffer=0;
	
	if (input.size() < INPUT_BUFFER_SIZE)
	{
		if (input.fromfile(sourcefd, INPUT_BUFFER_SIZE) < INPUT_BUFFER_SIZE)
			flushbuffer=1;
	}
	
	while (1) // input.size() > INPUT_BUFFER_SIZE)
	{
		const int OUTPUT_BUFFER_SIZE=1152*2;
		int status=0;
	
		if ((!stream.buffer) || (stream.error == MAD_ERROR_BUFLEN))
		{
			size_t read_size, remaining;
			unsigned char *read_start;
	
			if (stream.next_frame)
			{
				remaining=stream.bufend-stream.next_frame;
				memmove(input_buffer, stream.next_frame, remaining);
				read_start=input_buffer+remaining;
				read_size=INPUT_BUFFER_SIZE-remaining;
			} else
				read_size=INPUT_BUFFER_SIZE,
				read_start=input_buffer,
				remaining=0;
		
				read_size=input.read(read_start, read_size);
		
			mad_stream_buffer(&stream, input_buffer, read_size+remaining);
			stream.error=(mad_error)0;
		}

		if (mad_frame_decode(&frame, &stream))
			if (MAD_RECOVERABLE(stream.error))
			{
				eWarning("mp3: recoverable frame level error (%s)", mad_stream_errorstr(&stream));
				continue;
			} else
				if (stream.error == MAD_ERROR_BUFLEN)
				{
//					eDebug("MAD_ERROR_BUFLEN");
					continue;
				} else
				{
					eWarning("mp3: unrecoverable frame level error (%s)", mad_stream_errorstr(&stream));
					status=2;
					break;
				}
		
		mad_timer_add(&timer, frame.header.duration);

		mad_synth_frame(&synth, &frame);
		unsigned short outbuffer[OUTPUT_BUFFER_SIZE];
		int ptr=0, len=synth.pcm.length;
		int stereo=MAD_NCHANNELS(&frame.header)==2;
		
		/* endianess kaputt !!! */
		while (len)
		{
			int tw=len;
			if (!stereo)
			{
				if (tw > OUTPUT_BUFFER_SIZE)
					tw=OUTPUT_BUFFER_SIZE;
			} else
			{
				if (tw > OUTPUT_BUFFER_SIZE/2)
					tw=OUTPUT_BUFFER_SIZE/2;
			}
			if (stereo)
				for (int i=0; i<tw; i++)
				{
					outbuffer[i*2]=MadFixedToUshort(synth.pcm.samples[0][ptr]);
					outbuffer[i*2+1]=MadFixedToUshort(synth.pcm.samples[1][ptr++]);
				}
			else
				for (int i=0; i<tw; i++)
					outbuffer[i]=MadFixedToUshort(synth.pcm.samples[0][ptr++]);

			output.write(outbuffer, tw*(stereo?4:2));
			len-=tw;
		}
		if (status)
		{
			state=stateFileEnd;
			eDebug("datei TOTAL kaputt");
		}
//		if (output.size() > maxOutputBufferSize)
		if (!flushbuffer)
			break;
	}

	if ((state == stateBuffering) && (output.size()>16384))
	{
		state=statePlaying;
		outputsn->start();
	}
	
	if (flushbuffer)
		eDebug("end of file...");
	
	if ((state == statePlaying) && (output.size() > maxOutputBufferSize))
	{
		state=stateBufferFull;
		inputsn->stop();
	}
}

eMP3Decoder::~eMP3Decoder()
{
	kill(); // wait for thread exit.

	delete inputsn;
	delete outputsn;
	mad_synth_finish(&synth);
	mad_frame_finish(&frame);
	mad_stream_finish(&stream);
	if (dspfd >= 0)
		close(dspfd);
	if (sourcefd >= 0)
		close(sourcefd);
}

void eMP3Decoder::gotMessage(const eMP3DecoderMessage &message)
{
	switch (message.type)
	{
	case eMP3DecoderMessage::start:
		if (state == stateInit)
		{
			state=stateBuffering;
			inputsn->start();
		}
		break;
	case eMP3DecoderMessage::exit:
		eDebug("got quit message..");
		quit();
		break;
	}
}

eService *eServiceHandlerMP3::createService(const eServiceReference &service)
{
	id3_file *file;
	
	file=::id3_file_open(service.path.c_str(), ID3_FILE_MODE_READONLY);
	if (!file)
		return new eService(eServiceID(0), (eString("MP3: ") + service.path).c_str());
		
	id3_tag *tag=id3_file_tag(file);
	if (!tag)
	{
		id3_file_close(file);
		return new eService(eServiceID(0), (eString("MP3: ") + service.path).c_str());
	}

	eString description="";

  struct id3_frame const *frame;
  id3_ucs4_t const *ucs4;
  id3_latin1_t *latin1;

	struct
	{
		char const *id;
		char c;
	} const info[] = {
		{ ID3_FRAME_TITLE,  '2'},
		{ "TIT3",           's'}, 
		{ "TCOP",           'd'},
		{ "TPRO",           'p'},
		{ "TCOM",           'b'},
		{ ID3_FRAME_ARTIST, '1'},
		{ "TPE2",           'f'},
		{ "TPE3",           'c'},
 		{ "TEXT",           'l'},
		{ ID3_FRAME_ALBUM,  '3'},
		{ ID3_FRAME_YEAR,   '4'},
		{ ID3_FRAME_TRACK,  'a'},
		{ "TPUB",           'P'},
		{ ID3_FRAME_GENRE,  '6'},
 		{ "TRSN",           'S'},
		{ "TENC",           'e'}
	};
	
	const char *naming="[%1 - %3] %2";
	
	for (const char *c=naming; *c; ++c)
	{
		if ((*c != '%') || (*++c=='%') || !*c)
		{
			description+=*c;
			continue;
		}
		
		unsigned int i;
		
		for (i=0; i<sizeof(info)/sizeof(*info); ++i)
			if (info[i].c == *c)
				break;
		if (i == sizeof(info)/sizeof(*info))
			continue;

		union id3_field const *field;
		unsigned int nstrings, j;

		frame = id3_tag_findframe(tag, info[i].id, 0);
		if (frame == 0)
			continue;

		field    = &frame->fields[1];
		nstrings = id3_field_getnstrings(field);
	
		for (j = 0; j < nstrings; ++j) 
		{
			ucs4 = id3_field_getstrings(field, j);
			assert(ucs4);

			if (strcmp(info[i].id, ID3_FRAME_GENRE) == 0)
				ucs4 = id3_genre_name(ucs4);

			latin1 = id3_ucs4_latin1duplicate(ucs4);
			if (latin1 == 0)
				break;

			description+=eString((const char*)latin1);
			free(latin1);
		}
	}
	
	id3_file_close(file);

	return new eService(eServiceID(0), description.c_str());
}

int eServiceHandlerMP3::play(const eServiceReference &service)
{
	state=statePlaying;
	
	decoder=new eMP3Decoder(service.path.c_str());
	decoder->messages.send(eMP3Decoder::eMP3DecoderMessage(eMP3Decoder::eMP3DecoderMessage::start));
	
	serviceEvent(eServiceEvent(eServiceEvent::evtStart));

	return 0;
}

eServiceHandlerMP3::eServiceHandlerMP3(): eServiceHandler(0x1000)
{
	if (eServiceInterface::getInstance()->registerHandler(id, this)<0)
		eFatal("couldn't register serviceHandler %d", id);
	CONNECT(eServiceFileHandler::getInstance()->fileHandlers, eServiceHandlerMP3::addFile);
}

eServiceHandlerMP3::~eServiceHandlerMP3()
{
	eServiceInterface::getInstance()->unregisterHandler(id);
}

void eServiceHandlerMP3::addFile(void *node, const eString &filename)
{
	if (filename.right(4).upper()==".MP3")
		eServiceFileHandler::getInstance()->addReference(node, eServiceReference(id, 0, filename));
}

eService *eServiceHandlerMP3::lookupService(const eServiceReference &service)
{
	return eServiceFileHandler::getInstance()->lookupService(service);
}

int eServiceHandlerMP3::getFlags()
{
	return 0;
}

int eServiceHandlerMP3::getState()
{
	return state;
}

int eServiceHandlerMP3::getErrorInfo()
{
	return 0;
}

#if 0
void eServiceHandlerMP3::internalstop()
{
	if (state == statePlaying)
	{
		eDebug("MP3: stop");
//		serviceEvent(eServiceEvent(eServiceEvent::evtEnd));
	
		state=stateStopped;
	}
}
#endif

int eServiceHandlerMP3::stop()
{
	decoder->messages.send(eMP3Decoder::eMP3DecoderMessage(eMP3Decoder::eMP3DecoderMessage::exit));
	delete decoder;
	serviceEvent(eServiceEvent(eServiceEvent::evtStop));
	return 0;
}

eAutoInitP0<eServiceHandlerMP3> i_eServiceHandlerMP3(7, "eServiceHandlerMP3");

#include <lib/dvb/servicemp3.h>
#include <lib/dvb/servicefile.h>
#include <lib/system/init.h>
#include <lib/base/i18n.h>

#include <unistd.h>
#include <fcntl.h>
#include <id3tag.h>
#include <linux/soundcard.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>

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

eMP3Decoder::eMP3Decoder(const char *filename, eServiceHandlerMP3 *handler): handler(handler), input(8*1024), output(256*1024), messages(this, 1)
{
	state=stateInit;

	mad_stream_init(&stream);
	mad_frame_init(&frame);
	mad_synth_init(&synth);
	mad_timer_reset(&timer);

	sourcefd=::open(filename, O_RDONLY);
	if (sourcefd<0)
	{
		error=errno;
		eDebug("error opening %s", filename);
		state=stateError;
	} else
	{
		filelength=::lseek(sourcefd, 0, SEEK_END);
		lseek(sourcefd, 0, SEEK_SET);
	}
	
	length=-1;
	avgbr=-1;

	pcmsettings.reconfigure=1;
	
	dspfd=::open("/dev/sound/dsp", O_WRONLY|O_NONBLOCK);
	if (dspfd<0)
	{
		eDebug("output failed! (%m)");
		error=errno;
		state=stateError;
	}
	
	if (dspfd >= 0)
	{
		outputsn=new eSocketNotifier(this, dspfd, eSocketNotifier::Write, 0);
		CONNECT(outputsn->activated, eMP3Decoder::outputReady);
	} else
		outputsn=0;
	
	if (sourcefd >= 0)
	{
		inputsn=new eSocketNotifier(this, sourcefd, eSocketNotifier::Read, 0);
		CONNECT(inputsn->activated, eMP3Decoder::decodeMore);
	} else
		inputsn=0;
	
	CONNECT(messages.recv_msg, eMP3Decoder::gotMessage);
	
	maxOutputBufferSize=256*1024;
	
	speed=1;
	framecnt=0;

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
		outputbr=synth.pcm.samplerate*synth.pcm.channels*16;
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
		if (state!=stateFileEnd)
			state=stateBuffering;
		else
		{
			eDebug("ok, everything played..");
			handler->messages.send(eServiceHandlerMP3::eMP3DecoderMessage(eServiceHandlerMP3::eMP3DecoderMessage::done));
		}
	}
}

void eMP3Decoder::dspSync()
{
	if (dspfd >= 0)
		::ioctl(dspfd, SNDCTL_DSP_RESET);
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
			
			if (!read_size)
				break;
		
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
		{
			eLocker l(poslock);
			if (avgbr==-1)
				avgbr=frame.header.bitrate;
			else
				avgbr=(avgbr*7+frame.header.bitrate)>>3;
			if (avgbr)
			{
				length=filelength/(avgbr>>3);
				position=::lseek(sourcefd, 0, SEEK_CUR);
				if (position > 0 )
					position/=(avgbr>>3);
				else
					position=-1;
			} else
				length=position=-1;
		}

		if (++framecnt < speed)
			continue;
		framecnt=0;

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
	{
		eDebug("end of file...");
		state=stateFileEnd;
		inputsn->stop();
	}
	
	if ((state == statePlaying) && (output.size() >= maxOutputBufferSize))
	{
		state=stateBufferFull;
		inputsn->stop();
	}
}

eMP3Decoder::~eMP3Decoder()
{
	kill(); // wait for thread exit.

	if (inputsn)
		delete inputsn;
	if (outputsn)
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
		if (state == stateError)
			break;
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
	case eMP3DecoderMessage::setSpeed:
		if (state == stateError)
			break;
		speed=message.parm;
		if (message.parm == 0)
		{
			if ((state==stateBuffering) || (state==stateBufferFull) || (statePlaying))
			{
				inputsn->stop();
				outputsn->stop();
				state=statePause;
				dspSync();
			}
		} else if (state == statePause)
		{
			inputsn->start();
			outputsn->start();
			speed=message.parm;
			state=stateBuffering;
		} else
		{
			output.clear();
			dspSync();
		}
		break;
	case eMP3DecoderMessage::seek:
	case eMP3DecoderMessage::seekreal:
	case eMP3DecoderMessage::skip:
	{
		if (state == stateError)
			break;
		int offset=0;
		eDebug("cmd");
		
		if (message.type != eMP3DecoderMessage::seekreal)
		{
			int br=avgbr;
			if (br <= 0)
				br=192000;
			br/=8;
		
			br*=message.parm;
			offset=input.size();
			input.clear();
			offset+=br/1000;
			eDebug("skipping %d bytes (br: %d)..", offset, br);
			if (message.type == eMP3DecoderMessage::skip)
				offset+=::lseek(sourcefd, 0, SEEK_CUR);
			if (offset<0)
				offset=0;
		} else
		{
			input.clear();
			offset=message.parm;
		}

		eDebug("seeking to %d", offset);
		::lseek(sourcefd, offset, SEEK_SET);
		dspSync();
		output.clear();
		if (stream.buffer)
			mad_stream_sync(&stream);
		
		if (state == statePlaying)
		{
			inputsn->start();
			state=stateBuffering;
		}
		
		break;
	}
	}
}

int eMP3Decoder::getPosition(int real)
{
	eLocker l(poslock);
	if (real)
		return ::lseek(sourcefd, 0, SEEK_CUR)-input.size();
	return position;
}

int eMP3Decoder::getLength(int real)
{
	eLocker l(poslock);
	if (real)
		return filelength;
	return length+output.size()/(outputbr/8);
}

void eServiceHandlerMP3::gotMessage(const eMP3DecoderMessage &message)
{
	if (message.type == eMP3DecoderMessage::done)
	{
		state=stateStopped;
		serviceEvent(eServiceEvent(eServiceEvent::evtEnd));
	}
}

eService *eServiceHandlerMP3::createService(const eServiceReference &service)
{
#if 0
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
	
	const char *naming="[%a] [%1 - %3] %2";
	
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
#else
	eString l=service.path.mid(service.path.rfind('/')+1);
	return new eService(eServiceID(0), l.c_str());
#endif
}

int eServiceHandlerMP3::play(const eServiceReference &service)
{
	decoder=new eMP3Decoder(service.path.c_str(), this);
	decoder->messages.send(eMP3Decoder::eMP3DecoderMessage(eMP3Decoder::eMP3DecoderMessage::start));
	
	if (!decoder->getError())
		state=statePlaying;
	else
		state=stateError;
	
	serviceEvent(eServiceEvent(eServiceEvent::evtStart));
	serviceEvent(eServiceEvent(eServiceEvent::evtFlagsChanged) );

	return 0;
}

int eServiceHandlerMP3::getErrorInfo()
{
	return decoder ? decoder->getError() : -1;
}

int eServiceHandlerMP3::serviceCommand(const eServiceCommand &cmd)
{
	if (!decoder)
	{
		eDebug("no decoder");
		return 0;
	}
	switch (cmd.type)
	{
	case eServiceCommand::cmdSetSpeed:
		if ((state == statePlaying) || (state == statePause) || (state == stateSkipping))
		{
			if (cmd.parm < 0)
				return -1;
			decoder->messages.send(eMP3Decoder::eMP3DecoderMessage(eMP3Decoder::eMP3DecoderMessage::setSpeed, cmd.parm));
			if (cmd.parm == 0)
				state=statePause;
			else if (cmd.parm == 1)
				state=statePlaying;
			else
				state=stateSkipping;
		} else
			return -2;
		break;
	case eServiceCommand::cmdSkip:
		decoder->messages.send(eMP3Decoder::eMP3DecoderMessage(eMP3Decoder::eMP3DecoderMessage::skip, cmd.parm));
		break;
	case eServiceCommand::cmdSeekAbsolute:
		decoder->messages.send(eMP3Decoder::eMP3DecoderMessage(eMP3Decoder::eMP3DecoderMessage::seek, cmd.parm));
		break;
	case eServiceCommand::cmdSeekReal:
		eDebug("seekreal");
		decoder->messages.send(eMP3Decoder::eMP3DecoderMessage(eMP3Decoder::eMP3DecoderMessage::seekreal, cmd.parm));
		break;
	default:
		return -1;
	}
	return 0;
}

eServiceHandlerMP3::eServiceHandlerMP3(): eServiceHandler(0x1000), messages(eApp, 0)
{
	if (eServiceInterface::getInstance()->registerHandler(id, this)<0)
		eFatal("couldn't register serviceHandler %d", id);
	CONNECT(eServiceFileHandler::getInstance()->fileHandlers, eServiceHandlerMP3::addFile);
	CONNECT(messages.recv_msg, eServiceHandlerMP3::gotMessage);
	decoder=0;
}

eServiceHandlerMP3::~eServiceHandlerMP3()
{
	eServiceInterface::getInstance()->unregisterHandler(id);
}

void eServiceHandlerMP3::addFile(void *node, const eString &filename)
{
	if (filename.right(4).upper()==".MP3")
	{
		struct stat s;
		if (::stat(filename.c_str(), &s))
			return;
		eServiceFileHandler::getInstance()->addReference(node, eServiceReference(id, 0, filename));
	}
}

eService *eServiceHandlerMP3::addRef(const eServiceReference &service)
{
	return eServiceFileHandler::getInstance()->addRef(service);
}

void eServiceHandlerMP3::removeRef(const eServiceReference &service)
{
	return eServiceFileHandler::getInstance()->removeRef(service);
}

int eServiceHandlerMP3::getFlags()
{
	return flagIsSeekable|flagSupportPosition;
}

int eServiceHandlerMP3::getState()
{
	return state;
}

int eServiceHandlerMP3::stop()
{
	decoder->messages.send(eMP3Decoder::eMP3DecoderMessage(eMP3Decoder::eMP3DecoderMessage::exit));
	delete decoder;
	decoder=0;
	serviceEvent(eServiceEvent(eServiceEvent::evtStop));
	return 0;
}

int eServiceHandlerMP3::getPosition(int what)
{
	if (!decoder)
		return -1;
	switch (what)
	{
	case posQueryLength:
		return decoder->getLength(0);
	case posQueryCurrent:
		return decoder->getPosition(0);
	case posQueryRealLength:
		return decoder->getLength(1);
	case posQueryRealCurrent:
		return decoder->getPosition(1);
	default:
		return -1;
	}
}

eAutoInitP0<eServiceHandlerMP3> i_eServiceHandlerMP3(7, "eServiceHandlerMP3");

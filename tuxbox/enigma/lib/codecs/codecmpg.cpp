#ifndef DISABLE_FILE

#include "lib/codecs/codecmpg.h"
#include <lib/dvb/decoder.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <lib/base/eerror.h>

unsigned long eMPEGDemux::getLong()
{
	unsigned long c;
	if (input.read(&c, 4) != 4)
	{
		eDebug("read error ! :))");
		return 0x1b9;		// simulate end of program stream.
	}
	c=htonl(c);
	return c;
}

void eMPEGDemux::refill()
{
	last=getLong();
	remaining=32;
}

unsigned long eMPEGDemux::getBits(unsigned int num)
{
	unsigned long res=0;
	while (num)
	{
		if (!remaining)
			refill();
		unsigned int d=num;
		if (d > remaining)
			d=remaining;
		res<<=d;
		res|=(last>>(remaining-d))&~(-1<<d);
		remaining-=d;
		num-=d;
	}
	return res;
}

void eMPEGDemux::syncBits()
{
		// round UP. so we re-read the last octet.
		// that's ok, since syncBits() only does something
		// when we're out of sync. and in that case, we might have
		// already read one bit of the startcode.
	remaining+=7;
	remaining&=~7;
}

eMPEGDemux::eMPEGDemux(eIOBuffer &input, eIOBuffer &video, eIOBuffer &audio)
	:input(input), video(video), audio(audio), minFrameLength(4096),
	mpegtype(-1), curAudioStreamID(0), validpackets(0),
	lastAudioPTS(-1), lastVideoPTS(-1), synced(0)
{
	remaining=0;
	memset(&pcmsettings, 0, sizeof(pcmsettings));
//	fAudio=fopen("/hdd/audio.pes", "w+" );
//	fVideo=fopen("/hdd/video.pes", "w+" );
}

int eMPEGDemux::decodeMore(int last, int maxsamples, Signal1<void,unsigned int>*newastreamid )
{
//	eDebug("decoderMore");
	int written=0;
	(void)last;

	while (written < maxsamples)
	{
		unsigned int code=0;
		while (1)	// search for start code.
		{
			if (input.size() < 4096)
			{
				maxsamples=0;
				break;
			}
			syncBits();
			if (getBits(8))
			{
//				eDebug("skip");
				continue;
			}
a:
			if (getBits(8))
			{
//				eDebug("skip");
				continue;
			}
			int c=getBits(8);
			if (!c)
			{
//				eDebug("skip");
				goto a;
			}
			if (c != 1)
			{
//				eDebug("skip");
				continue;
			}
			if (!maxsamples)
				break;
			code = getBits(8);
			switch (code)
			{
				case 0xb9: // MPEG_program_end_code
					goto finish;
				case 0xba: // pack_start_code
				{
					int type=getBits(2);
					if ( type != mpegtype )
					{
						switch (type)
						{
							case 1:
								Decoder::SetStreamType(TYPE_PES);
								break;
							default:
								Decoder::SetStreamType(TYPE_MPEG1);
								break;
						}
						mpegtype=type;
						eDebug("set %s", type == 1 ? "MPEG-2" : "MPEG-1" );
					}
					if (type != 1)
					{
						getBits(6);
						getBits(16);
						getBits(16);
						getBits(16);
						getBits(8);
						continue;
					}
					int scr_base0, scr_base, scr_ext;
					scr_base0=getBits(3);
					scr_base=scr_base0<<30;
					scr_base0>>=2;
					if (!getBits(1))
						continue;
					scr_base|=getBits(15)<<15;
					if (!getBits(1))
						continue;
					scr_base|=getBits(15);
					if (!getBits(1))
						continue;
					scr_ext=getBits(9);
					if (!getBits(1))
						continue;
					/* int program_mux_rate= */ getBits(22);
					if (getBits(2) != 3)
						continue;
					getBits(5);
					int pack_stuffing_length=getBits(3);
					while (pack_stuffing_length--)
						if (getBits(8) != 0xFF)
							break;
					if (pack_stuffing_length >= 0)
						continue;
//					eDebug("scr: %08x:%02d\n", scr_base, scr_ext);
					break;
				}
				case 0xbb:  // system_header_start_code
				case 0xBE:  // Padding Stream
				case 0xBF:  // Private Stream 2 (???)
				case 0xF0:
				case 0xF1:
				case 0xF2:
				case 0xF3:
				case 0xFF:
				{
//					eDebug("system_header");
					int length=getBits(16);
					while ( length && remaining )
					{
						getBits(8);
						--length;
					}
					if ( length )
					{
						char buffer[length];
						if ( input.read(buffer, length) != length )
							eDebug("read Error in skip");
					}
					break;
				}
				case 0xbc: // program_stream_map
				{
					eDebug("program stream map!\n");
#if 0  //
					int program_stream_map_length=getBits(16);
					eDebug("program stream map!\n");
					int current_next_indicator=getBits(1);
					getBits(2);
					int program_stream_map_version=getBits(5);
					getBits(7);
					if (!getBits(1))
						continue;
					int program_stream_info_length=getBits(16);
					for (int r=0; r<program_stream_info_length; )
					{
						int tag=getBits(8);
						length=getBits(8);
						r+=length+2;
						eDebug("tag: %02x %02x ", tag, length);
						while (length--)
							eDebug("%02lx ", getBits(8));
						eDebug("\n");
					}
					int elementary_stream_map_length=getBits(16);
					for (int r=0; r < elementary_stream_map_length; )
					{
						int stream_type=getBits(8);
						int elementary_stream_id=getBits(8);
						int elementary_stream_info_length=getBits(16);
						for (int a=0; a < elementary_stream_info_length; )
						{
							int tag=getBits(8);
							int length=getBits(8);
							eDebug("elementary: %02x %02x ", tag, length);
							while (length--)
								eDebugNoNewLine("%02x ", getBits(8));
							eDebug("\n");
						}
						r+=elementary_stream_info_length+4;
					}
					getBits(32);
#endif
					break;
				}
  			case 0xC0 ... 0xCF:  // Audio Stream
				case 0xD0 ... 0xDF:
				{
					int &cnt = audiostreams[code];
					if ( cnt < 30 )
					{
						cnt++;
						if ( cnt == 30 )
						{
							eDebug("/*emit*/ (*newastreamid)(%02x)", code);
							if ( !curAudioStreamID )
							{
								Decoder::parms.audio_type = DECODE_AUDIO_MPEG;
								Decoder::Set();
								curAudioStreamID = code;
							}
							/*emit*/ (*newastreamid)(code);
						}
					}
				}
				case 0xBD:  // Private Stream 1 (AC3 or ttx)
				case 0xE0 ... 0xEF:  // Video Stream
				{
					int length=getBits(16);
					unsigned char buffer[6+length];
					int p=0;

					buffer[p++]=0;
					buffer[p++]=0;
					buffer[p++]=1;
					buffer[p++]=code;

					buffer[p++]=length>>8;
					buffer[p++]=length&0xFF;

					while ( length && remaining )
					{
						buffer[p++]=getBits(8);
						--length;
					}

					if ( length )
					{
						if ( input.read(buffer+p, length) != length )
						{
							eDebug("read Error");
							minFrameLength+=4096;
						}
/*						else
							eDebug("read %04x bytes", length);*/
						
						p+=length;
					}

					if ( code == 0xBD )
					{
						int offs = buffer[8];
						int subid = buffer[8+offs+1];

//						eDebug("offs = %02x, subid = %02x",offs, subid);

						
//						if ( offs == 0x24 && subid == 0x10 ) // TTX stream...
//							break;

						if ( subid < 0x80 )
							break;

//						if ( subid > 0xA7 ) // 0xA0 .. 0xA7  (LPCM)
//							break;

						if ( subid > 0x87 ) // 0x88 .. 0x89   (DTS)
							break;
/*
						for (int i=0; i < 32; ++i )
							eDebugNoNewLine("%02x ", buffer[i]);
						eDebug("");*/

						// here we have subid 0x80 .. 0x87
						code |= (subid << 8);
						int &cnt = audiostreams[code];
						if ( cnt < 30 )
						{
							cnt++;
							if ( cnt == 30 )
							{
								eDebug("found new AC3 stream subid %02x", subid);
								eDebug("/*emit*/ (*newastreamid)(%04x)", code);
								if ( !curAudioStreamID )
								{
									Decoder::parms.audio_type = DECODE_AUDIO_AC3_VOB;
									Decoder::Set();
									curAudioStreamID = code;
								}
								/*emit*/ (*newastreamid)(code);
							}
						}
					}
					if (code == 0xE0)
					{
						if (validpackets < 100)
						{
							++validpackets;
							break;
						}
						if ( lastVideoPTS == -1 ||
							( lastAudioPTS != -1 && abs(lastAudioPTS-lastVideoPTS) > 0x30000 ) )
						{
							int pos=5;
							while( buffer[++pos] == 0xFF );  // stuffing überspringen
								if ( (buffer[pos] & 0xC0) == 0x40 ) // buffer scale size
									pos+=2;
							if ( ((buffer[pos] & 0xF0) == 0x20) ||  //MPEG1
									 ((buffer[pos] & 0xF0) == 0x30) ||  //MPEG1
									 ((buffer[pos] & 0xC0) == 0x80) )   //MPEG2
							{
								int readPTS=1;
								if ((buffer[pos] & 0xC0) == 0x80) // we must skip many bytes
								{
									if ((c & 0x30) != 0)
										eDebug("warning encrypted multiplex not handled!!!");
									++pos; // flags
									if ( ((buffer[pos]&0xC0) != 0x80) &&
											 ((buffer[pos]&0xC0) != 0xC0) )
										readPTS=0;
									pos+=2;
								}
								if (readPTS)
								{
									lastVideoPTS = (buffer[pos++] & 0x0E) << 28;
									lastVideoPTS |= (buffer[pos++] & 0xFF) << 21;
									lastVideoPTS |= (buffer[pos++] & 0xFE) << 13;
									lastVideoPTS |= (buffer[pos++] & 0xFF) << 5;
									lastVideoPTS |= (buffer[pos] >> 1) & 0x1F;
									eDebug("VPTS %08x", lastVideoPTS);
								}
							}
						}
						video.write(buffer, p);
//						fwrite(buffer, p, 1, fVideo);
						written+=p;
					}
					else if ( code == curAudioStreamID )
					{
						if ( validpackets < 100)
						{
							++validpackets;
							break;
						}
						if ( !synced )
						{
							int pos=5;
							while( buffer[++pos] == 0xFF );  // stuffing überspringen
								if ( (buffer[pos] & 0xC0) == 0x40 ) // buffer scale size
									pos+=2;
							if ( ((buffer[pos] & 0xF0) == 0x20) ||  //MPEG1
									 ((buffer[pos] & 0xF0) == 0x30) ||  //MPEG1
									 ((buffer[pos] & 0xC0) == 0x80) )   //MPEG2
							{
								int readPTS=1;
								if ((buffer[pos] & 0xC0) == 0x80) // we must skip many bytes
								{
									if ((c & 0x30) != 0)
										eDebug("warning encrypted multiplex not handled!!!");
									++pos; // flags
									if ( ((buffer[pos]&0xC0) != 0x80) &&
											 ((buffer[pos]&0xC0) != 0xC0) )
										readPTS=0;
									pos+=2;
								}
								if (readPTS)
								{
									lastAudioPTS = (buffer[pos++] & 0x0E) << 28;
									lastAudioPTS |= (buffer[pos++] & 0xFF) << 21;
									lastAudioPTS |= (buffer[pos++] & 0xFE) << 13;
									lastAudioPTS |= (buffer[pos++] & 0xFF) << 5;
									lastAudioPTS |= (buffer[pos] >> 1) & 0x1F;
									if ( lastVideoPTS != -1 )
										eDebug("APTS %08x", lastAudioPTS);
								}
							}
						}
						if ( lastVideoPTS != -1 && lastAudioPTS != -1 )
						{
							if ( synced || abs(lastAudioPTS-lastVideoPTS) < 0x750 )
							{
								synced=1;
								audio.write(buffer, p);
//								fwrite(buffer, p, 1, fAudio);
							}
							else
								eDebug("PTSA = %08x\nPTSV = %08x\nDIFF = %08x",
									lastAudioPTS, lastVideoPTS, abs(lastAudioPTS-lastVideoPTS) );
						}
						written+=p;
					}
					break;
				}
				default:
				{
					if ( validpackets )
					{
						for (std::map<int,int>::iterator it(audiostreams.begin());
							it != audiostreams.end();)
							if ( it->second < 30 )
							{
								audiostreams.erase(it);
								it = audiostreams.begin();
							}
							else
								++it;
						validpackets=0;
						video.clear();
						audio.clear();
					}
					eDebug("unhandled code %02x", code);
				}
			}
		}
	}
finish:
	return written;
}

void eMPEGDemux::resync()
{
	remaining=synced=0;
	lastAudioPTS=lastVideoPTS=-1;
}

int eMPEGDemux::getMinimumFramelength()
{
	return minFrameLength;
}

int eMPEGDemux::getAverageBitrate()
{
	return 3*1024*1024;
}

void eMPEGDemux::setAudioStream( unsigned int id )
{
	if ( curAudioStreamID != id && audiostreams.find(id) != audiostreams.end() )
	{
		if ( (id&0xFF) == 0xBD)
		{
			// not thread safe !!
			Decoder::parms.audio_type = DECODE_AUDIO_AC3_VOB;
			Decoder::Set();
		}
		else
		{
			// not thread safe !!
			Decoder::parms.audio_type = DECODE_AUDIO_MPEG;
			Decoder::Set();
		}
		curAudioStreamID = id;
	}
//	fseek(fAudio, 0, SEEK_SET);
//	fseek(fVideo, 0, SEEK_SET);
}

#endif // DISABLE_FILE

// lpcm
// lpcm dvd
// frame rate 600Hz (48/96kHz)
// 16/20/24 bits
// 8ch
// max 6.144Mbps
// [0] private stream sub type
// [1] number_of_frame_headers
// [2-3] first_access_unit_pointer (offset from [3])
// [4] audio_frame_number
//          (of first access unit (wraps at 19 ?))
//          (20 frames at 1/600Hz = 1/30 (24 frames for 25fps?)
// [5]:
//      b7-b6: quantization_word_length,
//            0: 16bits, 1: 20bits, 2: 24bits, 3: reserved
//      b5: reserved
//      b4: audio_sampling_frequency,
//            0: 48 kHz, 1: 96 kHz
//      b3: reserved
//      b2-b0: number_of_audio_channels
//            0: mono (2ch ? dual-mono: L=R)
//            1: 2ch (stereo)
//            2: 3 channel
//            3: 4 ch
//            4: 5 ch
//            5: 6 ch
//            6: 7 ch
//            7: 8 ch
// [6]: dynamic_range

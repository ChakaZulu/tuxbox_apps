#ifndef DISABLE_FILE

#include "lib/codecs/codecmpg.h"
#include <lib/dvb/decoder.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <lib/base/eerror.h>
#include <lib/system/econfig.h>

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

eMPEGDemux::eMPEGDemux(eIOBuffer &input, eIOBuffer &video, eIOBuffer &audio, int fd)
	:input(input), video(video), audio(audio), minFrameLength(4096),
	mpegtype(-1), curAudioStreamID(0), synced(0), fd(fd)
{
	remaining=0;
	memset(&pcmsettings, 0, sizeof(pcmsettings));
}

eMPEGDemux::~eMPEGDemux()
{
	eConfig::getInstance()->setKey("/ezap/audio/prevAudioStreamID", curAudioStreamID);
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
				continue;
			if (getBits(8))
				continue;
a:
			int c=getBits(8);
			if (!c)
				goto a;
			if (c != 1)
				continue;
			if (!maxsamples)
				break;
			code = getBits(8);
			switch (code)
			{
				case 0xb9: // MPEG_program_end_code
				{
					eDebug("program_end_code");
					goto finish;
				}
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
//					eDebug("system_header %02x", code);
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
					if ( cnt < 10 )
					{
						cnt++;
						if ( cnt == 10 )
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
					if ( (length+6) > minFrameLength )
					{
						
						if ( (minFrameLength+2048) > (length+6) )
							minFrameLength=length+6;
						else
							minFrameLength+=2048;
						eDebug("minFrameLength now %d", minFrameLength );
					}
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
						int rd = input.read(buffer+p, length);
						if ( rd != length ) 
						{  // buffer empty.. put all data back in input buffer
							input.write(buffer, p+rd);
							return written;
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
						if ( cnt < 10 )
						{
							cnt++;
							if ( cnt == 10 )
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
		// check old audiopackets in syncbuffer
					if ( syncbuffer.size() )
					{
						unsigned int VideoPTS=0xFFFFFFFF;
						Decoder::getVideoPTS(VideoPTS);
						if ( VideoPTS != 0xFFFFFFFF )
						{
							std::list<syncAudioPacket>::iterator it( syncbuffer.begin() );
							for (;it != syncbuffer.end(); ++it )
							{
								if ( abs(VideoPTS - it->pts) <= 0x1000 )
								{
									eDebug("synced2 :)");
									break;
								}
							}
							if ( it != syncbuffer.end() )
							{
								synced=1;
		// write data from syncbuffer to audio device
								for (;it != syncbuffer.end(); ++it )
									audio.write( it->data, it->len );
		// cleanup syncbuffer
								for (it=syncbuffer.begin();it != syncbuffer.end(); ++it )
									delete [] it->data;
								syncbuffer.clear();
							}
						}
					}
					if (code > 0xDF && code < 0xF0)
					{
						videostreams.insert(code);
						if ( code != 0xE0 && videostreams.find(240) != videostreams.end() )
							; // dont play video streams != 0xE0 when 0xE0 is avail...
						else
						{
							video.write(buffer, p);
							written+=p;
						}
					}
					else if ( code == curAudioStreamID )
					{
		// check current audiopacket
						if (!synced)
						{
							unsigned int AudioPTS = 0xFFFFFFFF,
													 VideoPTS = 0xFFFFFFFF,
													 pos=5;
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
									AudioPTS = (buffer[pos++] >> 1) << 29;
									AudioPTS |= buffer[pos++] << 21;
									AudioPTS |=(buffer[pos++] >> 1) << 14;
									AudioPTS |= buffer[pos++] << 6;
									AudioPTS |= buffer[pos] >> 2;
//									eDebug("APTS %08x", AudioPTS);
								}
							}
							Decoder::getVideoPTS(VideoPTS);
							if ( VideoPTS != 0xFFFFFFFF && abs(VideoPTS - AudioPTS) <= 0x1000 )
							{
								synced=1;
								eDebug("synced1 :)");
		// cleanup syncbuffer.. we don't need content of it
								std::list<syncAudioPacket>::iterator it( syncbuffer.begin() );
								for (;it != syncbuffer.end(); ++it )
									delete [] it->data;
								syncbuffer.clear();
							}
							else if ( (AudioPTS > VideoPTS) || VideoPTS == 0xFFFFFFFF )
							{
								syncAudioPacket pack;
								pack.pts = AudioPTS;
								pack.len = p;
								pack.data = new __u8[p];
								memcpy( pack.data, buffer, pack.len );
								syncbuffer.push_back( pack );
//								eDebug("PTSA = %08x\nPTSV = %08x\nDIFF = %08x",
//									AudioPTS, VideoPTS, abs(AudioPTS-VideoPTS) );
							}
						}
						if ( synced )
							audio.write(buffer, p);
						written+=p;
					}
					break;
				}
				default:
				{
					if ( audio.size() || video.size() )
						eDebug("unhandled code... but already data in buffers!!");
					for (std::map<int,int>::iterator it(audiostreams.begin());
						it != audiostreams.end();)
					{
						if ( it->second < 10 )
						{
							audiostreams.erase(it);
							it = audiostreams.begin();
						}
						else
							++it;
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
// clear syncbuffer
	std::list<syncAudioPacket>::iterator it( syncbuffer.begin() );
	for (;it != syncbuffer.end(); ++it )
		delete [] it->data;
	syncbuffer.clear();
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

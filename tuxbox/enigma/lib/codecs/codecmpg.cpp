#include "lib/codecs/codecmpg.h"
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

unsigned long eMPEGDemux::getLong()
{
	unsigned long c;
	if (input.read(&c, 4) != 4)
	{
		printf("read error ! :))\n");
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
	: input(input), video(video), audio(audio)
{
	remaining=0;
	memset(&pcmsettings, 0, sizeof(pcmsettings));
}

int eMPEGDemux::decodeMore(int last, int maxsamples)
{
	int written=0;
	
	while (written < maxsamples)
	{
		int scerr=0;
		while (1)	// search for start code.
		{
			if (input.size() < 4096)
			{
				maxsamples=0;
				break;
			}
			if (scerr++)
				printf("startcode search!\n");
			syncBits();
			if (getBits(8))
				continue;
a:
			if (getBits(8))
				continue;
			int c=getBits(8);
			if (!c)
				goto a;
			if (c != 1)
				continue;
			break;
		}
		if (!maxsamples)
			break;
		unsigned int code=getBits(8);
//		printf("startcode: %08x\n", code|0x100);
		if (code == 0xb9) // MPEG_program_end_code
			break;
		else if (code == 0xba) // pack_start_code
		{
			if (getBits(2) != 1)
				continue;
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
//			printf("scr: %08x:%02d\n", scr_base, scr_ext);
		} else if (code == 0xbb) // system_header_start_code
		{
			printf("system_header\n");
			int header_length=getBits(16);
			while (header_length--)
				getBits(8);
#if 0
		} else if (code == 0xbc) // program_stream_map
		{
			int program_stream_map_length=getBits(16);
			printf("program stream map!\n");
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
				int length=getBits(8);
				printf("tag: %02x %02x ", tag, length);
				while (length--)
					printf("%02lx ", getBits(8));
				printf("\n");
				r+=length+2;
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
					printf("elementary: %02x %02x ", tag, length);
					while (length--)
						printf("%02x ", getBits(8));
					printf("\n");
				}
				
				r+=elementary_stream_info_length+4;
			}
			getBits(32);
#endif
		} else  // if (((code & 0xE0) == 0xC0) || ((code & 0xF0)==0xE0))
		{
//			printf("PES: %x\n", code);
			int length=getBits(16);
//			printf("%d bytes!\n", length);
			unsigned char buffer[65536+6];
			int p=0;
			
			buffer[p++]=0;
			buffer[p++]=0;
			buffer[p++]=1;
			buffer[p++]=code;
			buffer[p++]=length>>8;
			buffer[p++]=length&0xFF;
			
					// empty bitbuffer
			while (length && remaining)
			{
				buffer[p++]=getBits(8);
				length--;
			}
					// now we are synced (if still something to read)
			if (length)
				input.read(buffer+p, length);
			p+=length;
			
			if (code == 0xE0)
			{
				video.write(buffer, p);
				written+=p;
			}
			else if (code == 0xC0)
			{
				audio.write(buffer, p);
				written+=p;
			}
		}
	}
	return written;
}

void eMPEGDemux::resync()
{
	remaining=0;
}

int eMPEGDemux::getMinimumFramelength()
{
	return 4096;
}

int eMPEGDemux::getAverageBitrate()
{
	return 1234567;
}

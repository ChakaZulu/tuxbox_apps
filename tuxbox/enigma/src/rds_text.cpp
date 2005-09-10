#include <src/rds_text.h>

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include <errno.h>

#define SWAP(x)	((x<<8)|(x>>8))
#define LO(x)	(x&0xFF)

static inline unsigned short crc_ccitt_byte( unsigned short crc, unsigned char c )
{
	crc = SWAP(crc) ^ c;
	crc = crc ^ (LO(crc) >> 4);
	crc = crc ^ (SWAP(LO(crc)) << 4) ^ (LO(crc) << 5);
	return crc;
}

RDSTextDecoder::RDSTextDecoder()
	:bytesread(0), ptr(0), p1(-1), p2(-1), leninfo(0), text_len(0), m_ptr(0), state(0), sn(0)
{
	int fd=open("/dev/dvb/card0/ancillary0", O_RDONLY|O_NONBLOCK );
	if ( fd < 0 )
		eDebug("open /dev/dvb/card0/ancillary0 failed(%m)");
	else
	{
		sn = new eSocketNotifier(eApp, fd, eSocketNotifier::Read);
		CONNECT(sn->activated, RDSTextDecoder::process_data);
	}
}

RDSTextDecoder::~RDSTextDecoder()
{
	int fd = sn ? sn->getFD() : -1;
	if ( fd != -1 )
	{
		delete sn;
		close(fd);
	}
}

void RDSTextDecoder::process_data(int what)
{
	int rd=read(sn->getFD(), buf+bytesread, 128-bytesread);
	if ( rd >= 0 )
	{
		bytesread+=rd;
		if ( bytesread == 128 )
		{
			while(ptr<128)
			{
				if ( buf[ptr] == 0xFD )
				{
					if (p1 == -1)
						p1 = ptr;
					else
						p2 = ptr;
				}
				if ( p1 != -1 && p2 != -1 )
				{
					int cnt=buf[--p2];
					while ( cnt-- > 0 )
					{
						unsigned char c = buf[--p2];
						if ( state == 1 )
							crc=0xFFFF;
						if ( state >= 1 && state < 11 )
							crc = crc_ccitt_byte(crc, c);

						switch (state)
						{
							case 0:
								if ( c==0xFE )  // Startkennung
									state=1;
								break;
							case 1: // 10bit Site Address + 6bit Encoder Address
							case 2:
							case 3: // Sequence Counter
								++state;
								break;
							case 4:
								leninfo=c;
								++state;
								break;
							case 5:
								if ( c==0x0A ) // message element code 0x0A Radio Text
									++state;
								else
									state=0;
								break;
							case 6: // Data Set Number ... ignore
							case 7: // Program Service Number ... ignore
								++state;
								break;
							case 8: // Message Element Length
								text_len=c;
								if ( !text_len || text_len > 65 || text_len > leninfo-4)
									state=0;
								else
								{
									++state;
									text_len-=2;
									m_ptr=0;
								}
								break;
							case 9: // Radio Text Status bit:
								// 0   = AB-flagcontrol
								// 1-4 = Transmission-Number
								// 5-6 = Buffer-Config
								++state; // ignore ...
								break;
							case 10:
	// TODO build a complete radiotext charcode to UTF8 conversion table for all character > 0x80
								switch (c)
								{
									case 0 ... 0x79: break;
									case 0x8d: c='ß'; break;
									case 0x91: c='ä'; break;
									case 0xd1: c='Ä'; break;
									case 0x97: c='ö'; break;
									case 0xd7: c='Ö'; break;
									case 0x99: c='ü'; break;
									case 0xd9: c='Ü'; break;
									default: c=' '; break;  // convert all unknown to space
								}
								message[m_ptr++]=c;
								if(text_len)
									--text_len;
								else
									++state;
								break;
							case 11:
								crc16=c<<8;
								++state;
								break;
							case 12:
								crc16|=c;
								message[m_ptr--]=0;
								while(message[m_ptr] == ' ' && m_ptr > 0)
									message[m_ptr--] = 0;
								if ( crc16 == (crc^0xFFFF) )
									/*emit*/ textReady((const char*)message);
								else
									eDebug("invalid rdstext crc (%s)", message);
								state=0;
								break;
						}
					}
					p1=ptr;
					p2=-1;
				}
				++ptr;
			}
			if (p1 != -1 && (128-p1) != 128)
			{
				bytesread=ptr=128-p1;
				memcpy(buf, buf+p1, ptr);
				p1=0;
			}
			else
				bytesread=ptr=0;
		}
	}
}

#include <lib/dvb/cahandler.h>
#include <unistd.h>
#include <lib/base/eerror.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/system/info.h>
#include <lib/gdi/font.h>

eDVBCAHandler::eDVBCAHandler()
{
	CONNECT( eDVB::getInstance()->leaveTransponder, eDVBCAHandler::leaveTransponder );
	eDVBCaPMTClientHandler::registerCaPMTClient(this);  // static method...
}

eDVBCAHandler::~eDVBCAHandler()
{
	eDVBCaPMTClientHandler::unregisterCaPMTClient(this);  // static method...
}

void eDVBCAHandler::leaveTransponder( eTransponder* t )
{
	if ( t )
	{
		int sock, clilen;
		struct sockaddr_un servaddr;
		memset(&servaddr, 0, sizeof(struct sockaddr_un));
		servaddr.sun_family = AF_UNIX;
		strcpy(servaddr.sun_path, "/tmp/camd.socket");
		clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
		sock = socket(PF_UNIX, SOCK_STREAM, 0);
		int err=0;
		if ( connect(sock, (struct sockaddr *) &servaddr, clilen) )
		{
			eDebug("[eDVBCAHandler] (leaveTP) connect (%m)");
			err = 1;
		}
		fcntl(sock, F_SETFL, O_NONBLOCK);
		int val=1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, 4);
		if (!err)
		{
			const char *msg="\x9f\x80\x3f\x04\x83\x02\x03\x01";
			if ( ::write(sock, msg, 8) != 8 )
				eDebug("[eDVBCAHandler] (leaveTP) write (%m)");
		}
		::close(sock);
	}
}

void CAService::sendCAPMT( PMT *pmt )
{
	if ( pmt->version == lastPMTVersion )
	{
		eDebug("[eDVBCAHandler] dont send the self pmt version");
		return;
	}
	unsigned char capmt[1024];

	memcpy(capmt,"\x9f\x80\x32\x82\x00\x00", 6);

	capmt[6]=lastPMTVersion==-1 ? 0x03 /*only*/ : 0x05 /*update*/;
	capmt[7]=(unsigned char)((pmt->program_number>>8) & 0xff);			//prg-nr
	capmt[8]=(unsigned char)(pmt->program_number & 0xff);					//prg-nr

	capmt[9]=pmt->version;	//reserved - version - current/next
	capmt[10]=0x00;	//reserved - prg-info len
	capmt[11]=0x00;	//prg-info len

	capmt[12]=0x01;  // ca pmt command id
	capmt[13]=0x81;  // private descr.. dvbnamespace
	capmt[14]=0x08;
	capmt[15]=me.getDVBNamespace().get()>>24;
	capmt[16]=(me.getDVBNamespace().get()>>16)&0xFF;
	capmt[17]=(me.getDVBNamespace().get()>>8)&0xFF;
	capmt[18]=me.getDVBNamespace().get()&0xFF;
	capmt[19]=me.getTransportStreamID().get()>>8;
	capmt[20]=me.getTransportStreamID().get()&0xFF;
	capmt[21]=me.getOriginalNetworkID().get()>>8;
	capmt[22]=me.getOriginalNetworkID().get()&0xFF;

	capmt[23]=0x82;  // demuxer kram..
	capmt[24]=0x02;
	if ( eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000 )
	{
		capmt[25]=0x03;  // descramble on demux0 and demux1
		capmt[26]=0x01;  // get section data from demux1
	}
	else
	{
		capmt[25]=0x01;  // only descramble on demux0
		capmt[26]=0x00;  // get section data from demux0
	}

	capmt[27]=0x84;  // pmt pid
	capmt[28]=0x02;
	capmt[29]=pmt->pid>>8;
	capmt[30]=pmt->pid&0xFF;

	lastPMTVersion=pmt->version;
	int lenpos=10;
	int len=19;
	int first=0;
	int wp=31;

	// program_info
	for (ePtrList<Descriptor>::const_iterator i(pmt->program_info);
		i != pmt->program_info.end(); ++i)
	{
		if (i->Tag()==9)	// CADescriptor
		{
			CADescriptor *ca=(CADescriptor*)*i;
			memcpy(capmt+wp, ca->data, ca->data[1]+2);
			wp+=ca->data[1]+2;
			len+=ca->data[1]+2;
		}
	}

	for (ePtrList<PMTEntry>::iterator i(pmt->streams); i != pmt->streams.end(); ++i)
	{
		PMTEntry *pe=*i;

		first=1;
		capmt[lenpos]=((len & 0xf00)>>8);
		capmt[lenpos+1]=(len & 0xff);
		len=0;
		lenpos=wp+3;
		first=1;
		capmt[wp++]=(pe->stream_type & 0xffff);
		capmt[wp++]=((pe->elementary_PID >> 8) & 0xff);
		capmt[wp++]=(pe->elementary_PID & 0xff);
		wp+=2;

		switch (pe->stream_type)
		{
			case 1: // ISO/IEC 11172 Video
			case 2: // ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream
			case 3: // ISO/IEC 11172 Audio
			case 4: // ISO/IEC 13818-3 Audio
			case 6: // private stream ( ttx or AC3 or DTS )
				for (ePtrList<Descriptor>::const_iterator i(pe->ES_info);
					i != pe->ES_info.end(); ++i)
				{
					if (i->Tag()==9)	// CADescriptor
					{
						CADescriptor *ca=(CADescriptor*)*i;
						if(first)
						{
							first=0;
							capmt[wp++]=0x01;				//ca_pmt_command_id
							len++;
						}
						memcpy(capmt+wp, ca->data, ca->data[1]+2);
						wp+=ca->data[1]+2;
						len+=ca->data[1]+2;
					}
				}
			default:
				break;
		}
	}
	capmt[lenpos]=((len & 0xf00)>>8);
	capmt[lenpos+1]=(len & 0xff);

	capmt[4]=((wp-6)>>8) & 0xff;
	capmt[5]=(wp-6) & 0xff;

	if ( write(sock, capmt, wp) == wp )
	{
#if 0
		eDebug("[eDVBCAHandler] send %d bytes",wp);

		for(int i=0;i<wp;i++)
			eDebugNoNewLine("%02x ",capmt[i]);
		eDebug("");
#endif
	}
	else
		eDebug("[eDVBCAHandler] (sendCAPMT) write (%m)");
}

eAutoInitP0<eDVBCAHandler> init_eDVBCAHandler(eAutoInitNumbers::osd-2, "eDVBCAHandler");

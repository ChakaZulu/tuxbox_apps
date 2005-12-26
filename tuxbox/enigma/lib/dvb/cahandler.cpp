#include <unistd.h>
#include <string.h>

#include <lib/dvb/cahandler.h>
#include <lib/base/eerror.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/system/info.h>

ePMTClient::ePMTClient(eDVBCAHandler *handler, int socket)
 : eUnixDomainSocket(socket, 1, eApp), parent(handler)
{
	CONNECT(connectionClosed_, ePMTClient::connectionLost);
}

void ePMTClient::connectionLost()
{
	if (parent) parent->connectionLost(this);
}

eDVBCAHandler::eDVBCAHandler()
 : eServerSocket(PMT_SERVER_SOCKET, eApp)
{
	services.setAutoDelete(true);
	clients.setAutoDelete(true);
	CONNECT( eDVB::getInstance()->leaveTransponder, eDVBCAHandler::leaveTransponder );
	eDVBCaPMTClientHandler::registerCaPMTClient(this);  // static method...
}

eDVBCAHandler::~eDVBCAHandler()
{
	eDVBCaPMTClientHandler::unregisterCaPMTClient(this);  // static method...
}

void eDVBCAHandler::newConnection(int socket)
{
	ePMTClient *client = new ePMTClient(this, socket);
	clients.push_back(client);

	/* inform the new client about our current services, if we have any */
	distributeCAPMT();
}

void eDVBCAHandler::connectionLost(ePMTClient *client)
{
	ePtrList<ePMTClient>::iterator it = std::find(clients.begin(), clients.end(), client);
	if (it != clients.end())
	{
		clients.erase(it);
	}
}

void eDVBCAHandler::leaveTransponder( eTransponder* t )
{
	if ( t )
	{
		const char *msg = "\x9f\x80\x3f\x04\x83\x02\x03\x01";
		
		/* send msg to the listening client */
		eUnixDomainSocket socket(eApp);
		socket.connectToPath(PMT_CLIENT_SOCKET);
		if (socket.state() == eSocket::Connection) socket.writeBlock(msg, strlen(msg));
	}
}

void eDVBCAHandler::enterService( const eServiceReferenceDVB &service )
{
	ePtrList<CAService>::iterator it =
		std::find(services.begin(), services.end(), service );
	if ( it == services.end() )
	{
		services.push_back(new CAService( service ));
	}

	/*
	 * our servicelist has changed, but we have to wait till we receive PMT data
	 * for this service, before we distribute a new list of CAPMT objects to our clients.
	 */
}

void eDVBCAHandler::leaveService( const eServiceReferenceDVB &service )
{
	ePtrList<CAService>::iterator it =
		std::find(services.begin(), services.end(), service );
	if ( it != services.end() )
	{
		services.erase(it);
	}

	/* our servicelist has changed, distribute the list of CAPMT objects to all our clients */
	distributeCAPMT();
}

void eDVBCAHandler::distributeCAPMT()
{
	/*
	 * write the list of CAPMT objects to each connected client, if it's not empty
	 */
	if (services.empty()) return;

	ePtrList<ePMTClient>::iterator client_it = clients.begin();
	for ( ; client_it != clients.end(); ++client_it)
	{
		if (client_it->state() == eSocket::Connection)
		{
			unsigned char list_management = LIST_FIRST;
			ePtrList<CAService>::iterator it = services.begin();
			for ( ; it != services.end(); )
			{
				CAService *current = it;
				++it;
				if (it == services.end()) list_management |= LIST_LAST;
				current->writeCAPMTObject(*client_it, list_management);
				list_management = LIST_MORE;
			}
		}
	}
}

void eDVBCAHandler::handlePMT( const eServiceReferenceDVB &service, PMT *pmt )
{
	ePtrList<CAService>::iterator it = std::find(services.begin(), services.end(), service);
	if (it != services.end())
	{
		/* we found the service in our list */
		if (it->getCAPMTVersion() == pmt->version)
		{
			eDebug("[eDVBCAHandler] dont send the self pmt version");
			return;
		}
		
		bool isUpdate = (it->getCAPMTVersion() >= 0);

		/* prepare the data */
		it->buildCAPMT(pmt);

		/* send the data to the listening client */
		it->sendCAPMT();

		if (isUpdate)
		{
			/* this is a PMT update, we should distribute the new CAPMT object to all our connected clients */
			ePtrList<ePMTClient>::iterator client_it = clients.begin();
			for ( ; client_it != clients.end(); ++client_it)
			{
				if (client_it->state() == eSocket::Connection)
				{
					it->writeCAPMTObject(*client_it, LIST_UPDATE);
				}
			}
		}
		else
		{
			/*
			 * this is PMT information for a new service, so we can now distribute
			 * the CAPMT objects to all our connected clients
			 */
			distributeCAPMT();
		}
	}
}
 
CAService::CAService( const eServiceReferenceDVB &service )
	: eUnixDomainSocket(eApp), lastPMTVersion(-1), me(service), capmt(NULL), retry(eApp)
{
	int socketReconnect = 0;
	eConfig::getInstance()->getKey("/elitedvb/extra/cahandlerReconnect", socketReconnect);
	if (socketReconnect)
	{
		CONNECT(connectionClosed_, CAService::connectionLost);
	}
	CONNECT(retry.timeout, CAService::sendCAPMT);
//		eDebug("[eDVBCAHandler] new service %s", service.toString().c_str() );
}

void CAService::connectionLost()
{
	/* reconnect in 1s */
	retry.startLongTimer(1);
}

void CAService::buildCAPMT( PMT *pmt )
{
	if ( !capmt )
		capmt = new unsigned char[1024];

	memcpy(capmt,"\x9f\x80\x32\x82\x00\x00", 6);

	capmt[6]=lastPMTVersion==-1 ? LIST_ONLY : LIST_UPDATE;
	capmt[7]=(unsigned char)((pmt->program_number>>8) & 0xff);			//prg-nr
	capmt[8]=(unsigned char)(pmt->program_number & 0xff);					//prg-nr

	capmt[9]=pmt->version;	//reserved - version - current/next
	capmt[10]=0x00;	//reserved - prg-info len
	capmt[11]=0x00;	//prg-info len

	capmt[12]=CMD_OK_DESCRAMBLING;  // ca pmt command id
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
	if ( eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000 
		|| eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7020 )
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
}

void CAService::sendCAPMT()
{
	if (state() == Idle || state() == Invalid)
	{
		/* we're not connected yet */
		connectToPath(PMT_CLIENT_SOCKET);
	}

	if (state() == Connection)
	{
		/*
		 * Send the CAPMT object which we just constructed, with unmodified list_management field.
		 * This should work in case of a new service, as well as for an updated service.
		 */
		writeCAPMTObject(this, -1);
	}
	else
	{
		/* we're not connected, try again in 5s */
		retry.startLongTimer(5);
	}
}

int CAService::writeCAPMTObject(eSocket *socket, int list_management)
{
	int wp;

	if (!capmt) return 0;

	if (list_management >= 0) capmt[6] = (unsigned char)list_management;

	wp = capmt[4] << 8;
	wp |= capmt[5];
	wp += 6;

	return socket->writeBlock((const char*)capmt, wp);
}

eAutoInitP0<eDVBCAHandler> init_eDVBCAHandler(eAutoInitNumbers::osd-2, "eDVBCAHandler");

#include <errno.h>
#include <stdio.h>
#include "dvb.h"
#include "frontend.h"
#include "si.h"
#include <set>

static struct
{
	int original_network_id, service_id, channel_number;
} defaultChannelNumbers[]=
	{{0x0001, 0x6dca,  1},		// "Das Erste"
	 {0x0001, 0x6d66,  2},		// "ZDF"
	 {0x0001, 0x6e40,  3},		// "N3" *hihi* -> sollte vielleicht raus damit entry #3 nicht blockiert wird.
	 {0x0001, 0x6d67,  4},		// "3sat"
	 {0x0085, 0x0383,  5},		// "KABEL1"
	 {0x0085, 0x002e,  6},		// "Sat.1"
	 {0x0085, 0x0382,  7}, 		// "ProSieben"
	 {0x0001, 0x2ee3,  8}, 		// "RTL"
	 {0x0085, 0x0381,  9},		// "NEUN LIV Television"
	 {0x0085, 0x0034, 10}, 		// "N24"
	 {0x0001, 0x31ba, 11},		// "n-tv"
	 {0x0001, 0x2f1c, 12},		// "VOX"
	 {0x0001, 0x2f08, 13},		// "Super RTL"
	 {0x0001, 0x2ef4, 13},		// "RTL2"
	 {0x0001, 0x31bc, 20},		// "VIVA"
	 {0x0001, 0x2f58, 21},		// "VIVA ZWEI"
	 {0x0001, 0x6fe3, 21}			// "MTV Central"
	};

static int beautifyChannelNumber(int transport_stream_id, int original_network_id, int service_id)
{
	for (unsigned int i=0; i<sizeof(defaultChannelNumbers)/sizeof(*defaultChannelNumbers); i++)
		if ((defaultChannelNumbers[i].original_network_id==original_network_id) &&
				(defaultChannelNumbers[i].service_id==service_id))
			return defaultChannelNumbers[i].channel_number;	
	if ((original_network_id==0x0001) && (service_id>=0x6D80) && (service_id<=0x6F40))		// lame ARD/ZDF heuristic
		return 30;

	if (original_network_id==0x0085)
			/*
				eigentlich mag ich das ja nicht so gerne, weil auf diese weise der komplette kirch-krams oben steht.
				aber anders gehts wohl leider nicht.
			*/
		return 100;
	return -1;
}

#if 0
static std::string escape(const std::string s)
{
	std::string t;
	for (unsigned int i=0; i<s.length(); i++)
	{
		char c = s[i];
		switch (c)
		{
		case '&':
			t+="&amp;";
			break;
		case '\"':
			t+="&quot;";
			break;
		case '<':
			t+="&lt;";
			break;
		default:
			t+=c;
		}
	}
	return t;
}
#endif

void eTransponder::cable::set(const CableDeliverySystemDescriptor *descriptor)
{
	frequency=descriptor->frequency;
	symbol_rate=descriptor->symbol_rate;
	modulation=descriptor->modulation;
	fec_inner=descriptor->FEC_inner;
	inversion=0;
	valid=1;
}

int eTransponder::cable::tune(eTransponder *trans)
{
	eDebug("[TUNE] tuning to %d/%d", frequency, symbol_rate);
	int inv=0;
	return eFrontend::fe()->tune_qam(trans, frequency, symbol_rate, fec_inner, inv, modulation);
}

void eTransponder::satellite::set(const SatelliteDeliverySystemDescriptor *descriptor)
{
	frequency=descriptor->frequency;
	symbol_rate=descriptor->symbol_rate;
	polarisation=descriptor->polarisation;
	fec=descriptor->FEC_inner;
	lnb=0;	// TODO: lookupSatellite(descr->orbital_position, descr->west_east_flag);
	inversion=0;
	valid=1;
}

int eTransponder::satellite::tune(eTransponder *trans)
{
	eDebug("[TUNE] tuning to %d/%d/%s/%d@%d\n", frequency, symbol_rate, polarisation?"V":"H", fec, lnb);
	int inv=0;
	return eFrontend::fe()->tune_qpsk(trans, frequency, polarisation, symbol_rate, fec, inv, lnb);
}

eService::eService(eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id, eServiceID service_id, int service_number):
		transport_stream_id(transport_stream_id), original_network_id(original_network_id), service_id(service_id), service_number(service_number)
{
	clearCache();
}

eService::eService(eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id, const SDTEntry *sdtentry, int service_number):
		transport_stream_id(transport_stream_id), original_network_id(original_network_id), service_number(service_number)
{
	clearCache();
	service_id=sdtentry->service_id;
	update(sdtentry);
}


void eBouquet::add(const eServiceReference &service)
{
	list.push_back(service);
}

int eBouquet::remove(const eServiceReference &service)
{
	list.remove(service);
	return 0;
}

eTransponder::eTransponder(int transport_stream_id, int original_network_id):
	transport_stream_id(transport_stream_id), original_network_id(original_network_id)
{
	cable.valid=0;
	satellite.valid=0;
	state=stateToScan;
}

eTransponder::eTransponder(): transport_stream_id(-1), original_network_id(-1)
{
	cable.valid=satellite.valid=0;
	state=stateToScan;
}

void eTransponder::setSatellite(int frequency, int symbol_rate, int polarisation, int fec, int lnb, int inversion)
{
	satellite.frequency=frequency;
	satellite.symbol_rate=symbol_rate;
	satellite.polarisation=polarisation;
	satellite.fec=fec;
	satellite.lnb=lnb;
	satellite.valid=1;
	satellite.inversion=inversion;
}

void eTransponder::setCable(int frequency, int symbol_rate, int inversion)
{
	cable.frequency=frequency;
	cable.symbol_rate=symbol_rate;
	cable.inversion=inversion;
	cable.valid=1;
}

int eTransponder::tune()
{
	switch (eFrontend::fe()->Type())
	{
	case eFrontend::feCable:
		if (cable.isValid())
			return cable.tune(this);
		else
			return -ENOENT;
	case eFrontend::feSatellite:
		if (satellite.isValid())
			return satellite.tune(this);
		else
			return -ENOENT;
	default:
		return -ENOSYS;
	}
}

int eTransponder::isValid()
{
	switch (eFrontend::fe()->Type())
	{
	case eFrontend::feCable:
		return cable.isValid();
	case eFrontend::feSatellite:
		return satellite.isValid();
	default:
		return 0;
	}
}

void eService::update(const SDTEntry *sdtentry)
{
	if (eServiceID(sdtentry->service_id) != service_id)
	{
		eDebug("tried to update sid %x with sdt-sid %x", service_id.get(), sdtentry->service_id);
		return;
	}
	service_name="unknown";
	service_provider="unknown";
	service_type=0;
	service_id=sdtentry->service_id;
	for (ePtrList<Descriptor>::const_iterator d(sdtentry->descriptors); d != sdtentry->descriptors.end(); ++d)
		if (d->Tag()==DESCR_SERVICE)
		{
			const ServiceDescriptor *nd=(ServiceDescriptor*)*d;
			service_name=nd->service_name;
			service_provider=nd->service_provider;
			service_type=nd->service_type;
		}
//	printf("%04x:%04x %02x %s", transport_stream_id, service_id, service_type, (const char*)service_name);
}

eTransponderList::eTransponderList()
{
}

eTransponder &eTransponderList::createTransponder(int transport_stream_id, int original_network_id)
{
	std::map<tsref,eTransponder>::iterator i=transponders.find(tsref(original_network_id,transport_stream_id));
	if (i==transponders.end())
	{
		i=transponders.insert(
				std::pair<tsref,eTransponder>
					(tsref(original_network_id, transport_stream_id),
						eTransponder(original_network_id,transport_stream_id)
					)
			).first;
		/*emit*/ transponder_added(&(*i).second);
	}
	return (*i).second;
}

eService &eTransponderList::createService(const eServiceReference &service, int chnum, bool *newService)
{
	std::map<eServiceReference,eService>::iterator i=services.find(service);
	if ( i == services.end() )
	{
		if (newService)
			*newService=true;

		if (chnum==-1)
			chnum=beautifyChannelNumber(service.transport_stream_id.get(), service.original_network_id.get(), service.service_id.get());
		
		if (chnum==-1)
		{
			if (channel_number.end()==channel_number.begin())
				chnum=200;
			else
			{
				std::map<int,eService*>::iterator i=channel_number.end();
				--i;
				chnum=i->first+1;	// letzte kanalnummer +1
			}
		}
		
		while (channel_number.find(chnum)!=channel_number.end())
			chnum++;

		eService *n=&services.insert(
					std::pair<eServiceReference,eService>
						(service,
						eService(service.transport_stream_id, service.original_network_id, service.service_id, chnum))
					).first->second;
		channel_number.insert(std::pair<int,eService*>(chnum,n));
		
		return *n;
	}
	return (*i).second;
}

void eTransponderList::updateStats(int &numtransponders, int &scanned, int &nservices)
{
	numtransponders=0;
	scanned=0;
	nservices=services.size();
	for(std::map<tsref,eTransponder>::iterator i(transponders.begin()); i!=transponders.end(); ++i)
	{
		eTransponder &t=i->second;
		if (!t.isValid())
			continue;
		if (t.transport_stream_id==-1)
			continue;
		numtransponders++;
		if (t.state==eTransponder::stateOK)
			scanned++;
	}
}

int eTransponderList::handleSDT(const SDT *sdt)
{
	std::set<eServiceID> s;
	int changed=0;
	bool newAdded;
	for (ePtrList<SDTEntry>::const_iterator i(sdt->entries); i != sdt->entries.end(); ++i)
	{
		int service_type=-1;
		for (ePtrList<Descriptor>::const_iterator d(i->descriptors); d != i->descriptors.end(); ++d)
			if (d->Tag()==DESCR_SERVICE)
			{
				const ServiceDescriptor *nd=(ServiceDescriptor*)*d;
				service_type=nd->service_type;
			}
		if (service_type == -1)
			continue;
		
		eServiceReference sref=
				eServiceReference(
					eTransportStreamID(sdt->transport_stream_id), 
					eOriginalNetworkID(sdt->original_network_id), 
					eServiceID(i->service_id),
					service_type);

		eService &service=createService(sref, newAdded);
		service.update(*i);
		s.insert(eServiceID(i->service_id));

		/*emit*/ service_found(sref, newAdded);
	}
	for (std::map<eServiceReference,eService>::iterator i(services.begin()); i != services.end(); ++i)
		if ((i->first.original_network_id == sdt->original_network_id)	&& // if service on this on
				(i->second.transport_stream_id == sdt->transport_stream_id) && 	// and on this transponder
				(!s.count(i->first.service_id))) // but does not exist
			{
				services.erase(i->first);
				for (std::map<int,eService*>::iterator m(channel_number.begin()); m != channel_number.end(); ++m)
					if (m->second == &i->second)
					{
						channel_number.erase(m);
						break;
					}
				i=services.begin();
				changed=1;
			}
	return changed;
}

eTransponder *eTransponderList::searchTS(eOriginalNetworkID original_network_id, eTransportStreamID transport_stream_id)
{
	std::map<tsref,eTransponder>::iterator i=transponders.find(tsref(original_network_id,transport_stream_id));
	if (i==transponders.end())
		return 0;
	return &i->second;
}

eService *eTransponderList::searchService(const eServiceReference &service)
{
	std::map<eServiceReference,eService>::iterator i=services.find(service);
	if (i==services.end())
		return 0;
	return &i->second;
}

const eServiceReference *eTransponderList::searchService(eOriginalNetworkID original_network_id, eServiceID service_id)
{
	for (std::map<eServiceReference,eService>::iterator i(services.begin()); i != services.end(); ++i)
		if ((i->first.original_network_id == original_network_id) &&
				(i->first.service_id == service_id))
					return &i->first;
	return 0;
}

eService *eTransponderList::searchServiceByNumber(int chnum)
{
	std::map<int,eService*>::iterator i=channel_number.find(chnum);
	if (i==channel_number.end())
		return 0;
	return i->second;
}

eTransponder *eTransponderList::getFirstTransponder(int state)
{
	for (std::map<tsref,eTransponder>::iterator i(transponders.begin()); i!=transponders.end(); ++i)
		if (i->second.state==state)
			return &i->second;
	return 0;
}


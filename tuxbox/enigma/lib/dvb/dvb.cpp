#include <errno.h>
#include <stdio.h>
#include "dvb.h"
#include "frontend.h"
#include "si.h"

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

static void Indent(FILE *out, int ind)
{
	while (ind--)
		fprintf(out, "\t");
}

static QString escape(const QString s)
{
	QString t;
	for (unsigned int i=0; i<s.length(); i++)
	{
		QChar c=s[i];
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

void eTransponder::cable::set(const CableDeliverySystemDescriptor *descriptor)
{
	frequency=descriptor->frequency;
	symbol_rate=descriptor->symbol_rate;
	modulation=descriptor->modulation;
	fec_inner=descriptor->FEC_inner;
	valid=1;
}

int eTransponder::cable::tune(eTransponder *trans)
{
	qDebug("[TUNE] tuning to %d/%d\n", frequency, symbol_rate);
	int inv=0;
	return eFrontend::fe()->tune_qam(trans, frequency, symbol_rate, fec_inner, inv, modulation);
}

void eTransponder::satellite::set(const SatelliteDeliverySystemDescriptor *descriptor)
{
	frequency=descriptor->frequency;
	symbol_rate=descriptor->symbol_rate;
	polarisation=descriptor->polarisation;
	fec=descriptor->FEC_inner;
	sat=0;	// TODO: lookupSatellite(descr->orbital_position, descr->west_east_flag);
	valid=1;
}

int eTransponder::satellite::tune(eTransponder *trans)
{
	qDebug("[TUNE] tuning to %d/%d/%s/%d@%d", frequency, symbol_rate, polarisation?"V":"H", fec, sat);
	int inv=0;
	return eFrontend::fe()->tune_qpsk(trans, frequency, polarisation, symbol_rate, fec, inv, sat);
}

eService::eService(int transport_stream_id, int original_network_id, int service_id, int service_number):
		transport_stream_id(transport_stream_id), original_network_id(original_network_id), service_id(service_id), service_number(service_number)
{
}

eService::eService(int transport_stream_id, int original_network_id, SDTEntry *sdtentry, int service_number):
		transport_stream_id(transport_stream_id), original_network_id(original_network_id), service_number(service_number)
{
	service_id=sdtentry->service_id;
	update(sdtentry);
}

eBouquet::eBouquet(int bouquet_id, QString bouquet_name): bouquet_id(bouquet_id), bouquet_name(bouquet_name)
{
	list.setAutoDelete(true);
}

void eBouquet::add(int transport_stream_id, int original_network_id, int service_id)
{
	remove(transport_stream_id, original_network_id, service_id);
	list.append(new eServiceReference(transport_stream_id, original_network_id, service_id));
}

int eBouquet::remove(int transport_stream_id, int original_network_id, int service_id)
{
	for (QListIterator<eServiceReference> i(list); i.current(); ++i)
		if ((i.current()->transport_stream_id==transport_stream_id) &&
		    (i.current()->original_network_id==original_network_id) &&
		    (i.current()->service_id==service_id))
		{
			list.remove(i.current());
			return 0;
		}
	return -ENOENT;
}

eTransponder::eTransponder(int transport_stream_id, int original_network_id):
	transport_stream_id(transport_stream_id), original_network_id(original_network_id)
{
	cable.valid=0;
	satellite.valid=0;
	state=stateListed;
}

void eTransponder::setSatellite(int frequency, int symbol_rate, int polarisation, int fec, int sat)
{
	satellite.frequency=frequency;
	satellite.symbol_rate=symbol_rate;
	satellite.polarisation=polarisation;
	satellite.fec=fec;
	satellite.sat=sat;
	satellite.valid=1;
}

void eTransponder::setCable(int frequency, int symbol_rate)
{
	cable.frequency=frequency;
	cable.symbol_rate=symbol_rate;
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

void eService::update(SDTEntry *sdtentry)
{
	if (sdtentry->service_id != service_id)
	{
		qDebug("tried to update sid %x with sdt-sid %x", service_id, sdtentry->service_id);
		return;
	}
	service_name="unknown";
	service_provider="unknown";
	service_type=0;
	service_id=sdtentry->service_id;
	for (QListIterator<Descriptor> d(sdtentry->descriptors); d.current(); ++d)
		if (d.current()->Tag()==DESCR_SERVICE)
		{
			ServiceDescriptor *nd=(ServiceDescriptor*)d.current();
			service_name=nd->service_name;
			service_provider=nd->service_provider;
			service_type=nd->service_type;
		}
	qDebug("%04x:%04x %02x %s", transport_stream_id, service_id, service_type, (const char*)service_name);
}

eTransponderList::eTransponderList()
{
	transponders.setAutoDelete(true);
	services.setAutoDelete(true);
	lowest_channelnum=200;
}

eTransponder *eTransponderList::create(int transport_stream_id, int original_network_id)
{
	for (QListIterator<eTransponder> i(transponders); i.current(); ++i)
		if (((i.current()->transport_stream_id)==transport_stream_id) &&
				((i.current()->original_network_id)==original_network_id))
		{
			return i.current();
		}
	eTransponder *n;
	transponders.append(n=new eTransponder(transport_stream_id, original_network_id));
	return n;
}

void eTransponderList::addTransponder(eTransponder *tp)
{
	transponders.append(tp);
}

eService *eTransponderList::createService(int transport_stream_id, int original_network_id, int service_id, int chnum)
{
	for (QListIterator<eService> i(services); i.current(); ++i)
		if (((i.current()->service_id)==service_id) &&
				((i.current()->original_network_id)==original_network_id))
		{
			return i.current();
		}
	if (chnum==-1)
	{
		chnum=beautifyChannelNumber(transport_stream_id, original_network_id, service_id);
		if (chnum==-1)
			chnum=lowest_channelnum++;
		else
			while (searchServiceByNumber(chnum))
				chnum++;
	}
	eService *n;
	services.append(n=new eService(transport_stream_id, original_network_id, service_id, chnum));
	return n;
}

void eTransponderList::updateStats(int &numtransponders, int &scanned, int &nservices)
{
	numtransponders=0;
	scanned=0;
	nservices=services.count();
	for (QListIterator<eTransponder> i(transponders); i.current(); ++i)
	{
		if (!i.current()->isValid())
			continue;
		if (i.current()->transport_stream_id==-1)
			continue;
		numtransponders++;
		if (i.current()->state==eTransponder::stateOK)
			scanned++;
	}
}

void eTransponderList::handleSDT(SDT *sdt)
{
		// todo: remove dead services (clean up current transport_stream)
	for (QListIterator<SDTEntry> i(sdt->entries); i.current(); ++i)
	{
		SDTEntry *entry=i.current();
		eService *service=createService(sdt->transport_stream_id, sdt->original_network_id, entry->service_id);
		service->update(entry);
	}
}

eTransponder *eTransponderList::searchTS(int original_network_id, int transport_stream_id)
{
	for (QListIterator<eTransponder> i(transponders); i.current(); ++i)
		if ((i.current()->original_network_id==original_network_id) &&
				(i.current()->transport_stream_id==transport_stream_id))
			return i.current();
	return 0;
}

eService *eTransponderList::searchService(int original_network_id, int service_id)
{
	for (QListIterator<eService> i(services); i.current(); ++i)
		if ((i.current()->original_network_id==original_network_id) &&
/*				(i.current()->transport_stream_id==transport_stream_id) && */
				(i.current()->service_id==service_id))
			return i.current();
	return 0;
}

eService *eTransponderList::searchServiceByNumber(int channel_number)
{
	for (QListIterator<eService> i(services); i.current(); ++i)
		if (i.current()->service_number==channel_number)
			return i.current();
	return 0;
}

eTransponder *eTransponderList::getFirstTransponder(int state)
{
	for (QListIterator<eTransponder> i(transponders); i.current(); ++i)
		if (i.current()->state==state)
			return i.current();
	return 0;
}


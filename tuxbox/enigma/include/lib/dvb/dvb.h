#ifndef __dvb_h
#define __dvb_h

#include <qstring.h>
#include <qlist.h>
#include <stdio.h>
#include "si.h"

/** reihenfolge: transport_stream_id, original_network_id, service_id
    bei services wird die transport_stream_id evtl. ignoriert */

class eDVB;

class eTransponder
{
public:
	struct cable
	{
		int valid;
		int frequency, symbol_rate;
		int modulation;
		int inversion, fec_inner;
		void set(const CableDeliverySystemDescriptor *descriptor);
		int tune(eTransponder *);
		int isValid() { return valid; }
	} cable;
	struct satellite
	{
		int valid;
		int frequency, symbol_rate, polarisation, fec, inversion, sat;
		void set(const SatelliteDeliverySystemDescriptor *descriptor);
		int tune(eTransponder *);
		int isValid() { return valid; }
	} satellite;
	eTransponder(int transport_stream_id, int original_network_id);
	void setSatellite(SatelliteDeliverySystemDescriptor *descr) { satellite.set(descr); }
	void setCable(CableDeliverySystemDescriptor *descr) { cable.set(descr); }
	void setSatellite(int frequency, int symbol_rate, int polarisation, int fec, int sat);
	void setCable(int frequency, int symbol_rate);
	int tune();
	int isValid(); 
		
	int transport_stream_id, original_network_id;
	enum
	{
		stateListed, stateError, stateOK
	};
	int state;
};

class eService
{
public:
	eService(int transport_stream_id, int original_network_id, SDTEntry *sdtentry, int service_number=-1);
	eService(int transport_stream_id, int original_network_id, int service_id, int service_number=-1);
	void update(SDTEntry *sdtentry);
	
	int transport_stream_id, original_network_id;
	int service_id, service_type;
	
	QString service_name, service_provider;
	
	int service_number;		// gleichzeitig sortierkriterium.
};

struct eServiceReference
{
	int transport_stream_id, original_network_id, service_id;
	
	eService *service;
	eServiceReference(int transport_stream_id, int original_network_id, int service_id):
		transport_stream_id(transport_stream_id), original_network_id(original_network_id), service_id(service_id)
	{
		service=0;
	}
};

class eBouquet
{
public:
	eBouquet(int bouquet_id, QString bouquet_name);
	void add(int transport_stream_id, int original_network_id, int service_id);
	int remove(int transport_stream_id, int original_network_id, int service_id);

	int bouquet_id;
	QString bouquet_name;
	
	QList<eServiceReference> list;
};

class eTransponderList
{
	QList<eTransponder> transponders;
	QList<eService> services;
	int lowest_channelnum;
public:
	eTransponderList();

	void updateStats(int &transponders, int &scanned, int &services);
	eTransponder *create(int transport_stream_id, int original_network_id);
	void addTransponder(eTransponder *transponder);
	eService *createService(int transport_stream_id, int original_network_id, int service_id, int service_number=-1);
	void handleSDT(SDT *sdt);

	QList<eService> *getServices() { return &services; }
	QList<eTransponder> *getTransponders() { return &transponders; }
	
	void serialize(FILE *out, int ind);
	eTransponder *searchTS(int original_network_id, int transport_stream_id);
	eService *searchService(int original_network_id, int service_id);
	eService *searchServiceByNumber(int channel_number);

	eTransponder *getFirstTransponder(int state);
};

#endif

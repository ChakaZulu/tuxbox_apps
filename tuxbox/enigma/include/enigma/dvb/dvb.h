#ifndef __dvb_h
#define __dvb_h

#include <qstring.h>
#include <qlist.h>
#include <stdio.h>
#include "si.h"

/** reihenfolge: transport_stream_id, original_network_id, service_id
    bei services wird die transport_stream_id evtl. ignoriert */

class eDVB;

#include <map>
#include <utility>
#include <functional>

struct tsref: public std::pair<int,int>
{
	bool operator<(const tsref &c)
	{
		if (first < c.first)
			return 1;
		if (second < c.second)
			return 1;
		return 0;
	}
	tsref(int a, int b): std::pair<int,int>(a,b)
	{
	}
};

struct sref: public std::pair<int,int>
{
	bool operator<(const sref &c)
	{
		if (first < c.first)
			return 1;
		if (second < c.second)
			return 1;
		return 0;
	}
	sref(int a, int b): std::pair<int,int>(a,b)
	{
	}
};

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
	
	void set(const eTransponder &ref)
	{
		cable=ref.cable;
		satellite=ref.satellite;
		state=ref.state;
	}
	int tune();
	int isValid(); 
		
	int transport_stream_id, original_network_id;
	enum
	{
		stateListed, stateError, stateOK
	};
	int state;
	
	bool operator<(const eTransponder &c) const
	{
		if (original_network_id < c.original_network_id)
			return 1;
		if (transport_stream_id < c.transport_stream_id)
			return 1;
		return 0;
	}

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
	
	bool operator<(const eService &c) const
	{
		if (original_network_id < c.original_network_id)
			return 1;
		if (service_id < c.service_id)
			return 1;
		return 0;
	}
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
	std::map<tsref,eTransponder> transponders;
	std::map<sref,eService> services;
	int lowest_channelnum;

public:
	eTransponderList();

	void updateStats(int &transponders, int &scanned, int &services);
	eTransponder &createTransponder(int transport_stream_id, int original_network_id);
	eService &createService(int transport_stream_id, int original_network_id, int service_id, int service_number=-1);
	void handleSDT(SDT *sdt);

	void serialize(FILE *out, int ind);
	eTransponder *searchTS(int original_network_id, int transport_stream_id);
	eService *searchService(int original_network_id, int service_id);
	eService *searchServiceByNumber(int channel_number);
	
	template <class T> void forEachService(T ob) { for_each(services.begin(), services.end(), ob); }
	template <class T> void forEachTransponder(T ob) { for_each(transponders.begin(), transponders.end(), ob); }

	eTransponder *getFirstTransponder(int state);
};

#endif

#ifndef __dvb_h
#define __dvb_h

#include <stdio.h>
#include "si.h"

/** reihenfolge: transport_stream_id, original_network_id, service_id
    bei services wird die transport_stream_id evtl. ignoriert */

class eDVB;

#include <list>
#include <map>
#include <utility>
#include <functional>
#include <string>

#define ServiceReferenceIterator std::list<eServiceReference>::iterator

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
	
	std::string service_name, service_provider;
	
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
	bool operator==(const eServiceReference &c) const
	{
		return (transport_stream_id==c.transport_stream_id) && (original_network_id==c.original_network_id) && (service_id==c.service_id);
	}
};

class eBouquet
{
public:
	const eBouquet *parent;
	int bouquet_id;
	eString bouquet_name;
	std::list<eServiceReference> list;

	inline eBouquet(const eBouquet *parent, int bouquet_id, eString& bouquet_name)
		:parent(parent), bouquet_id(bouquet_id), bouquet_name(bouquet_name)
	{
	}

	void add(int transport_stream_id, int original_network_id, int service_id);
	int remove(int transport_stream_id, int original_network_id, int service_id);
	bool operator == (const eBouquet &c) const
	{
		return bouquet_id==c.bouquet_id;
	}
	bool operator < (const eBouquet &c) const
	{
		return (bouquet_name.compare(c.bouquet_name));
	}
};

class eTransponderList
{
	std::map<tsref,eTransponder> transponders;
	std::map<sref,eService> services;
	std::map<int,eService*> channel_number;

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
	
	template <class T> 
	void forEachService(T ob)
	{
		for (std::map<sref,eService>::iterator i(services.begin()); i!=services.end(); ++i)
			ob(i->second);
	}
	template <class T> void forEachTransponder(T ob)
	{
		for (std::map<tsref,eTransponder>::iterator i(transponders.begin()); i!=transponders.end(); ++i)
			ob(i->second);
	}
	template <class T> void forEachChannel(T ob)
	{
		for (std::map<int,eService*>::iterator i(channel_number.begin()); i!=channel_number.end(); ++i)
			ob(*i->second);
	}

	eTransponder *getFirstTransponder(int state);
};

#endif

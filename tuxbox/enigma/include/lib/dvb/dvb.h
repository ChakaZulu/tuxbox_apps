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
		else if (first == c.first)
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
		else if (first == c.first)			
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
		bool operator == (const cable &c) const
		{
			if (valid != c.valid)
				return 0;
			if (frequency != c.frequency)
				return 0;
			if (symbol_rate != c.symbol_rate)
				return 0;
			if (modulation != c.modulation)
				return 0;
			if (inversion != c.inversion)
				return 0;
			if (fec_inner != c.fec_inner)
				return 0;
			return 1;
		}
	} cable;
	struct satellite
	{
		int valid;
		int frequency, symbol_rate, polarisation, fec, inversion, lnb;
		void set(const SatelliteDeliverySystemDescriptor *descriptor);
		int tune(eTransponder *);
		int isValid() { return valid; }
		bool operator == (const satellite &c) const
		{
			if (valid != c.valid)
				return 0;
			if (frequency != c.frequency)
				return 0;
			if (symbol_rate != c.symbol_rate)
				return 0;
			if (polarisation != c.polarisation)
				return 0;
			if (fec != c.fec)
				return 0;
			if (inversion != c.inversion)
				return 0;
			if (lnb != c.lnb)
				return 0;
			return 1;
		}
	} satellite;
	eTransponder(int transport_stream_id, int original_network_id);
	eTransponder();
	void setSatellite(SatelliteDeliverySystemDescriptor *descr) { satellite.set(descr); }
	void setCable(CableDeliverySystemDescriptor *descr) { cable.set(descr); }
	void setSatellite(int frequency, int symbol_rate, int polarisation, int fec, int lnb, int inversion);
	void setCable(int frequency, int symbol_rate, int inversion);
	
	eTransponder &operator=(const eTransponder &ref)
	{
		cable=ref.cable;
		satellite=ref.satellite;
		state=ref.state;
		transport_stream_id=ref.transport_stream_id;
		original_network_id=ref.original_network_id;
		return *this;
	}
	int tune();
	int isValid(); 
		
	int transport_stream_id, original_network_id;
	enum
	{
		stateToScan, stateError, stateOK
	};
	int state;
	
	bool operator==(const eTransponder &c) const
	{
		if (original_network_id != c.original_network_id)
			return 0;
		if (transport_stream_id != c.transport_stream_id)
			return 0;
		if ((original_network_id == -1) && (transport_stream_id == -1))	// yet unnamed transponder. compare settings.
		{
			if (satellite.valid && c.satellite.valid)
				return satellite == c.satellite;
			if (cable.valid && c.cable.valid)
				return cable == c.cable;
			return 1;
		}
		return 1;
	}

	bool operator<(const eTransponder &c) const
	{
		if ((original_network_id == -1) && (transport_stream_id == -1))
		{
			if ((c.original_network_id == -1) && (c.transport_stream_id == -1))
				return this < &c;
			else
				return 1;
		}

		if (original_network_id < c.original_network_id)
			return 1;
		else if (original_network_id == c.original_network_id)
			if (transport_stream_id < c.transport_stream_id)
				return 1;
		return 0;
	}

};

class eService
{
public:
	enum cacheID
	{
		cVPID, cAPID, cTPID, cPCRPID, cacheMax
	};
	eService(int transport_stream_id, int original_network_id, const SDTEntry *sdtentry, int service_number=-1);
	eService(int transport_stream_id, int original_network_id, int service_id, int service_number=-1);
	void update(const SDTEntry *sdtentry);
	
	int transport_stream_id, original_network_id;
	int service_id, service_type;
	
	std::string service_name, service_provider;
	
	int service_number;		// gleichzeitig sortierkriterium.
	
	int cache[cacheMax];
	
	void set(cacheID c, int v)
	{
		cache[c]=v;
	}
	
	int get(cacheID c)
	{
		return cache[c];
	}
	
	void clearCache()
	{
		for (int i=0; i<cacheMax; i++)
			cache[i]=-1;
	}
	
	bool operator<(const eService &c) const
	{
		if (original_network_id < c.original_network_id)
			return 1;
		else if (original_network_id == c.original_network_id)
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
	eService &createService(int transport_stream_id, int original_network_id, int service_id, int service_number = -1, bool* newService=0);
	int handleSDT(const SDT *sdt);
	Signal1<void, eTransponder*> transponder_added;
	Signal2<void, eService*, bool> service_found;

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

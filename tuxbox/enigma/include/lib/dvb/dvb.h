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
#include <set>
#include <stack>

#ifndef MIN
	#define MIN(a,b) (a < b ? a : b)
#endif

#ifndef MAX
	#define MAX(a,b) (a > b ? a : b)
#endif

#define MAXDIFF(a,b) (MAX(a,b)-MIN(a,b))

class eTransponderList;
class eServiceReference;
class eLNB;
class eSatellite;

		// bitte KEINE operator int() definieren, sonst bringt das ganze nix!
struct eTransportStreamID
{
private:
	int v;
public:
	int get() const { return v; }
	eTransportStreamID(int i): v(i) { }
	eTransportStreamID(): v(-1) { }
	bool operator == (const eTransportStreamID &c) const { return v == c.v; }
	bool operator != (const eTransportStreamID &c) const { return v != c.v; }
	bool operator < (const eTransportStreamID &c) const { return v < c.v; }
	bool operator > (const eTransportStreamID &c) const { return v > c.v; }
};

struct eServiceID
{
private:
	int v;
public:
	int get() const { return v; }
	eServiceID(int i): v(i) { }
	eServiceID(): v(-1) { }
	bool operator == (const eServiceID &c) const { return v == c.v; }
	bool operator != (const eServiceID &c) const { return v != c.v; }
	bool operator < (const eServiceID &c) const { return v < c.v; }
	bool operator > (const eServiceID &c) const { return v > c.v; }
};

struct eOriginalNetworkID
{
private:
	int v;
public:
	int get() const { return v; }
	eOriginalNetworkID(int i): v(i) { }
	eOriginalNetworkID(): v(-1) { }
	bool operator == (const eOriginalNetworkID &c) const { return v == c.v; }
	bool operator != (const eOriginalNetworkID &c) const { return v != c.v; }
	bool operator < (const eOriginalNetworkID &c) const { return v < c.v; }
	bool operator > (const eOriginalNetworkID &c) const { return v > c.v; }
};

struct tsref: public std::pair<eTransportStreamID,eOriginalNetworkID>
{
	bool operator<(const tsref &c)
	{
		if (second < c.second)
			return 1;
		else if (second == c.second)
			if (first < c.first)
				return 1;
		return 0;
	}
	tsref(eTransportStreamID tsid, eOriginalNetworkID onid): std::pair<eTransportStreamID,eOriginalNetworkID>(tsid,onid)
	{
	}
};

class eTransponder
{
	eTransponderList &tplist;
//	friend struct eTransponder::satellite;
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
		int frequency, symbol_rate, polarisation, fec, inversion, orbital_position;
		void set(const SatelliteDeliverySystemDescriptor *descriptor);
		int tune(eTransponder *);
		int isValid() { return valid; }
		bool operator == (const satellite &c) const
		{
			if (valid != c.valid)
				return 0;
//   		eDebug("frequency %i - %i = %i", frequency, c.frequency, MAXDIFF(frequency,c.frequency) );
			if ( MAXDIFF(frequency,c.frequency) > 1000 )
				return 0;
//   		eDebug("symbol_rate -> %i != %i", symbol_rate, c.symbol_rate );
			if (symbol_rate != c.symbol_rate)
				return 0;
//   		eDebug("polarisation -> %i != %i", polarisation, c.polarisation );
			if (polarisation != c.polarisation)
				return 0;
//   		eDebug("fec -> %i != %i", fec, c.fec );
			if (fec != c.fec)
				return 0;
//   		eDebug("inversion -> %i != %i", inversion, c.inversion );
			if (inversion != c.inversion)
				return 0;
//			eDebug("orbital_position -> %i != %i", orbital_position, c.orbital_position);
			if (orbital_position != c.orbital_position)
				return 0;
//			eDebug("Satellite Data is equal");
			return 1;
		}
	} satellite;
	eTransponder(eTransponderList &tplist, eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id);
	eTransponder(eTransponderList &tplist);
	void setSatellite(SatelliteDeliverySystemDescriptor *descr) { satellite.set(descr); }
	void setCable(CableDeliverySystemDescriptor *descr) { cable.set(descr); }
	void setSatellite(int frequency, int symbol_rate, int polarisation, int fec, int orbital_position, int inversion);
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
		
	eTransportStreamID transport_stream_id;
	eOriginalNetworkID original_network_id;
	enum
	{
		stateToScan, stateError, stateOK
	};
	int state;
	
	bool operator==(const eTransponder &c) const
	{
//		eDebug("onid = %i, c.onid = %i, tsid = %i, c.tsid = %i", original_network_id.get(), transport_stream_id.get(), c.original_network_id.get(), c.transport_stream_id.get() );
		if ( original_network_id != -1 && c.original_network_id != -1 && transport_stream_id != -1 && c.transport_stream_id != -1)
//		{
//	  	eDebug("TSID / ONID Vergleich");
			return ( (original_network_id == c.original_network_id) && (transport_stream_id == c.transport_stream_id) );
//		}
		else
		{
			if (satellite.valid && c.satellite.valid)
				return satellite == c.satellite;
			if (cable.valid && c.cable.valid)
				return cable == c.cable;
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
	eService(eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id, const SDTEntry *sdtentry/*, int service_number=-1*/);
	eService(eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id, eServiceID service_id/*, int service_number=-1*/);
	eService(eServiceID service_id, const char *name);
	void update(const SDTEntry *sdtentry);
	
	eTransportStreamID transport_stream_id;
	eOriginalNetworkID original_network_id;
	eServiceID service_id;
	int service_type;
	
	std::string service_name, service_provider;
	
//	int service_number;		// gleichzeitig sortierkriterium.
	
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
	enum
	{
		idInvalid=-1,
		idStructure,	// service_id == 0 is root
		idDVB,
		idFile,
		idUser=0x1000
	};
	int type;

	eString descr;

	int flags;
	enum
	{
		isDirectory=1,		// SHOULD enter  (implies mustDescent)
		mustDescent=2,		// cannot be played directly - often used with "isDirectory" (implies canDescent)
		/*
			for example:
				normal services have none of them - they can be fed directly into the "play"-handler.
				normal directories have both of them set - you cannot play a directory directly and the UI should descent into it.
				playlists have "mustDescent", but not "isDirectory" - you don't want the user to browse inside the playlist (unless he really wants)
				services with sub-services have none of them, instead the have the "canDecsent" flag (as all of the above)
		*/
		canDescent=4,			// supports enterDirectory/leaveDirectory
		flagDirectory=isDirectory|mustDescent|canDescent,
		shouldSort=8,			// should be ASCII-sorted according to service_name. great for directories.
		hasSortKey=16,		// has a sort key in data[3]. not having a sort key implies 0.
		sort1=32					// sort key is 1 instead of 0
	};

	inline int getSortKey() const { return (flags & hasSortKey) ? data[3] : ((flags & sort1) ? 1 : 0); }

	int data[4];
	eString path;

	eServiceReference()
		: type(idInvalid), flags(0)
	{
	}

	eServiceReference(int type, int flags)
		: type(type), flags(flags)
	{
		data[0]=data[1]=data[2]=data[3]=0;
	}
	eServiceReference(int type, int flags, int data0)
		: type(type), flags(flags)
	{
		data[0]=data0;
		data[1]=data[2]=data[3]=0;
	}
	eServiceReference(int type, int flags, int data0, int data1)
		: type(type), flags(flags)
	{
		data[0]=data0;
		data[1]=data1;
		data[2]=data[3]=0;
	}
	eServiceReference(int type, int flags, int data0, int data1, int data2)
		: type(type), flags(flags)
	{
		data[0]=data0;
		data[1]=data1;
		data[2]=data2;
		data[3]=0;
	}
	eServiceReference(int type, int flags, int data0, int data1, int data2, int data3)
		: type(type), flags(flags)
	{
		data[0]=data0;
		data[1]=data1;
		data[2]=data2;
		data[3]=data3;
	}
	eServiceReference(int type, int flags, const eString &path)
		: type(type), flags(flags), path(path)
	{
		data[0]=data[1]=data[2]=data[3]=0;
	}
	eServiceReference(const eString &string);
	eString toString() const;
	bool operator==(const eServiceReference &c) const
	{
		if (type != c.type)
			return 0;
		return (flags == c.flags) && (memcmp(data, c.data, sizeof(int)*4)==0) && (path == c.path);
	}
	bool operator!=(const eServiceReference &c) const
	{
		return !(*this == c);
	}
	bool operator<(const eServiceReference &c) const
	{
		if (type < c.type)
			return 1;

		if (type > c.type)
			return 0;
			
		if (flags < c.flags)
			return 1;
		if (flags > c.flags)
			return 0;

		int r=memcmp(data, c.data, sizeof(int)*4);
		if (r)
			return r < 0;
		return path < c.path;
	}
	operator bool() const
	{
		return type != idInvalid;
	}
};

class eServicePath
{
	std::stack<eServiceReference> path;
public:
	eServicePath()	{	}
	eServicePath( const eString& data );
	eServicePath(const eServiceReference &ref);
	void setString( const eString& data );
	eString toString();
	bool up();
	void down(const eServiceReference &ref);
	eServiceReference current() const;
};

struct eServiceReferenceDVB: public eServiceReference
{
	int getServiceType() const { return data[0]; }
	void setServiceType(int service_type) { data[0]=service_type; }

	eServiceID getServiceID() const { return eServiceID(data[1]); }
	void setServiceID(eServiceID service_id) { data[1]=service_id.get(); }

	eTransportStreamID getTransportStreamID() const { return eTransportStreamID(data[2]); }
	void setTransportStreamID(eTransportStreamID transport_stream_id) { data[2]=transport_stream_id.get(); }

	eOriginalNetworkID getOriginalNetworkID() const { return eOriginalNetworkID(data[3]); }
	void setOriginalNetworkID(eOriginalNetworkID original_network_id) { data[3]=original_network_id.get(); }

	eServiceReferenceDVB(eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id, eServiceID service_id, int service_type)
		:eServiceReference(eServiceReference::idDVB, 0)
	{
		setTransportStreamID(transport_stream_id);
		setOriginalNetworkID(original_network_id);
		setServiceID(service_id);
		setServiceType(service_type);
	}

	eServiceReferenceDVB()
	{
	}
};

class eBouquet
{
public:
	const eBouquet *parent;
	int bouquet_id;
	eString bouquet_name;
	std::list<eServiceReferenceDVB> list;

	inline eBouquet(const eBouquet *parent, int bouquet_id, eString& bouquet_name)
		:parent(parent), bouquet_id(bouquet_id), bouquet_name(bouquet_name)
	{
	}

	void add(const eServiceReferenceDVB &);
	int remove(const eServiceReferenceDVB &);
	bool operator == (const eBouquet &c) const
	{
		return bouquet_id==c.bouquet_id;
	}
	bool operator < (const eBouquet &c) const
	{
		return (bouquet_name.compare(c.bouquet_name));
	}
};

struct eSwitchParameter
{
	enum SIG22	{	HILO=0, ON=1, OFF=2	}; // 22 Khz
	enum VMODE	{	HV=0, _14V=1, _18V=2 }; // 14/18 V
	VMODE VoltageMode;
	SIG22 HiLoSignal;
};

class eSatellite
{
	eTransponderList &tplist;
	int orbital_position;
	eString description;
	eSwitchParameter switchParams;
	eLNB *lnb;
	std::map<int, eSatellite*>::iterator tpiterator;
	friend class eLNB;
public:
	eSatellite(eTransponderList &tplist, int orbital_position, eLNB &lnb);
	~eSatellite();
	
	const eString &getDescription() const
	{
		return description;
	}
	
	void setDescription(const eString &description)
	{
		this->description=description;
	}
	
	int getOrbitalPosition() const
	{
		return orbital_position;
	}

	eSwitchParameter &getSwitchParams()
	{
		return switchParams;
	}	

	eLNB *getLNB() const
	{
		return lnb;
	}

	void setLNB( eLNB* _lnb )
	{
		lnb = _lnb;
	}
	
	void setOrbitalPosition(int orbital_position);

	bool operator<(const eSatellite &sat) const
	{
		return orbital_position < sat.orbital_position;
	}

	bool operator==(const eSatellite &sat) const
	{
		return orbital_position == sat.orbital_position;
	}
};

struct eDiSEqC
{
	enum tDiSEqCParam	{	AA=0, AB=1, BA=2, BB=3, USER=4 }; // DiSEqC Parameter
	enum tDiSEqCMode	{	MINI=0, V1_0=1, V1_1=2, V1_2=3 }; // DiSEqC Mode
	tDiSEqCParam DiSEqCParam;
	tDiSEqCMode DiSEqCMode;
};

class eLNB
{
	unsigned int lof_hi, lof_lo, lof_threshold;
	ePtrList<eSatellite> satellites;
	eTransponderList &tplist;
	eDiSEqC DiSEqC;
public:

	eLNB(eTransponderList &tplist): tplist(tplist)
	{
		satellites.setAutoDelete(true);
	}
	
	void setLOFHi(unsigned int lof_hi) { this->lof_hi=lof_hi; }
	void setLOFLo(unsigned int lof_lo) { this->lof_lo=lof_lo; }
	void setLOFThreshold(unsigned int lof_threshold) { this->lof_threshold=lof_threshold; }
	unsigned int getLOFHi() const { return lof_hi; }
	unsigned int getLOFLo() const { return lof_lo; }
	unsigned int getLOFThreshold() const { return lof_threshold; }
	eDiSEqC& getDiSEqC() { return DiSEqC; }	
	eSatellite *addSatellite(int orbital_position);
	void deleteSatellite(eSatellite *satellite);
	void addSatellite( eSatellite *satellite);
	eSatellite* takeSatellite( eSatellite *satellite);
	bool operator==(const eLNB& lnb) { return this == &lnb; }
	ePtrList<eSatellite> &getSatelliteList() { return satellites; }
};

class eTransponderList
{
	static eTransponderList* instance;
	std::map<tsref,eTransponder> transponders;
	std::map<eServiceReferenceDVB,eService> services;
	
	std::map<int,eSatellite*> satellites;
	std::list<eLNB> lnbs;
	friend class eLNB;
	friend class eSatellite;
public:
	void clearServices()	{	services.clear(); }
	void clearTransponders()	{	transponders.clear(); }

	static eTransponderList* getInstance()	{ return instance; }
	eTransponderList();

	~eTransponderList()
	{
		writeLNBData();  // write Data to registry

		if (instance == this)
			instance = 0;
	}

	void readLNBData();
	void writeLNBData();

	eTransponder &createTransponder(eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id);
	eService &createService(const eServiceReferenceDVB &service/*, int service_number=-1*/, bool *newService=0);
	int handleSDT(const SDT *sdt);
	Signal1<void, eTransponder*> transponder_added;
	Signal2<void, const eServiceReferenceDVB &, bool> service_found;

	eTransponder *searchTS(eTransportStreamID transport_stream_id, eOriginalNetworkID original_network_id);
	eService *searchService(const eServiceReference &service);
	const eServiceReferenceDVB *searchService(eOriginalNetworkID original_network_id, eServiceID service_id);
//	eService *searchServiceByNumber(int channel_number);
	
	template <class T> 
	void forEachService(T ob)
	{
		for (std::map<eServiceReferenceDVB,eService>::iterator i(services.begin()); i!=services.end(); ++i)
			ob(i->second);
	}
	template <class T> 
	void forEachServiceReference(T ob)
	{
		for (std::map<eServiceReferenceDVB,eService>::iterator i(services.begin()); i!=services.end(); ++i)
			ob(i->first);
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
	eSatellite *findSatellite(int orbital_position);
	std::list<eLNB>& getLNBs()	{	return lnbs;	}
};

#endif

#ifndef __scan_h__
#define __scan_h__


#include <config.h>
#include "zapitclient.h"
#include "eventserver.h"
#include "getservices.h"

struct scanchannel{
	std::string name;
	int sid;
	int tsid;
	int service_type;
	int pmt;
	int onid;

	scanchannel(std::string Name, int Sid, int Tsid,int Onid, int Service_type)
	{
		name = Name;
		sid = Sid;
		tsid = Tsid;
		onid = Onid;
		service_type = Service_type;
		pmt = 0;
	}
	scanchannel(std::string Name, int Sid, int Tsid,int Onid)
	{
		name = Name;
		sid = Sid;
		tsid = Tsid;
		onid = Onid;
		service_type = 1;
		pmt = 0;
	}

	scanchannel(int Sid, int Tsid, int Pmt)
	{
		sid = Sid;
		tsid = Tsid;
		pmt = Pmt;
		onid = 0;
		service_type = 0;
	}
};

struct transpondermap
{
	uint16_t transport_stream_id;
	FrontendParameters feparams;
	uint8_t polarization;
	uint8_t DiSEqC;

	transpondermap(uint16_t p_transport_stream_id, FrontendParameters p_feparams)
	{
		transport_stream_id = p_transport_stream_id;
		feparams = p_feparams;
		polarization = 0;
		DiSEqC = 0;
	}

	transpondermap(uint16_t p_transport_stream_id, FrontendParameters p_feparams, uint8_t p_polarization, uint8_t p_DiSEqC)
	{
		transport_stream_id = p_transport_stream_id;
		feparams = p_feparams;
		polarization = p_polarization;
		DiSEqC = p_DiSEqC;
	}
};

struct bouquet_mulmap
{
	std::string provname;
	std::string servname;
	int sid;
	int onid;

	bouquet_mulmap(std::string Provname, std::string Servname, int Sid, int Onid)
	{
		provname = Provname;
		servname = Servname;
		sid = Sid;
		onid = Onid;
	}
};

extern std::map<int, transpondermap> scantransponders;
extern std::map<int, scanchannel> scanchannels;
extern std::multimap<std::string, bouquet_mulmap> scanbouquets;

#endif /* __scan_h__ */

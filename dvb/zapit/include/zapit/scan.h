
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
} ;

struct transpondermap
{
	int tsid;
	int freq;
	int symbolrate;
	int fec_inner;
	int polarization;
	int diseqc;
	
	
	transpondermap(int Tsid, int Freq, int Symbolrate, int Fec_inner)
	{
		tsid = Tsid;
		freq = Freq;
		symbolrate = Symbolrate;
		fec_inner = Fec_inner;
		polarization = 0;
		diseqc=0;
	}
	
	transpondermap(int Tsid, int Freq, int Symbolrate, int Fec_inner,int Polarization,int Diseqc)
	{
		tsid = Tsid;
		freq = Freq;
		symbolrate = Symbolrate;
		fec_inner = Fec_inner;
		polarization = Polarization;
		diseqc = Diseqc;
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


// #define NVOD_HACK

extern std::map<int, transpondermap> scantransponders;
extern std::map<int, scanchannel> scanchannels;
extern std::multimap<std::string, bouquet_mulmap> scanbouquets;

#include <lib/dvb/si.h>

#include <stdio.h>
#include <time.h>
#include <sstream>
#include <iomanip>

extern "C"
{
	time_t my_mktime (struct tm *tp);
}
#define HILO(x) (x##_hi << 8 | x##_lo) 
#include <lib/dvb/lowlevel/decode.h>
#include <lib/dvb/lowlevel/dvb.h>
#include <lib/dvb/lowlevel/pat.h>
#include <lib/dvb/lowlevel/tdt.h>

static eString qHex(int v)
{
	return eString().sprintf("%04x", v);
}

int fromBCD(int bcd)
{
	if ((bcd&0xF0)>=0xA0)
		return -1;
	if ((bcd&0xF)>=0xA0)
		return -1;
	return ((bcd&0xF0)>>4)*10+(bcd&0xF);
}

time_t parseDVBtime(__u8 t1, __u8 t2, __u8 t3, __u8 t4, __u8 t5)
{
	tm t;
	t.tm_sec=fromBCD(t5);
	t.tm_min=fromBCD(t4);
	t.tm_hour=fromBCD(t3);
	int mjd=(t1<<8)|t2;
	int k;

	t.tm_year = (int) ((mjd - 15078.2) / 365.25);
	t.tm_mon = (int) ((mjd - 14956.1 - (int)(t.tm_year * 365.25)) / 30.6001);
	t.tm_mday = (int) (mjd - 14956 - (int)(t.tm_year * 365.25) - (int)(t.tm_mon * 30.6001));
	k = (t.tm_mon == 14 || t.tm_mon == 15) ? 1 : 0;
	t.tm_year = t.tm_year + k;
	t.tm_mon = t.tm_mon - 1 - k * 12;
	t.tm_mon--;

	t.tm_isdst =  0;
	t.tm_gmtoff = 0;

//	return timegm(&t);
	return my_mktime(&t)-timezone;
}
		// 8859-x to dvb coding tables. taken from www.unicode.org/Public/MAPPINGS/ISO8859/

unsigned long c88595[128]={
0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087, 0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f, 
0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097, 0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f, 
0x00a0, 0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0406, 0x0407, 0x0408, 0x0409, 0x040a, 0x040b, 0x040c, 0x00ad, 0x040e, 0x040f, 
0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, 0x041f, 
0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042a, 0x042b, 0x042c, 0x042d, 0x042e, 0x042f, 
0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, 0x043f, 
0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044a, 0x044b, 0x044c, 0x044d, 0x044e, 0x044f, 
0x2116, 0x0451, 0x0452, 0x0453, 0x0454, 0x0455, 0x0456, 0x0457, 0x0458, 0x0459, 0x045a, 0x045b, 0x045c, 0x00a7, 0x045e, 0x045f};

unsigned long c88596[128]={
0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087, 0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f, 
0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097, 0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f, 
0x00a0, 0x0000, 0x0000, 0x0000, 0x00a4, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x060c, 0x00ad, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x061b, 0x0000, 0x0000, 0x0000, 0x061f, 
0x0000, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627, 0x0628, 0x0629, 0x062a, 0x062b, 0x062c, 0x062d, 0x062e, 0x062f, 
0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637, 0x0638, 0x0639, 0x063a, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647, 0x0648, 0x0649, 0x064a, 0x064b, 0x064c, 0x064d, 0x064e, 0x064f, 
0x0650, 0x0651, 0x0652, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};

unsigned long c88597[128]={
0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087, 0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f, 
0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097, 0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f, 
0x00a0, 0x2018, 0x2019, 0x00a3, 0x0000, 0x0000, 0x00a6, 0x00a7, 0x00a8, 0x00a9, 0x0000, 0x00ab, 0x00ac, 0x00ad, 0x0000, 0x2015, 
0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x0384, 0x0385, 0x0386, 0x00b7, 0x0388, 0x0389, 0x038a, 0x00bb, 0x038c, 0x00bd, 0x038e, 0x038f, 
0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e, 0x039f, 
0x03a0, 0x03a1, 0x0000, 0x03a3, 0x03a4, 0x03a5, 0x03a6, 0x03a7, 0x03a8, 0x03a9, 0x03aa, 0x03ab, 0x03ac, 0x03ad, 0x03ae, 0x03af, 
0x03b0, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7, 0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf, 
0x03c0, 0x03c1, 0x03c2, 0x03c3, 0x03c4, 0x03c5, 0x03c6, 0x03c7, 0x03c8, 0x03c9, 0x03ca, 0x03cb, 0x03cc, 0x03cd, 0x03ce, 0x0000};

unsigned long c88598[128]={
0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087, 0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f, 
0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097, 0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f, 
0x00a0, 0x0000, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7, 0x00a8, 0x00a9, 0x00d7, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af, 
0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00f7, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2017, 
0x05d0, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5, 0x05d6, 0x05d7, 0x05d8, 0x05d9, 0x05da, 0x05db, 0x05dc, 0x05dd, 0x05de, 0x05df, 
0x05e0, 0x05e1, 0x05e2, 0x05e3, 0x05e4, 0x05e5, 0x05e6, 0x05e7, 0x05e8, 0x05e9, 0x05ea, 0x0000, 0x0000, 0x200e, 0x200f, 0x0000};

unsigned long c88599[128]={
0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087, 0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f, 
0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097, 0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f, 
0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7, 0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af, 
0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf, 
0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf, 
0x011e, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7, 0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x0130, 0x015e, 0x00df, 
0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7, 0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef, 
0x011f, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x0131, 0x015f, 0x00ff};


unsigned int recode(unsigned char d, int cp)
{
	if (d < 0x80)
		return d;
	switch (cp)
	{
	case 0:		// 8859-1 Latin1 <-> unicode mapping
		return d;
	case 1:		// 8859-5 -> unicode mapping
		return c88595[d-0x80];
	case 2:		// 8859-6 -> unicode mapping
		return c88596[d-0x80];
	case 3:		// 8859-7 -> unicode mapping
		return c88597[d-0x80];
	case 4:		// 8859-8 -> unicode mapping
		return c88598[d-0x80];
	case 5:		// 8859-9 -> unicode mapping
		return c88599[d-0x80];
	default:
		return d;
	}
}

static eString convertDVBUTF8(unsigned char *data, int len)
{
	int table=5;
	int i;
	if (!len)
		return "";
		
	i=0;
	if (data[0] <= 5)
		table=data[i++];
	if ((data[0] >= 0x10) && (data[0] <= 0x12))
		return "<unsupported encoding>";

	int bytesneeded=0, t=0, s=i;
	
	for (; i<len; ++i)
	{
		unsigned long code=recode(data[i], table);
		if (!code)
			continue;
		if (code >= 0x10000)
			bytesneeded++;
		if (code >= 0x800)
			bytesneeded++;
		if (code >= 0x80)
			bytesneeded++;
		bytesneeded++;
	}
	
	i=s;
	
	unsigned char res[bytesneeded];
	
	while (i < len)
	{
		unsigned long code=recode(data[i++], table);
		if (!code)
			continue;
				// Unicode->UTF8 encoding
		if (code < 0x80) // identity latin <-> utf8 mapping
			res[t++]=char(code);
		else if (code < 0x800) // two byte mapping
		{
			res[t++]=(code>>6)|0xC0;
			res[t++]=(code&0x3F)|0x80;
		} else if (code < 0x10000) // three bytes mapping
		{
			res[t++]=(code>>12)|0xE0;
			res[t++]=((code>>6)&0x3F)|0x80;
			res[t++]=(code&0x3F)|0x80;
		} else
		{
			res[t++]=(code>>18)|0xF0;
			res[t++]=((code>>12)&0x3F)|0x80;
			res[t++]=((code>>6)&0x3F)|0x80;
			res[t++]=(code&0x3F)|0x80;
		}
	}
	if ( t != bytesneeded)
		eFatal("t: %d, bytesneeded: %d", t, bytesneeded);
	return eString().assign((char*)res, t);
}

Descriptor *Descriptor::create(descr_gen_t *descr)
{
	switch (descr->descriptor_tag)
	{
	case DESCR_SERVICE:
		return new ServiceDescriptor((sdt_service_desc*)descr);
	case DESCR_CA_IDENT:
		return new CAIdentifierDescriptor(descr);
	case DESCR_LINKAGE:
		return new LinkageDescriptor((descr_linkage_struct*)descr);
	case DESCR_NVOD_REF:
		return new NVODReferenceDescriptor(descr);
	case DESCR_TIME_SHIFTED_SERVICE:
		return new TimeShiftedServiceDescriptor((descr_time_shifted_service_struct*)descr);
	case DESCR_TIME_SHIFTED_EVENT:
		return new TimeShiftedEventDescriptor((descr_time_shifted_event_struct*)descr);	
	case DESCR_STREAM_ID:
		return new StreamIdentifierDescriptor((descr_stream_identifier_struct*)descr);
	case 9:
		return new CADescriptor((ca_descr_t*)descr);
	case DESCR_NW_NAME:
	  return new NetworkNameDescriptor(descr);
	case DESCR_CABLE_DEL_SYS:
	  return new CableDeliverySystemDescriptor((descr_cable_delivery_system_struct*)descr);
	case DESCR_SERVICE_LIST:
	  return new ServiceListDescriptor(descr);
	case DESCR_SAT_DEL_SYS:
	  return new SatelliteDeliverySystemDescriptor((descr_satellite_delivery_system_struct*)descr);
	case DESCR_SHORT_EVENT:
		return new ShortEventDescriptor(descr);
	case DESCR_ISO639_LANGUAGE:
		return new ISO639LanguageDescriptor(descr);
	case DESCR_AC3:
		return new AC3Descriptor(descr);
	case DESCR_BOUQUET_NAME:
		return new BouquetNameDescriptor(descr);
	case DESCR_EXTENDED_EVENT:
		return new ExtendedEventDescriptor(descr);
	case DESCR_COMPONENT:
		return new ComponentDescriptor((descr_component_struct*)descr);
	case DESCR_LESRADIOS:
		return new LesRadiosDescriptor((descr_lesradios_struct*)descr);
	case DESCR_MHW_DATA:
		return new MHWDataDescriptor((descr_mhw_data_struct*)descr);
	case DESCR_PARENTAL_RATING:
		return new ParentalRatingDescriptor((descr_gen_struct*)descr);
	case DESCR_CONTENT:
		return new ContentDescriptor((descr_gen_struct*)descr);
	case DESCR_STUFFING:
	case DESCR_COUNTRY_AVAIL:
	case DESCR_MOSAIC:
	case DESCR_TELETEXT:
	case DESCR_TELEPHONE:
	case DESCR_LOCAL_TIME_OFF:
	case DESCR_SUBTITLING:
	case DESCR_TERR_DEL_SYS:
	case DESCR_ML_NW_NAME:
	case DESCR_ML_BQ_NAME:
	case DESCR_ML_SERVICE_NAME:
	case DESCR_ML_COMPONENT:
	case DESCR_PRIV_DATA_SPEC:
	case DESCR_SERVICE_MOVE:
	case DESCR_SHORT_SMOOTH_BUF:
	case DESCR_FREQUENCY_LIST:
	case DESCR_PARTIAL_TP_STREAM:
	case DESCR_DATA_BROADCAST:
	case DESCR_CA_SYSTEM:
	case DESCR_DATA_BROADCAST_ID:
	default:
		return new UnknownDescriptor(descr);
	}
}

UnknownDescriptor::UnknownDescriptor(descr_gen_t *descr): Descriptor(descr->descriptor_tag)
{
	data=new __u8[len=descr->descriptor_length+2];
	memcpy(data, descr, len);
}

UnknownDescriptor::~UnknownDescriptor()
{
	delete data;
}

eString UnknownDescriptor::toString()
{
	std::stringstream res;
	res << "UnknownDescriptor: " << decode_descr(data[0]) << " (" << std::hex << std::setfill('0') << std::setw(4) << data[0] << ")\n" << "	rawData:";
	for (int i=0; i<len; i++)
		res << std::setw(2) << data[i];
	res << std::endl;
	return res.str();
}

ServiceDescriptor::ServiceDescriptor(sdt_service_desc *descr): Descriptor(CTag())
{
	int spl=descr->service_provider_name_length;
	service_type=descr->service_type;
	service_provider=convertDVBUTF8((unsigned char*)(descr+1), spl);
	sdt_service_descriptor_2 *descr2=(sdt_service_descriptor_2*)((__u8*)(descr+1)+spl);
	spl=descr2->service_name_length;
	service_name=convertDVBUTF8((unsigned char*)(descr2+1), spl);
}

ServiceDescriptor::~ServiceDescriptor()
{
}

eString ServiceDescriptor::toString()
{
	eString res="ServiceDescriptor\n	service_type: " + qHex(service_type) + " (" + decode_service_type(service_type) + ")";
	res+="	service_provider: ";
	res+=service_provider;
	res+='\n';
	res+="	service_name: ";
	res+=service_name;
	res+='\n';
	return res;
}

CAIdentifierDescriptor::CAIdentifierDescriptor(descr_gen_t *descr): Descriptor(DESCR_CA_IDENT)
{
	CA_system_ids=descr->descriptor_length/2;
	CA_system_id=new __u16[CA_system_ids];
	for (int i=0; i<CA_system_ids; i++)
		CA_system_id[i]=(((__u8*)(descr+1))[i*2]<<8)|(((__u8*)(descr+1))[i*2+1]);
}

eString CAIdentifierDescriptor::toString()
{
	eString res="CAIdentifier\n	CA_system_id:";
	for (int i=0; i<CA_system_ids; i++)
		res+=" "+qHex(CA_system_id[i]);
	res+="\n";
	return res;
}

CAIdentifierDescriptor::~CAIdentifierDescriptor()
{
	delete[] CA_system_id;
}

LinkageDescriptor::LinkageDescriptor(descr_linkage_struct *descr): Descriptor(DESCR_LINKAGE)
{
	private_data=0;
	priv_len=0;
	int len=descr->descriptor_length+2;
	transport_stream_id=HILO(descr->transport_stream_id);
	original_network_id=HILO(descr->original_network_id);
	service_id=HILO(descr->service_id);
	linkage_type=descr->linkage_type;
	if (linkage_type!=8)
	{
		priv_len=len-LINKAGE_LEN;
		if (priv_len)
		{
			private_data=new __u8[priv_len+1];
			private_data[priv_len]=0;
			memcpy(private_data, ((__u8*)descr)+LINKAGE_LEN, priv_len);
		}
	} else
	{
		handover_type=descr->handover_type;
		origin_type=descr->origin_type;
		__u8 *ptr=((__u8*)descr)+LINKAGE_LEN+1;
		if ((handover_type == 1) ||
				(handover_type == 2) ||
				(handover_type == 3))
		{
			network_id=*ptr++ << 8;
			network_id|=*ptr++;
		}
		if (!origin_type)
		{
			initial_service_id=*ptr++ << 8;
			initial_service_id|=*ptr++;
		}
		priv_len=((__u8*)descr)+len-ptr;
		if (priv_len)
		{
			private_data=new __u8[priv_len];
			memcpy(private_data, ptr, priv_len);
		}
	}
}

LinkageDescriptor::~LinkageDescriptor()
{
	if (private_data)
		delete[] private_data;
}

eString LinkageDescriptor::toString()
{
	eString res="LinkageDescriptor\n";
	res+="	transport_stream_id: "+qHex(transport_stream_id)+"\n";
	res+="	original_network_id: "+qHex(original_network_id)+"\n";
	res+="	service_id: "+qHex(service_id)+"\n";
	res+="	linkage_type: "+qHex(linkage_type)+"\n";
	if (linkage_type==8)
	{
		res+="	hand-over_type: " + qHex(handover_type) + "\n";
		res+="	origin_type: " + qHex(origin_type) + "\n";
		if (!origin_type)
		{
			res+="	network_id: " + qHex(network_id)  + "\n";
			res+="	intial_service_id: " + qHex(initial_service_id)  + "\n";
		}
	}
	if (priv_len)
	{
		res+="	private data:";
		for (int i=0; i<priv_len; i++)
			res+=eString().sprintf(" %02x", private_data[i]);
		res+="\n";
	}
	return res;
}

NVODReferenceEntry::NVODReferenceEntry(__u16 transport_stream_id, __u16 original_network_id, __u16 service_id):
	transport_stream_id(transport_stream_id), original_network_id(original_network_id), service_id(service_id)
{
}

NVODReferenceEntry::~NVODReferenceEntry()
{
}

NVODReferenceDescriptor::NVODReferenceDescriptor(descr_gen_t *descr): Descriptor(DESCR_NVOD_REF)
{
	entries.setAutoDelete(true);
	int len=descr->descriptor_length;
	for (int i=0; i<len; i+=6)
		entries.push_back(new NVODReferenceEntry((((__u8*)(descr+1))[i+0]<<8) | (((__u8*)(descr+1))[i+1]),
			(((__u8*)(descr+1))[i+2]<<8) | (((__u8*)(descr+1))[i+3]),	(((__u8*)(descr+1))[i+4]<<8) | (((__u8*)(descr+1))[i+5])));
}

NVODReferenceDescriptor::~NVODReferenceDescriptor()
{
}

eString NVODReferenceDescriptor::toString()
{
	eString res;
	res="NVODReferenceDescriptor\n";
	for (ePtrList<NVODReferenceEntry>::iterator i(entries); i != entries.end(); ++i)
	{
		res+="	NVODReferenceEntry\n";
		res+="		transport_stream_id: " + qHex(i->transport_stream_id) + "\n";
		res+="		original_network_id: " + qHex(i->original_network_id) + "\n";
		res+="		service_id: " + qHex(i->service_id) + "\n";
	}
	return res;
}

TimeShiftedServiceDescriptor::TimeShiftedServiceDescriptor(descr_time_shifted_service_struct *descr): Descriptor(DESCR_TIME_SHIFTED_SERVICE)
{
	reference_service_id=HILO(descr->reference_service_id);
}

eString TimeShiftedServiceDescriptor::toString()
{
	eString res="TimeShiftedServiceDescriptor\n";
	res+="	reference_service_id: " + qHex(reference_service_id) + "\n";
	return res;
}

TimeShiftedEventDescriptor::TimeShiftedEventDescriptor(descr_time_shifted_event_struct *descr): Descriptor(DESCR_TIME_SHIFTED_EVENT)
{
	reference_service_id=HILO(descr->reference_service_id);
	reference_event_id=HILO(descr->reference_event_id);
}

eString TimeShiftedEventDescriptor::toString()
{
	eString res="TimeShiftedEventDescriptor\n";
	res+="	reference_service_id: " + qHex(reference_service_id) + "\n";
	res+="	reference_event_id: " + qHex(reference_event_id) + "\n";	
	return res;
}

StreamIdentifierDescriptor::StreamIdentifierDescriptor(descr_stream_identifier_struct *descr): Descriptor(DESCR_STREAM_ID)
{
	component_tag=descr->component_tag;
}

eString StreamIdentifierDescriptor::toString()
{
	eString res="StreamIdentifierDescriptor\n";
	res+="	component_tag: " + qHex(component_tag) + "\n";
	return res;
}

CADescriptor::CADescriptor(ca_descr_t *descr): Descriptor(9)
{
	data=new __u8[descr->descriptor_length+2];
	memcpy(data, descr, descr->descriptor_length+2);
	CA_system_ID=HILO(descr->CA_system_ID);
	CA_PID=HILO(descr->CA_PID);
}

CADescriptor::~CADescriptor()
{
	delete[] data;
}

eString CADescriptor::toString()
{
	eString res="CADescriptor\n";
	res+="	CA_system_ID: "+qHex(CA_system_ID)+"\n";
	res+="	CA_PID: "+qHex(CA_PID)+"\n";
	return res;
}

NetworkNameDescriptor::NetworkNameDescriptor(descr_gen_t *descr): Descriptor(DESCR_NW_NAME)
{
	int len=descr->descriptor_length;
	network_name=convertDVBUTF8((unsigned char*)descr+1, len);
}

NetworkNameDescriptor::~NetworkNameDescriptor()
{
}

eString NetworkNameDescriptor::toString()
{
	eString res="NetworkNameDescriptor\n";
	res+="  network_name: " + eString(network_name) + "\n";
	return res;
}

CableDeliverySystemDescriptor::CableDeliverySystemDescriptor(descr_cable_delivery_system_struct *descr): Descriptor(DESCR_CABLE_DEL_SYS)
{
	frequency= (descr->frequency1>>4) *1000000;
	frequency+=(descr->frequency1&0xF)*100000;
	frequency+=(descr->frequency2>>4) *10000;
	frequency+=(descr->frequency2&0xF)*1000;
	frequency+=(descr->frequency3>>4) *100;
	frequency+=(descr->frequency3&0xF)*10;
	frequency+=(descr->frequency4>>4) *1;
//	frequency+=(descr->frequency4&0xF)*1;
	FEC_outer=descr->fec_outer;
	modulation=descr->modulation;
	symbol_rate=(descr->symbol_rate1>>4)   * 100000000;
	symbol_rate+=(descr->symbol_rate1&0xF) * 10000000;
	symbol_rate+=(descr->symbol_rate2>>4)  * 1000000;
	symbol_rate+=(descr->symbol_rate2&0xF) * 100000;
	symbol_rate+=(descr->symbol_rate3>>4)  * 10000;
	symbol_rate+=(descr->symbol_rate3&0xF) * 1000;
	symbol_rate+=(descr->symbol_rate4&0xF) * 100;
	FEC_inner=descr->fec_inner;
}

CableDeliverySystemDescriptor::~CableDeliverySystemDescriptor()
{
}

eString CableDeliverySystemDescriptor::toString()
{
	eString res="CableDeliverySystemDescriptor\n";
	res+=eString().sprintf("  frequency: %d\n", frequency);
	res+=eString().sprintf("  FEC_outer: %d\n", FEC_outer);
	res+=eString().sprintf("  modulation: QAM%d\n", 8<<modulation);
	res+=eString().sprintf("  symbol_rate: %d\n", symbol_rate);
	res+=eString().sprintf("  FEC_inner: %d\n", FEC_inner);
	return res;
}

SatelliteDeliverySystemDescriptor::SatelliteDeliverySystemDescriptor(descr_satellite_delivery_system_struct *descr): Descriptor(DESCR_SAT_DEL_SYS)
{
	frequency= (descr->frequency1>>4) *100000000;
	frequency+=(descr->frequency1&0xF)*10000000;
	frequency+=(descr->frequency2>>4) *1000000;
	frequency+=(descr->frequency2&0xF)*100000;
	frequency+=(descr->frequency3>>4) *10000;
	frequency+=(descr->frequency3&0xF)*1000;
	frequency+=(descr->frequency4>>4) *100;
	frequency+=(descr->frequency4&0xF);
	orbital_position =(descr->orbital_position1>>4)*1000;
	orbital_position+=(descr->orbital_position1&0xF)*100;
	orbital_position+=(descr->orbital_position2>>4)*10;
	orbital_position+=(descr->orbital_position2&0xF)*1;
	west_east_flag=descr->west_east_flag;
	polarisation=descr->polarization;
	modulation=descr->modulation;
	symbol_rate=(descr->symbol_rate1>>4)   * 100000000;
	symbol_rate+=(descr->symbol_rate1&0xF) * 10000000;
	symbol_rate+=(descr->symbol_rate2>>4)  * 1000000;
	symbol_rate+=(descr->symbol_rate2&0xF) * 100000;
	symbol_rate+=(descr->symbol_rate3>>4)  * 10000;
	symbol_rate+=(descr->symbol_rate3&0xF) * 1000;
	symbol_rate+=(descr->symbol_rate4&0xF) * 100;
	FEC_inner=descr->fec_inner;
}

SatelliteDeliverySystemDescriptor::~SatelliteDeliverySystemDescriptor()
{
}

eString SatelliteDeliverySystemDescriptor::toString()
{
	eString res;
	res+=eString().sprintf("SatelliteDeliverySystemDescriptor\n");
	res+=eString().sprintf("  frequency: %d\n", frequency);
	res+=eString().sprintf("  orbital_position: %3d.%d%c\n", orbital_position/10, orbital_position%10, west_east_flag?'E':'W');
	res+="  polarisation: ";
	switch (polarisation)
	{
	case 0: res+=eString().sprintf("linear - horizontal\n"); break;
	case 1: res+=eString().sprintf("linear - vertical\n"); break;
	case 2: res+=eString().sprintf("circular - left (*cool*)\n"); break;
	case 3: res+=eString().sprintf("circular - right (*cool*)\n"); break;
  }
	res+=eString().sprintf("  modulation: %d\n", modulation);
	res+=eString().sprintf("  symbol_rate: %d\n", symbol_rate);
	res+=eString().sprintf("  FEC_inner: %d/%d\n", FEC_inner, FEC_inner+1);
	return res;
}

ServiceListDescriptorEntry::ServiceListDescriptorEntry(__u16 service_id, __u8 service_type):
	service_id(service_id), service_type(service_type)
{
}


ServiceListDescriptorEntry::~ServiceListDescriptorEntry()
{
}

ServiceListDescriptor::ServiceListDescriptor(descr_gen_t *descr): Descriptor(DESCR_SERVICE_LIST)
{
	entries.setAutoDelete(true);
	int len=descr->descriptor_length;
	for (int i=0; i<len; i+=3)
		entries.push_back(new ServiceListDescriptorEntry((((__u8*)(descr+1))[i+0]<<8) | (((__u8*)(descr+1))[i+1]), ((__u8*)(descr+1))[i+2]));
}

ServiceListDescriptor::~ServiceListDescriptor()
{
}

eString ServiceListDescriptor::toString()
{
	eString res="ServiceListDescriptor\n";
	for (ePtrList<ServiceListDescriptorEntry>::iterator i(entries); i != entries.end(); ++i)
	{
		res+=eString().sprintf("	ServiceListDescriptorEntry\n");
		res+=eString().sprintf("		service_id: %04x\n", i->service_id);
		res+=eString().sprintf("		service_type: %04x\n", i->service_type);
	}
	return res;
}

ShortEventDescriptor::ShortEventDescriptor(descr_gen_t *descr): Descriptor(DESCR_SHORT_EVENT)
{
	__u8 *data=(__u8*)descr;
	memcpy(language_code, data+2, 3);
	int ptr=5;
	int len=data[ptr++];

	if (len && (data[ptr]<0x20))			// ignore charset
	{
		ptr++;
		len--;
	}

	event_name=convertDVBUTF8((unsigned char*)data+ptr, len);
	ptr+=len;

	len=data[ptr++];

	if (len && (data[ptr]<0x20))			// ignore charset
	{
		ptr++;
		len--;
	}

	text=convertDVBUTF8((unsigned char*) data+ptr, len);
}

eString ShortEventDescriptor::toString()
{
	eString res="ShortEventDescriptor\n";
	res+="  event_name: "+event_name+'\n';
	res+="  text: "+text+'\n';
	return res;
}

ISO639LanguageDescriptor::ISO639LanguageDescriptor(descr_gen_t *descr): Descriptor(DESCR_ISO639_LANGUAGE)
{
	__u8 *data=(__u8*)descr;
	memcpy(language_code, data+2, 3);
	audio_type=data[5];
}

eString ISO639LanguageDescriptor::toString()
{
	eString res;
	res+=eString().sprintf("ISO639LangugageDescriptor\n");
	res+=eString().sprintf("  language_code: %c%c%c\n", language_code[0], language_code[1], language_code[2]);
	res+=eString().sprintf("  audio_type: %d\n", audio_type);
	return res;
}

AC3Descriptor::AC3Descriptor(descr_gen_t *descr): Descriptor(DESCR_AC3)
{
	__u8 *data=(__u8*)descr;
	data+=2;
	int flags=*data++;
	if (flags&0x80)
		AC3_type=*data++;
	else
		AC3_type=-1;
	if (flags&0x40)
		bsid=*data++;
	else
		bsid=-1;
	if (flags&0x20)
		mainid=*data++;
	else
		mainid=-1;
	if (flags&0x10)
		asvc=*data++;
	else
		asvc=-1;
}

eString AC3Descriptor::toString()
{
	eString res="AC3Descriptor\n";
	if (AC3_type!=-1)
		res+=eString().sprintf("  AC3_type: %d", AC3_type);
	if (bsid!=-1)
		res+=eString().sprintf("  bsid: %d", bsid);
	if (mainid!=-1)
		res+=eString().sprintf("  mainid: %d", mainid);
	if (asvc!=-1)
		res+=eString().sprintf("  asvc: %d", asvc);
	return res;
}

BouquetNameDescriptor::BouquetNameDescriptor(descr_gen_t *descr): Descriptor(DESCR_BOUQUET_NAME)
{
	__u8 *data=(__u8*)descr;
	int len=descr->descriptor_length;
	data+=2;
	name="";
	while (len--)
		name+=*data++;
}

eString BouquetNameDescriptor::toString()
{
	eString res="BouquetNameDescriptor\n";
	res+="  name: "+name+"\n";
	return res;
}

ExtendedEventDescriptor::ExtendedEventDescriptor(descr_gen_t *descr): Descriptor(DESCR_EXTENDED_EVENT)
{
	struct eit_extended_descriptor_struct *evt=(struct eit_extended_descriptor_struct *)descr;
	descriptor_number = evt->descriptor_number;
	last_descriptor_number = evt->last_descriptor_number;
	item_description_length = evt->item_description_length;
	language_code[0]=evt->iso_639_2_language_code_1;
	language_code[1]=evt->iso_639_2_language_code_2;
	language_code[2]=evt->iso_639_2_language_code_3;

	int ptr = sizeof(struct eit_extended_descriptor_struct);
	__u8* data = (__u8*) descr;
	item_description=convertDVBUTF8((unsigned char*)data+ptr, item_description_length);
}

eString ExtendedEventDescriptor::toString()
{
	eString res="ExtendedEventDescriptor\n";
	res+=eString().sprintf("  language_code: %c%c%c\n", language_code[0], language_code[1], language_code[2]);
	res+=eString().sprintf("  descriptor %i / %i\n", descriptor_number, last_descriptor_number);
	res+=eString().sprintf("  description length: %i\n", item_description_length);
	res+="  description : "+item_description+"\n";
	return res;
}

ComponentDescriptor::ComponentDescriptor(descr_component_struct *descr): Descriptor(DESCR_COMPONENT)
{
	int len=descr->descriptor_length+2;
	stream_content=descr->stream_content;
	component_type=descr->component_type;
	component_tag=descr->component_tag;
	language_code[0]=descr->lang_code1;
	language_code[1]=descr->lang_code2;
	language_code[2]=descr->lang_code3;
	len-=sizeof(descr_component_struct);
	__u8 *p=(__u8*)(descr+1);
	text="";
	while (len--)
		text+=*p++;
}

eString ComponentDescriptor::toString()
{
	eString res="ComponentDescriptor\n";
	res+=eString().sprintf("  stream_content: %d\n", stream_content);
	res+=eString().sprintf("  component_type: %d\n", component_type);
	res+=eString().sprintf("  component_tag: %d\n", component_tag);
	res+="  text: "+text+'\n';
	return res;
}

ContentDescriptor::ContentDescriptor(descr_gen_t *descr):Descriptor(DESCR_CONTENT)
{
	int len=descr->descriptor_length;
	contentList.setAutoDelete(true);
	__u8 *data=((__u8*)descr)+sizeof(descr_gen_t);
	__u8 *work=data;

  while( work < data+len )
	{
		descr_content_entry_struct *tmp = new descr_content_entry_struct();
		memcpy(tmp, work, sizeof(descr_content_entry_struct) );
		contentList.push_back( tmp );
		work+=2;
	}
}

eString ContentDescriptor::toString()
{
	eString res="ContentDescriptor\n";
	for (ePtrList<descr_content_entry_struct>::iterator it( contentList.begin() ); it != contentList.end(); it++)
		res+=eString().sprintf("nibble1 = %02x, nibble2 = %02x, user1 = %02x, user2 = %02x\n",
																	it->content_nibble_level_1, it->content_nibble_level_2, it->user_nibble_1, it->user_nibble_2 );
	return res;
}	

LesRadiosDescriptor::LesRadiosDescriptor(descr_lesradios_struct *descr): Descriptor(DESCR_LESRADIOS)
{
	int len=descr->descriptor_length+2;
	id=descr->id;
	len-=sizeof(descr_lesradios_struct);
	char *lname=(char*)(descr+1);
	name="";
	while (len--)
		name+=*lname++;
}

eString LesRadiosDescriptor::toString()
{
	eString res;
	res="LesRadioDescriptor\n";
	res+=eString().sprintf("  id: %d\n", id);
	res+="  name";
	res+=name;
	res+="\n";
	return res;
}

MHWDataDescriptor::MHWDataDescriptor(descr_mhw_data_struct *descr)
	: Descriptor(DESCR_MHW_DATA)
{
	memcpy(type, descr->type, 8);
}

eString MHWDataDescriptor::toString()
{
	eString res;
	res="MHWDataDescriptor\n  ";
	for (int i=0; i<8; i++)
		res+=type[i];
	res+="\n";
	return res;
}

ParentalRatingDescriptor::ParentalRatingDescriptor( descr_gen_struct *descr)
	: Descriptor(DESCR_PARENTAL_RATING)
{
  const char *data = ((char*)descr)+sizeof(struct descr_gen_struct);
	const char *work = data;
	int len=descr->descriptor_length;

  while( work < data+len )
	{
    entryMap[ eString(work, 3) ] = *(work+3)+3;
    work+=4;
  }
}

eString ParentalRatingDescriptor::toString()
{
	eString res="ParentalRatingDescriptor";
	for ( std::map<eString,int>::iterator it(entryMap.begin()); it != entryMap.end(); it++)
		res += eString().sprintf("\n  Country: %s, Age: %i",it->first.c_str(), it->second);
	return res;
}

PAT::PAT(): eTable(PID_PAT, TID_PAT)
{
	entries.setAutoDelete(true);
}

int PAT::data(__u8* data)
{
	pat_t &pat=*(pat_t*)data;
	if (pat.table_id!=TID_PAT)
		return -1;
	int slen=HILO(pat.section_length)+3;
	transport_stream_id=HILO(pat.transport_stream_id);
	
	pat_prog_t *prog=(pat_prog_t*)(data+PAT_LEN);
	
	for (int ptr=PAT_LEN; ptr<slen-4; ptr+=PAT_PROG_LEN, prog++)
		entries.push_back(new PATEntry(HILO(prog->program_number), HILO(prog->network_pid)));
	return 0;
}

SDTEntry::SDTEntry(sdt_descr_t *descr)
{
	service_id=HILO(descr->service_id);
	EIT_schedule_flag=descr->EIT_schedule_flag;
	EIT_present_following_flag=descr->EIT_present_following_flag;

	int dlen=HILO(descr->descriptors_loop_length)+SDT_DESCR_LEN;
	int ptr=SDT_DESCR_LEN;
	while (ptr<dlen)
	{
		descr_gen_t *d=(descr_gen_t*)(((__u8*)descr)+ptr);
		descriptors.push_back(Descriptor::create(d));
		ptr+=d->descriptor_length+2;
	}
}

SDT::SDT(int type): eTable(PID_SDT, type?TID_SDT_OTH:TID_SDT_ACT)
{
	entries.setAutoDelete(true);
}

int SDT::data(__u8 *data)
{
	sdt_t &sdt=*(sdt_t*)data;
	int slen=HILO(sdt.section_length)+3;
	transport_stream_id=HILO(sdt.transport_stream_id);
	original_network_id=HILO(sdt.original_network_id);
	
	int ptr=SDT_LEN;
	while (ptr<slen-4)
	{
		sdt_descr_t *descr=(sdt_descr_t*)(data+ptr);
		entries.push_back(new SDTEntry(descr));
		int dlen=HILO(descr->descriptors_loop_length);
		ptr+=SDT_DESCR_LEN+dlen;
	}
	if (ptr != (slen-4))
		return -1;
	
	return 0;
}

PMTEntry::PMTEntry(pmt_info_t* info)
{
	ES_info.setAutoDelete(true);
	stream_type=info->stream_type;
	elementary_PID=HILO(info->elementary_PID);
	int elen=HILO(info->ES_info_length);
	int ptr=0;
	while (ptr<elen)
	{
		descr_gen_t *d=(descr_gen_t*)(((__u8*)info)+PMT_info_LEN+ptr);
		ES_info.push_back(Descriptor::create(d));
		ptr+=d->descriptor_length+2;
	}
}

PMT::PMT(int pid, int service_id): eTable(pid, TID_PMT, service_id)
{
	program_info.setAutoDelete(true);
	streams.setAutoDelete(true);
}

int PMT::data(__u8 *data)
{
	pmt_struct *pmt=(pmt_struct*)data;
	program_number=HILO(pmt->program_number);
	PCR_PID=HILO(pmt->PCR_PID);
	
	int program_info_len=HILO(pmt->program_info_length);
	int len=HILO(pmt->section_length)+3-4;
	int ptr=PMT_LEN;
	while (ptr<(program_info_len+PMT_LEN))
	{
		descr_gen_t *d=(descr_gen_t*)(data+ptr);
		program_info.push_back(Descriptor::create(d));
		ptr+=d->descriptor_length+2;
	}
	while (ptr<len)
	{
		streams.push_back(new PMTEntry((pmt_info_t*)(data+ptr)));
		ptr+=HILO(((pmt_info_t*)(data+ptr))->ES_info_length)+PMT_info_LEN;
	}
	return ptr!=len;
}

NITEntry::NITEntry(nit_ts_t* ts)
{
	transport_descriptor.setAutoDelete(true);
	transport_stream_id=HILO(ts->transport_stream_id);
	original_network_id=HILO(ts->original_network_id);
	int elen=HILO(ts->transport_descriptors_length);
	int ptr=0;
	while (ptr<elen)
	{
		descr_gen_t *d=(descr_gen_t*)(((__u8*)ts)+NIT_TS_LEN+ptr);
		transport_descriptor.push_back(Descriptor::create(d));
		ptr+=d->descriptor_length+2;
  }
}

NIT::NIT(int pid, int type): eTable(pid, type?TID_NIT_OTH:TID_NIT_ACT)
{
	entries.setAutoDelete(true);
	network_descriptor.setAutoDelete(true);
}

int NIT::data(__u8* data)
{
	nit_t *nit=(nit_t*)data;
	network_id=HILO(nit->network_id);
	int network_descriptor_len=HILO(nit->network_descriptor_length);
	int len=HILO(nit->section_length)+3-4;
	int ptr=NIT_LEN;
	while (ptr<(network_descriptor_len+NIT_LEN))
	{
		descr_gen_t *d=(descr_gen_t*)(data+ptr);
		network_descriptor.push_back(Descriptor::create(d));
		ptr+=d->descriptor_length+2;
	}
	ptr+=2;
	while (ptr<len)
	{
		entries.push_back(new NITEntry((nit_ts_t*)(data+ptr)));
		ptr+=HILO(((nit_ts_t*)(data+ptr))->transport_descriptors_length)+NIT_TS_LEN;
	}
	return ptr!=len;
}

EITEvent::EITEvent(const eit_event_struct *event)
{
	descriptor.setAutoDelete(true);
	event_id=HILO(event->event_id);
	if (event->start_time_5!=0xFF)
		start_time=parseDVBtime(event->start_time_1, event->start_time_2, event->start_time_3,event->start_time_4, event->start_time_5);
	else
		start_time=-1;
	if ((event->duration_1==0xFF) || (event->duration_2==0xFF) || (event->duration_3==0xFF))
		duration=-1;
	else
		duration=fromBCD(event->duration_1)*3600+fromBCD(event->duration_2)*60+fromBCD(event->duration_3);
	running_status=event->running_status;
	free_CA_mode=event->free_CA_mode;
	int ptr=0;
	int len=HILO(event->descriptors_loop_length);
	while (ptr<len)
	{
		descr_gen_t *d=(descr_gen_t*) (((__u8*)(event+1))+ptr);
		descriptor.push_back(Descriptor::create(d));
		ptr+=d->descriptor_length+2;
	}
}

EITEvent::EITEvent()
{
	descriptor.setAutoDelete(true);
}

int EIT::data(__u8 *data)
{
	eit_t *eit=(eit_t*)data;
	service_id=HILO(eit->service_id);
	version_number=eit->version_number;
	current_next_indicator=eit->current_next_indicator;
	transport_stream_id=HILO(eit->transport_stream_id);
	original_network_id=HILO(eit->original_network_id);
	int len=HILO(eit->section_length)+3-4;
	int ptr=EIT_SIZE;
	while (ptr<len)
	{
		events.push_back(new EITEvent((eit_event_struct*)(data+ptr)));
		ptr+=HILO(((eit_event_struct*)(data+ptr))->descriptors_loop_length)+EIT_LOOP_SIZE;
	}
	return ptr!=len;
}

EIT::EIT(int type, int service_id, int ts, int version): eTable(PID_EIT, ts?TID_EIT_OTH:TID_EIT_ACT, service_id, version), type(type), ts(ts)
{
	events.setAutoDelete(true);
}

EIT::EIT()
{
	events.setAutoDelete(true);
}

eTable *EIT::createNext()
{
	if (ts != tsFaked)
		return new EIT(type, service_id, ts, incrementVersion(version));
	return 0;
}

int TDT::data(__u8 *data)
{
	tdt_t *tdt=(tdt_t*)data;
	if (tdt->utc_time5!=0xFF)
	{
		UTC_time=parseDVBtime(tdt->utc_time1, tdt->utc_time2, tdt->utc_time3,
			tdt->utc_time4, tdt->utc_time5);
		return 1;
	} else
	{
		eFatal("invalide TDT::data");
		UTC_time=-1;
		return -1;
	}
}

TDT::TDT(): eTable(PID_TDT, TID_TDT)
{
}

BATEntry::BATEntry(bat_loop_struct *entry)
{
	transport_stream_id=HILO(entry->transport_stream_id);
	original_network_id=HILO(entry->original_network_id);
	int len=HILO(entry->transport_descriptors_length);
	int ptr=0;
	__u8 *data=(__u8*)(entry+1);
	
	transport_descriptors.setAutoDelete(true);
	
	while (ptr<len)
	{
		descr_gen_t *d=(descr_gen_t*) (data+ptr);
		transport_descriptors.push_back(Descriptor::create(d));
		ptr+=d->descriptor_length+2;
	}
}

int BAT::data(__u8 *data)
{
	bat_t *bat=(bat_t*)data;
	bouquet_id=HILO(bat->bouquet_id);
	int looplen=HILO(bat->bouquet_descriptors_length);
	int ptr=0;
	while (ptr<looplen)
	{
		descr_gen_t *d=(descr_gen_t*) (((__u8*)(bat+1))+ptr);
		bouquet_descriptors.push_back(Descriptor::create(d));
		ptr+=d->descriptor_length+2;
	}
	data+=looplen+BAT_SIZE;
	looplen=((data[0]&0xF)<<8)|data[1];
	data+=2;
	ptr=0;
	while (ptr<looplen)
	{
		entries.push_back(new BATEntry((bat_loop_struct*)(data+ptr)));
		ptr+=HILO(((bat_loop_struct*)(data+ptr))->transport_descriptors_length)+BAT_LOOP_SIZE;
	}
	return ptr!=looplen;
}

BAT::BAT(): eTable(PID_BAT, TID_BAT)
{
	bouquet_descriptors.setAutoDelete(true);
	entries.setAutoDelete(true);
}

MHWEIT::MHWEIT(int pid, int service_id): eSection(pid, 0x90, service_id, -1, 0, 0xFD)
{
	available=0;
	events.resize(2);
}

void MHWEIT::sectionFinish(int err)
{
	/*emit*/ ready(err);
}

int MHWEIT::sectionRead(__u8 *data)
{
	struct mhweit90_s
	{
		__u8 table_id :8;
		__u8 section_length_hi :8;
		__u8 section_length_lo :8;
		__u8 tableid_ext_hi :8;
		__u8 tableid_ext_lo :8;
		__u8 starttime_hi;
		__u8 starttime_lo;
		__u8 flags_hi;
		__u8 flags_lo;
		__u8 duration_hi;
		__u8 duration_lo;
		__u8 event_name[30];
		__u8 short_description[15];
	} *table=(mhweit90_s*)data;
	if (table->table_id != 0x90)
		return 0;
	
	int nownext;
	switch ((HILO(table->flags)>>6)&3)
	{
	case 3:
		nownext=0;
		break;
	case 1:
		nownext=1;
		break;
	case 2:
		return -1;
		break;
	default:
		eDebug("bla falsche flags (%x)", HILO(table->flags));
		return -1;
	}
	available|=nownext?1:2;
	MHWEITEvent &event=events[nownext];

	event.service_id=HILO(table->tableid_ext);
	event.starttime=HILO(table->starttime);
	event.duration=HILO(table->duration);
	event.event_name="";
	event.flags=HILO(table->flags);
	int len=30;
	while (len-- && (table->event_name[len+1]==' '))
		;

/*	for (int i=0; i<len; i++)
		event.event_name+=table->event_name[i];*/

	event.event_name.append( (char*) &table->event_name[0], len);

	len=15;
	event.short_description="";

	while (len-- && (table->short_description[len+1]==' '))
		;

	event.short_description.append( (char*) &table->short_description[0], len);
/*	for (int i=0; i<len; i++)
		event.short_description+=table->short_description[i];*/

	if (available==3)
		return 1;

	return 0;
}

#include <lib/dvb/si.h>

#include <stdio.h>
#include <time.h>
#include <sstream>
#include <iomanip>
#include <lib/system/info.h>

extern "C"
{
	time_t my_mktime (struct tm *tp);
}
#define HILO(x) (x##_hi << 8 | x##_lo) 
#include <lib/dvb/lowlevel/decode.h>
#include <lib/dvb/lowlevel/dvb.h>
#include <lib/dvb/lowlevel/pat.h>
#include <lib/dvb/lowlevel/tdt.h>

static int getEncodingTable( const char * language_code )
{
	if (!memcmp(language_code, "gre", 3))
		return 7;  // ISO8859-7
	else 
		if (!memcmp(language_code, "pol", 3) // Polish
/*		|| !memcmp(language_code, "cze", 3)  // Czech
		|| !memcmp(language_code, "ces", 3)
		|| !memcmp(language_code, "slv", 3)  // Slovenian
		|| !memcmp(language_code, "slo", 3)  // Slovak
		|| !memcmp(language_code, "slk", 3)
		|| !memcmp(language_code, "scr", 3)  // Croatian
		|| !memcmp(language_code, "hrv", 3)
		|| !memcmp(language_code, "rum", 3)  // Romanian
		|| !memcmp(language_code, "ron", 3)
		|| !memcmp(language_code, "wen", 3)*/) // Sorbian language
			return 2; // ISO8859-2
	else 
		if (!memcmp(language_code,"rus", 3)  // Russian
/*		|| !memcmp(language_code, "bul", 3)  // Bulgarian
		|| !memcmp(language_code, "scc", 3)  // Serbian
		|| !memcmp(language_code, "srp", 3)
		|| !memcmp(language_code, "mac", 3)  // Macedonian 
		|| !memcmp(language_code, "mkd", 3)
		|| !memcmp(language_code, "ukr", 3)*/) // Ukrainian
			return 5; // ISO8859-5
	return 0;  // ISO8859-1 / Latin1
}

static eString qHex(int v)
{
	return eString().sprintf("%04x", v);
}

int fromBCD(int bcd)
{
	if ((bcd&0xF0)>=0xA0)
		return -1;
	if ((bcd&0xF)>=0xA)
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

	return timegm(&t);
//	return my_mktime(&t)-timezone;
}

static unsigned int crc32_table[256] = {
	0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
	0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
	0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
	0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
	0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
	0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
	0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
	0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
	0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
	0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
	0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
	0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
	0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
	0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
	0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
	0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
	0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
	0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
	0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
	0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
	0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
	0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
	0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
	0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
	0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
	0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
	0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
	0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
	0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
	0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
	0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
	0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
	0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
	0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
	0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
	0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
	0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
	0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
	0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
	0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
	0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
	0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
	0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4};

static unsigned int crc32_be(unsigned int crc, unsigned char const *data, unsigned int len)
{
	for (unsigned int i=0; i<len; i++)
		crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ *data++) & 0xff];

	return crc;
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
	case DESCR_REGISTRATION:
		return new RegistrationDescriptor((descr_gen_struct*)descr);
	case DESCR_TERR_DEL_SYS:
		return new TerrestrialDeliverySystemDescriptor((descr_terrestrial_delivery_system_struct*)descr);
	case DESCR_STUFFING:
	case DESCR_COUNTRY_AVAIL:
	case DESCR_MOSAIC:
	case DESCR_TELETEXT:
	case DESCR_TELEPHONE:
	case DESCR_LOCAL_TIME_OFF:
	case DESCR_SUBTITLING:
		return new SubtitlingDescriptor((descr_gen_struct*)descr);
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

eString Descriptor::toXML()
{
	return "<descriptor><parsed>" + toString() + "</parsed></descriptor>\n";
}

UnknownDescriptor::UnknownDescriptor(descr_gen_t *descr)
	:Descriptor(descr)
{
}

UnknownDescriptor::~UnknownDescriptor()
{
}

#ifdef SUPPORT_XML
eString UnknownDescriptor::toString()
{
	return "";
}
#endif

ServiceDescriptor::ServiceDescriptor(sdt_service_desc *descr)
	:Descriptor((descr_gen_t*)descr)
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

#ifdef SUPPORT_XML
eString ServiceDescriptor::toString()
{
	eString res=
	"<ServiceDescriptor>"
	"<service_type>" + qHex(service_type);
	res+="</service_type><service_provider>";
	res+=service_provider;
	res+="</service_provider><service_name>";
	res+=service_name;
	res+="</service_name></ServiceDescriptor>\n";
	return res;
}
#endif

CAIdentifierDescriptor::CAIdentifierDescriptor(descr_gen_t *descr)
	:Descriptor(descr)
{
	CA_system_ids=descr->descriptor_length/2;
	CA_system_id=new __u16[CA_system_ids];
	for (int i=0; i<CA_system_ids; i++)
		CA_system_id[i]=(((__u8*)(descr+1))[i*2]<<8)|(((__u8*)(descr+1))[i*2+1]);
}

#ifdef SUPPORT_XML
eString CAIdentifierDescriptor::toString()
{
	eString res="<CAIdentifier>";
	for (int i=0; i<CA_system_ids; i++)
		res+="<ca_system_id>"+qHex(CA_system_id[i])+"</ca_system_id>";
	res+="</CAIdentifier>\n";
	return res;
}
#endif

CAIdentifierDescriptor::~CAIdentifierDescriptor()
{
	delete[] CA_system_id;
}

LinkageDescriptor::LinkageDescriptor(descr_linkage_struct *descr)
	:Descriptor((descr_gen_t*)descr)
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

#ifdef SUPPORT_XML
eString LinkageDescriptor::toString()
{
	eString res="<LinkageDescriptor>";
	res+="<transport_stream_id>"+qHex(transport_stream_id)+"</transport_stream_id>";
	res+="<original_network_id>"+qHex(original_network_id)+"</original_network_id>";
	res+="<service_id>"+qHex(service_id)+"</service_id>";
	res+="<linkage_type>"+qHex(linkage_type)+"</linkage_type>";
	if (linkage_type==8)
	{
		res+="<handover_type>" + qHex(handover_type) + "</handover_type>";
		res+="<origin_type>" + qHex(origin_type) + "</handover_type>";
		if (!origin_type)
		{
			res+="<network_id>" + qHex(network_id)  + "</network_id>";
			res+="<initial_service_id>" + qHex(initial_service_id)  + "</intial_service_id>";
		}
	}
	if (priv_len)
	{
		res+="<private>";
		for (int i=0; i<priv_len; i++)
			res+=eString().sprintf(" %02x", private_data[i]);
		res+="</private>";
	}
	res+="</LinkageDescriptor>\n";
	return res;
}
#endif

NVODReferenceEntry::NVODReferenceEntry(__u16 transport_stream_id, __u16 original_network_id, __u16 service_id)
	:transport_stream_id(transport_stream_id), original_network_id(original_network_id), service_id(service_id)
{
}

NVODReferenceEntry::~NVODReferenceEntry()
{
}

NVODReferenceDescriptor::NVODReferenceDescriptor(descr_gen_t *descr)
	:Descriptor(descr)
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

#ifdef SUPPORT_XML
eString NVODReferenceDescriptor::toString()
{
	eString res;
	res="<NVODReferenceDescriptor>";
	for (ePtrList<NVODReferenceEntry>::iterator i(entries); i != entries.end(); ++i)
	{
		res+="<NVODReferenceEntry>";
		res+="<transport_stream_id>" + qHex(i->transport_stream_id) + "</transport_stream_id>";
		res+="<original_network_id>" + qHex(i->original_network_id) + "</original_network_id>";
		res+="<service_id>" + qHex(i->service_id) + "</service_id>";
	}
	res+="</NVODReferenceDescriptor>\n";
	return res;
}
#endif

TimeShiftedServiceDescriptor::TimeShiftedServiceDescriptor(descr_time_shifted_service_struct *descr)
	:Descriptor((descr_gen_t*)descr)
{
	reference_service_id=HILO(descr->reference_service_id);
}

#ifdef SUPPORT_XML
eString TimeShiftedServiceDescriptor::toString()
{
	eString res="<TimeShiftedServiceDescriptor>";
	res+="<reference_service_id>" + qHex(reference_service_id) + "</reference_service_id>";
	res+="</TimeShiftedServiceDescriptor>\n";
	return res;
}
#endif

TimeShiftedEventDescriptor::TimeShiftedEventDescriptor(descr_time_shifted_event_struct *descr)
	:Descriptor((descr_gen_t*)descr)
{
	reference_service_id=HILO(descr->reference_service_id);
	reference_event_id=HILO(descr->reference_event_id);
}

#ifdef SUPPORT_XML
eString TimeShiftedEventDescriptor::toString()
{
	eString res="<TimeShiftedEventDescriptor>";
	res+="<reference_service_id>" + qHex(reference_service_id) + "</reference_service_id>";
	res+="<reference_event_id>" + qHex(reference_event_id) + "</reference_event_id>";
	res+="</TimeShiftedEventDescriptor>\n";
	return res;
}
#endif

StreamIdentifierDescriptor::StreamIdentifierDescriptor(descr_stream_identifier_struct *descr)
	:Descriptor((descr_gen_t*)descr)
{
	component_tag=descr->component_tag;
}

#ifdef SUPPORT_XML
eString StreamIdentifierDescriptor::toString()
{
	eString res="<StreamIdentifierDescriptor>";
	res+="<component_tag>" + qHex(component_tag) + "</component_tag>";
	res+="</StreamIdentifierDescriptor>\n";
	return res;
}
#endif

CADescriptor::CADescriptor(ca_descr_t *descr)
	:Descriptor((descr_gen_t*)descr)
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

#ifdef SUPPORT_XML
eString CADescriptor::toString()
{
	eString res="<CADescriptor>";
	res+="<CA_system_ID>"+qHex(CA_system_ID)+"</CA_system_ID>";
	res+="<CA_PID>"+qHex(CA_PID)+"</CA_PID></CADescriptor>\n";
	return res;
}
#endif

NetworkNameDescriptor::NetworkNameDescriptor(descr_gen_t *descr)
	:Descriptor(descr)
{
	int len=descr->descriptor_length;
	network_name=convertDVBUTF8((unsigned char*)descr+1, len);
}

NetworkNameDescriptor::~NetworkNameDescriptor()
{
}

#ifdef SUPPORT_XML
eString NetworkNameDescriptor::toString()
{
	eString res="<NetworkNameDescriptor>";
	res+="<network_name>" + eString(network_name) + "</network_name>";
	res+="</NetworkNameDescriptor>\n";
	return res;
}
#endif

CableDeliverySystemDescriptor::CableDeliverySystemDescriptor(descr_cable_delivery_system_struct *descr)
	:Descriptor((descr_gen_t*)descr)
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

#ifdef SUPPORT_XML
eString CableDeliverySystemDescriptor::toString()
{
	eString res="<CableDeliverySystemDescriptor>";
	res+=eString().sprintf("<frequency>%d</frequency>", frequency);
	res+=eString().sprintf("<FEC_outer>%d</FEC_outer>", FEC_outer);
	res+=eString().sprintf("<modulation>QAM%d</modulation>", 8<<modulation);
	res+=eString().sprintf("<symbol_rate>%d</symbol_rate>", symbol_rate);
	res+=eString().sprintf("<FEC_inner>%d</FEC_inner>", FEC_inner);
	res+="</CableDeliverySystemDescriptor>\n";
	return res;
}
#endif

SatelliteDeliverySystemDescriptor::SatelliteDeliverySystemDescriptor(descr_satellite_delivery_system_struct *descr)
	:Descriptor((descr_gen_t*)descr)
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

#ifdef SUPPORT_XML
eString SatelliteDeliverySystemDescriptor::toString()
{
	eString res="<SatelliteDeliverySystemDescriptor>";
	res+=eString().sprintf("<frequency>%d</frequency>", frequency);
	res+=eString().sprintf("<orbital_position>%3d.%d%c</orbital_position>", orbital_position/10, orbital_position%10, west_east_flag?'E':'W');
	res+="<polarisation>";
	switch (polarisation)
	{
	case 0: res+="horizontal"; break;
	case 1: res+="vertical"; break;
	case 2: res+="left"; break;
	case 3: res+="right"; break;
	}
	res+=eString().sprintf("</polarisation><modulation>%d</modulation>", modulation);
	res+=eString().sprintf("<symbol_rate>%d</symbol_rate>", symbol_rate);
	res+=eString().sprintf("<FEC_inner>%d/%d</FEC_inner></SatelliteDeliverySystemDescriptor>\n", FEC_inner, FEC_inner+1);
	return res;
}
#endif

TerrestrialDeliverySystemDescriptor::TerrestrialDeliverySystemDescriptor(descr_terrestrial_delivery_system_struct *descr)
	:Descriptor((descr_gen_t*)descr) 
	{ 
		centre_frequency=(descr->centre_frequency1<<24)|
				(descr->centre_frequency2<<16)|
				(descr->centre_frequency3<<8)|
				(descr->centre_frequency4);
		centre_frequency*=10;
		bandwidth=descr->bandwidth;
		constellation=descr->constellation;
		hierarchy_information=descr->hierarchy_information;
		code_rate_hp_stream=descr->code_rate_hp_stream;
		code_rate_lp_stream=descr->code_rate_lp_stream;
		guard_interval=descr->guard_interval;
		transmission_mode=descr->transmission_mode;
		other_frequency_flag=descr->other_frequency_flag;
	}

TerrestrialDeliverySystemDescriptor::~TerrestrialDeliverySystemDescriptor()
{ 
}

#ifdef SUPPORT_XML
eString TerrestrialDeliverySystemDescriptor::toString()
{
	eString res=eString().sprintf("<TerrestrialDeliverySystemDescriptor><centre_frequency>%u</centre_frequency><bandwidth>", centre_frequency);
	switch (bandwidth)
	{
	case 0: res+="8MHz"; break;
	case 1: res+="7MHz"; break;
	case 2: res+="6MHz"; break;
	}
	res+="</bandwidth><constellation>";
	switch (constellation)
	{
		case 0: res+="QPSK"; break;
		case 1: res+="QAM16"; break;
		case 2: res+="QAM64"; break;
	} 
	res+="</constellation><hierarchy_information>";
	switch (hierarchy_information)
	{
		case 0: res+="none"; break;
		case 1: res+="1"; break;
		case 2: res+="2"; break;
		case 3: res+="3"; break;
	}
	res+="</hierarchy_information><code_rate_hp_stream>";
	switch (code_rate_hp_stream)
	{
		case 0: res+="1/2"; break;
		case 1: res+="2/3"; break;
		case 2: res+="3/4"; break;
		case 3: res+="5/6"; break;
		case 4: res+="7/8"; break;
	} 
	res+="</code_rate_hp_stream><code_rate_lp_stream>";
	switch (code_rate_lp_stream)
	{
		case 0: res+="1/2"; break;
		case 1: res+="2/3"; break;
		case 2: res+="3/4"; break;
		case 3: res+="5/6"; break;
		case 4: res+="7/8"; break;
	}
	res+="</code_rate_lp_stream><guard_interval>"; 
	switch (guard_interval)
	{
		case 0: res+="1/32"; break;
		case 1: res+="1/16"; break;
		case 2: res+="1/8"; break;
		case 3: res+="1/4"; break;
	}
	res+="</guard_interval><transmission_mode>";
	switch (transmission_mode)
	{
		case 0: res+="2k"; break;
		case 1: res+="8k"; break;
	}
	res+=eString().sprintf("</transmission_mode><other_frequency_flag>%d</other_frequency_flag></TerrestrialDeliverySystemDescriptor>", other_frequency_flag);
	return res;
}
#endif
									  
ServiceListDescriptorEntry::ServiceListDescriptorEntry(__u16 service_id, __u8 service_type)
	:service_id(service_id), service_type(service_type)
{
}

ServiceListDescriptorEntry::~ServiceListDescriptorEntry()
{
}

ServiceListDescriptor::ServiceListDescriptor(descr_gen_t *descr)
 :Descriptor(descr)
{
	entries.setAutoDelete(true);
	int len=descr->descriptor_length;
	for (int i=0; i<len; i+=3)
		entries.push_back(new ServiceListDescriptorEntry((((__u8*)(descr+1))[i+0]<<8) | (((__u8*)(descr+1))[i+1]), ((__u8*)(descr+1))[i+2]));
}

ServiceListDescriptor::~ServiceListDescriptor()
{
}

#ifdef SUPPORT_XML
eString ServiceListDescriptor::toString()
{
	eString res="<ServiceListDescriptor>";
	for (ePtrList<ServiceListDescriptorEntry>::iterator i(entries); i != entries.end(); ++i)
	{
		res+=eString().sprintf("<ServiceListDescriptorEntry>");
		res+=eString().sprintf("<service_id>%04x</service_id>", i->service_id);
		res+=eString().sprintf("<service_type>%04x</service_type>", i->service_type);
		res+="</ServiceListDescriptorEntry>";
	}
	res+="</ServiceListDescriptor>\n";
	return res;
}
#endif

ShortEventDescriptor::ShortEventDescriptor(descr_gen_t *descr)
	:Descriptor(descr)
{
	__u8 *data=(__u8*)descr;
	memcpy(language_code, data+2, 3);
	int ptr=5;
	int len=data[ptr++];

	int table=getEncodingTable(language_code);

	event_name=convertDVBUTF8((unsigned char*)data+ptr, len, table);
	// filter newlines in ARD ShortEventDescriptor event_name
	event_name.strReplace("\xc2\x8a",": ");
	ptr+=len;

	len=data[ptr++];

	text=convertDVBUTF8((unsigned char*) data+ptr, len, table);
}

#ifdef SUPPORT_XML
eString ShortEventDescriptor::toString()
{
	eString res="<ShortEventDescriptor>";
	res+="<event_name>"+event_name+"</event_name>";
	res+="<text>"+text+"</text>";
	res+="</ShortEventDescriptor>\n";
	return res;
}
#endif

ISO639LanguageDescriptor::ISO639LanguageDescriptor(descr_gen_t *descr)
	:Descriptor(descr)
{
	__u8 *data=(__u8*)descr;
	memcpy(language_code, data+2, 3);
	audio_type=data[5];
}

#ifdef SUPPORT_XML
eString ISO639LanguageDescriptor::toString()
{
	eString res;
	res+=eString().sprintf("<ISO639LangugageDescriptor>");
	res+=eString().sprintf("<language_code>%c%c%c</language_code>\n", language_code[0], language_code[1], language_code[2]);
	res+=eString().sprintf("<audio_type>%d</audio_type></ISO639LangugageDescriptor>\n", audio_type);
	return res;
}
#endif

AC3Descriptor::AC3Descriptor(descr_gen_t *descr)
	:Descriptor(descr)
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

#ifdef SUPPORT_XML
eString AC3Descriptor::toString()
{
	eString res="<AC3Descriptor>";
	if (AC3_type!=-1)
		res+=eString().sprintf("<AC3_type>%d</AC3_type>", AC3_type);
	if (bsid!=-1)
		res+=eString().sprintf("<bsid>%d</asvc>", bsid);
	if (mainid!=-1)
		res+=eString().sprintf("<mainid>%d</asvc>", mainid);
	if (asvc!=-1)
		res+=eString().sprintf("<asvc>%d</asvc>", asvc);
	res+="</AC3Descriptor>\n";
	return res;
}
#endif

BouquetNameDescriptor::BouquetNameDescriptor(descr_gen_t *descr)
	:Descriptor(descr)
{
	__u8 *data=(__u8*)descr;
	int len=descr->descriptor_length;
	data+=2;
	if (len > 0)
		name.assign((char*)data, len);
}

#ifdef SUPPORT_XML
eString BouquetNameDescriptor::toString()
{
	eString res="<BouquetNameDescriptor>";
	res+="<name>"+name+"</name></BouquetNameDescriptor>\n";
	return res;
}
#endif

ItemEntry::ItemEntry(eString &item_description, eString &item)
	:item_description(item_description), item(item)
{
}

ItemEntry::~ItemEntry()
{
}

ExtendedEventDescriptor::ExtendedEventDescriptor(descr_gen_t *descr)
	:Descriptor(descr)
{
	struct eit_extended_descriptor_struct *evt=(struct eit_extended_descriptor_struct *)descr;
	descriptor_number = evt->descriptor_number;
	last_descriptor_number = evt->last_descriptor_number;
	language_code[0]=evt->iso_639_2_language_code_1;
	language_code[1]=evt->iso_639_2_language_code_2;
	language_code[2]=evt->iso_639_2_language_code_3;

	int table=getEncodingTable(language_code);

	int ptr = sizeof(struct eit_extended_descriptor_struct);
	__u8* data = (__u8*) descr;

	int length_of_items=data[ptr++];
	int item_ptr=ptr;
	int item_description_len;
	int item_len;
	
	while (ptr < item_ptr+length_of_items)
	{
		eString item_description;
		eString item;
		
		item_description_len=data[ptr++];
		item_description=convertDVBUTF8((unsigned char*) data+ptr, item_description_len, table);
		ptr+=item_description_len;

		item_len=data[ptr++];
		item=convertDVBUTF8((unsigned char*) data+ptr, item_len, table);
		ptr+=item_len;
		
		items.push_back(new ItemEntry(item_description, item));
	}
	
	int text_length=data[ptr++];
	text=convertDVBUTF8((unsigned char*) data+ptr, text_length, table);
	ptr+=text_length;
}

#ifdef SUPPORT_XML
eString ExtendedEventDescriptor::toString()
{
	eString res="<ExtendedEventDescriptor>";
	res+=eString().sprintf("<language_code>%c%c%c</language_code>", language_code[0], language_code[1], language_code[2]);
	res+=eString().sprintf("<descriptor>%i</descriptor><last_descriptor_number>%i</last_descriptor_number>\n", descriptor_number, last_descriptor_number);

	for (ePtrList<ItemEntry>::iterator i(items); i != items.end(); ++i)
	{
		res+="<ItemEntry>";
		res+="<item_description>" + i->item_description + "</item_description>";
		res+="<item>" + i->item + "</item>";
		res+="</ItemEntry>";
	}
	res+="<text>"+text+"</text></ExtendedEventDescriptor>\n";
	return res;
}
#endif

ComponentDescriptor::ComponentDescriptor(descr_component_struct *descr)
	:Descriptor((descr_gen_t*)descr)
{
	int len=descr->descriptor_length+2;
	stream_content=descr->stream_content;
	component_type=descr->component_type;
	component_tag=descr->component_tag;
	language_code[0]=descr->lang_code1;
	language_code[1]=descr->lang_code2;
	language_code[2]=descr->lang_code3;
	len-=sizeof(descr_component_struct);
	if ( len > 0 )
		text.assign((char*)(descr+1), len);
}

#ifdef SUPPORT_XML
eString ComponentDescriptor::toString()
{
	eString res="<ComponentDescriptor>";
	res+=eString().sprintf("<stream_content>%d</stream_content>", stream_content);
	res+=eString().sprintf("<component_type>%d</component_type>", component_type);
	res+=eString().sprintf("<component_tag>%d</component_tag>\n", component_tag);
	res+="<text>"+text+"</text></ComponentDescriptor>\n";
	return res;
}
#endif

ContentDescriptor::ContentDescriptor(descr_gen_t *descr)
	:Descriptor(descr)
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

#ifdef SUPPORT_XML
eString ContentDescriptor::toString()
{
	eString res="<ContentDescriptor>";
	for (ePtrList<descr_content_entry_struct>::iterator it( contentList.begin() ); it != contentList.end(); it++)
		res+=eString().sprintf("nibble1 = %02x, nibble2 = %02x, user1 = %02x, user2 = %02x\n",
																	it->content_nibble_level_1, it->content_nibble_level_2, it->user_nibble_1, it->user_nibble_2 );
	res+="<!-- don't ask --></ContentDescriptor>\n";
	return res;
}	
#endif

LesRadiosDescriptor::LesRadiosDescriptor(descr_lesradios_struct *descr)
	:Descriptor((descr_gen_t*)descr)
{
	int len=descr->descriptor_length+2;
	id=descr->id;
	len-=sizeof(descr_lesradios_struct);
	if ( len > 0 )
		name.assign((char*)(descr+1), len);
}

#ifdef SUPPORT_XML
eString LesRadiosDescriptor::toString()
{
	eString res;
	res="<LesRadioDescriptor>";
	res+=eString().sprintf("<id>%d</id>", id);
	res+="<name>";
	res+=name;
	res+="</name></LesRadioDescriptor>\n";
	return res;
}
#endif

MHWDataDescriptor::MHWDataDescriptor(descr_mhw_data_struct *descr)
	:Descriptor((descr_gen_t*)descr)
{
	memcpy(type, descr->type, 8);
}

#ifdef SUPPORT_XML
eString MHWDataDescriptor::toString()
{
	eString res;
	res="<MHWDataDescriptor><data>";
	for (int i=0; i<8; i++)
		res+=type[i];
	res+="</data></MHWDataDescriptor>\n";
	return res;
}
#endif

ParentalRatingDescriptor::ParentalRatingDescriptor( descr_gen_struct *descr)
	: Descriptor((descr_gen_t*)descr)
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

#ifdef SUPPORT_XML
eString ParentalRatingDescriptor::toString()
{
	eString res="<ParentalRatingDescriptor>";
	for ( std::map<eString,int>::iterator it(entryMap.begin()); it != entryMap.end(); it++)
	{
		res += eString().sprintf("<entry><country>%s</country><age>%i</age></entry>",it->first.c_str(), it->second);
	}
	res+="</ParentalRatingDescriptor>\n";
	return res;
}
#endif

RegistrationDescriptor::RegistrationDescriptor( descr_gen_struct *descr)
	: Descriptor(descr)
{
  const char *data = ((char*)(descr+1));
	int len=descr->descriptor_length;
	if (len < 4)
		return;
	memcpy(format_identifier, data, 4);
	additional_identification_info.assign(data+4, len-4);
}

#ifdef SUPPORT_XML
eString RegistrationDescriptor::toString()
{
	eString res="<RegistrationDescriptor><format_identifier>";
	res+=eString().assign(format_identifier, 4);
	res+="</format_identifier><additional_identification_info>";
	res+=additional_identification_info;
	res+="</additional_identification_info></RegistrationDescriptor>\n";
	return res;
}
#endif

SubtitleEntry::SubtitleEntry(__u8* data)
{
	memcpy(language_code, data, 3);
	subtitling_type = *(data+3);
	composition_page_id = (*(data+4) << 8) | *(data+5);
	ancillary_page_id = (*(data+6) << 8) | *(data+7);
}

SubtitlingDescriptor::SubtitlingDescriptor(descr_gen_struct *descr)
	:Descriptor(descr)
{
	entries.setAutoDelete(true);
	int len=descr->descriptor_length;
	for (int i=0; i<len; i+=8)
		entries.push_back(new SubtitleEntry(((__u8*)descr)+2+i));
}

#ifdef SUPPORT_XML
eString SubtitlingDescriptor::toString()
{
	eString res;
	res += "<SubtitlingDescriptor>";
	for ( ePtrList<SubtitleEntry>::iterator it(entries.begin()); it != entries.end(); ++it )
		res+=eString().sprintf(
			"<ISO639_language_code>%c%c%c</ISO639_language_code>"
			"<subtitling_type>%d</subtitling_type>"
			"<composition_page_id>%d</composition_page_id>"
			"<ancillary_page_id>%d</ancillary_page_id",
			it->language_code[0], it->language_code[1], it->language_code[2],
			it->subtitling_type, it->composition_page_id, it->ancillary_page_id);
	res += "</SubtitlingDescriptor>";
	return res;
}
#endif

PAT::PAT()
	:eTable(PID_PAT, TID_PAT)
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

__u8 *PAT::getRAW()
{
	__u8 *data = new __u8[4096];
	int slen = PAT_LEN;  // 8
	data[0] = 0x00;                      // table ID;
	data[3] = (transport_stream_id >> 8);// tsid hi
	data[4] = transport_stream_id & 0xFF;// tsid lo
	data[5] = version;                   // version,cur/next
	data[6] = 0;                         // section no
	data[7] = 0;                         // last section no
	for ( ePtrList<PATEntry>::iterator it(entries);
		it != entries.end(); ++it)
	{
		data[slen++] = it->program_number >> 8;
		data[slen++] = it->program_number & 0xFF;
		data[slen++] = 0xE0 | (it->program_map_PID >> 8);
		data[slen++] = it->program_map_PID & 0xFF;
	}
	data[1] = 0xB0 | ((slen-3+4) >> 8);   // section length hi
	data[2] = (slen-3+4) & 0xFF;          // section length lo

	unsigned int crc32 = crc32_be(~0, data, slen);

	data[slen++] = crc32 >> 24;
	data[slen++] = crc32 >> 16;
	data[slen++] = crc32 >> 8;
	data[slen++] = crc32 & 0xFF;

	return data;
}

SDTEntry::SDTEntry(sdt_descr_t *descr)
{
	descriptors.setAutoDelete(true);
	service_id=HILO(descr->service_id);
	EIT_schedule_flag=descr->EIT_schedule_flag;
	EIT_present_following_flag=descr->EIT_present_following_flag;
	running_status=descr->running_status;
	free_CA_mode=descr->free_ca_mode;
	int dlen=HILO(descr->descriptors_loop_length)+SDT_DESCR_LEN;
	int ptr=SDT_DESCR_LEN;
	while (ptr<dlen)
	{
		descr_gen_t *d=(descr_gen_t*)(((__u8*)descr)+ptr);
		descriptors.push_back(Descriptor::create(d));
		ptr+=d->descriptor_length+2;
	}
}

SDT::SDT(int type)
	:eTable(PID_SDT, type?TID_SDT_OTH:TID_SDT_ACT)
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

PMT::PMT(int pid, int service_id, int version)
	:eTable(pid, TID_PMT, service_id, version), pid(pid)
{
	program_info.setAutoDelete(true);
	streams.setAutoDelete(true);
}

PMT::~PMT()
{
	for(ePtrList<__u8>::iterator i(program_infoPlain); i!=program_infoPlain.end(); ++i )
		delete [] *i;
	for(ePtrList<__u8>::iterator i(streamsPlain); i!=streamsPlain.end(); ++i )
		delete [] *i;
}

__u8* PMT::getRAW()
{
	__u8 *data = new __u8[4096];
	int slen = PMT_LEN;   // 12
	data[0] = 0x02;                      // table ID;
	data[3] = (program_number >> 8);     // prog_no hi
	data[4] = program_number & 0xFF;     // prog_no lo
	data[5] = version;                   // version,cur/next
	data[6] = 0;                         // section no
	data[7] = 0;                         // last section no
	data[8] = 0xE0 | (PCR_PID >> 8);     // PCR hi
	data[9] = PCR_PID & 0xFF;            // PCR lo
	int prog_info_len=0;
	for ( ePtrList<__u8>::iterator it(program_infoPlain);
		it != program_infoPlain.end(); ++it)
	{
		descr_gen_t *d=(descr_gen_t*)(*it);
		int len = d->descriptor_length+2;
		memcpy(data+slen, d, len);
		prog_info_len+=len;
		slen+=len;
	}
	data[10] = 0xF0 | (prog_info_len>>8); // prog_info len hi
	data[11] = prog_info_len&0xFF;        // prog_info len lo
	for ( ePtrList<__u8>::iterator it(streamsPlain);
		it != streamsPlain.end(); ++it)
	{
		int len = HILO(((pmt_info_t*)(*it))->ES_info_length)+PMT_info_LEN;
		memcpy(data+slen,*it, len);
		slen+=len;
	}
	data[1] = 0xB0 | ((slen-3+4) >> 8);   // section length hi
	data[2] = (slen-3+4) & 0xFF;          // section length lo

	unsigned int crc32 = crc32_be(~0, data, slen);

	data[slen++] = crc32 >> 24;
	data[slen++] = crc32 >> 16;
	data[slen++] = crc32 >> 8;
	data[slen++] = crc32 & 0xFF;

	return data;
}

eTable *PMT::createNext()
{
	if ( eSystemInfo::getInstance()->hasNegFilter() )
	{
		eDebug("PMT version = %d", version_number);
		return new PMT(pid, tableidext, version_number);
	}
	int newversion = incrementVersion(version);
	eDebug("PMT oldversion=%d, newversion=%d",version, newversion);
	return new PMT(pid, tableidext, newversion);
}

int PMT::data(__u8 *data)
{
	pmt_struct *pmt=(pmt_struct*)data;
	version_number=pmt->version_number;
	program_number=HILO(pmt->program_number);
	PCR_PID=HILO(pmt->PCR_PID);

	int program_info_len=HILO(pmt->program_info_length);
	int len=HILO(pmt->section_length)+3-4;
	int ptr=PMT_LEN;
	while (ptr<(program_info_len+PMT_LEN))
	{
		descr_gen_t *d=(descr_gen_t*)(data+ptr);
		int len = d->descriptor_length+2;
		program_info.push_back(Descriptor::create(d));

		// store plain data
		__u8 *plain = new __u8[len];
		memcpy(plain, data+ptr, len);
		program_infoPlain.push_back(plain);

		ptr+=len;
	}
	while (ptr<len)
	{
		int len = HILO(((pmt_info_t*)(data+ptr))->ES_info_length)+PMT_info_LEN;

		streams.push_back(new PMTEntry((pmt_info_t*)(data+ptr)));

		// store plain data
		__u8 *plain = new __u8[len];
		memcpy(plain, data+ptr, len);
		streamsPlain.push_back(plain);

		ptr+=len;
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

NIT::NIT(int pid, int type)
	:eTable(pid, type?TID_NIT_OTH:TID_NIT_ACT)
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
		int evLength=HILO(((eit_event_struct*)(data+ptr))->
			descriptors_loop_length)+EIT_LOOP_SIZE;

		events.push_back(new EITEvent((eit_event_struct*)(data+ptr)));

		// store plain data
		__u8 *plain = new __u8[evLength];
		memcpy(plain, data+ptr, evLength);
		eventsPlain.push_back(plain);

		ptr+=evLength;
	}
	return ptr!=len;
}

EIT::EIT(int type, int service_id, int ts, int version)
	:eTable(PID_EIT, ts?TID_EIT_OTH:TID_EIT_ACT, service_id, version)
	,type(type), ts(ts)
{
	events.setAutoDelete(true);
}

EIT::EIT()
{
	events.setAutoDelete(true);
}

EIT::EIT(const EIT* eit)
{
	// Vorsicht !! Hier wird autoDelete nicht auf true gesetzt...
	// Lebenszeit der Source EIT beachten !
	if (eit)
	{
		current_next_indicator = eit->current_next_indicator;
		events = eit->events;
		original_network_id = eit->original_network_id;
		service_id = eit->service_id;
		transport_stream_id = eit->transport_stream_id;
		ts = eit->ts;
		type = eit->type;
		version_number = eit->version_number;
	}
	else
	{
		current_next_indicator=0;
		original_network_id=0;
		service_id=0;
		transport_stream_id=0;
		ts=0;
		type=0;
		version_number=0;
	}
}

EIT::~EIT()
{
	for(ePtrList<__u8>::iterator i(eventsPlain); i!=eventsPlain.end(); ++i )
		delete [] *i;
}

eTable *EIT::createNext()
{
	if (ts != tsFaked)
	{
		if ( eSystemInfo::getInstance()->hasNegFilter() )
		{
			eDebug("EIT version = %d", version_number);
			return new EIT(type, service_id, ts, version_number);
		}
		int newversion = incrementVersion(version);
		eDebug("EIT oldversion=%d, newversion=%d",version, newversion);
		return new EIT(type, service_id, ts, newversion);
	}
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

TDT::TDT()
	:eTable(PID_TDT, TID_TDT)
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

BAT::BAT()
	:eTable(PID_BAT, TID_BAT)
{
	bouquet_descriptors.setAutoDelete(true);
	entries.setAutoDelete(true);
}

MHWEIT::MHWEIT(int pid, int service_id)
	:eSection(pid, 0x90, service_id, -1, 0, 0xFD)
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
	event.flags=HILO(table->flags);
	int len=30;
	while (len-- && (table->event_name[len+1]==' '))
		;

	event.event_name.append( (char*) &table->event_name[0], len);

	len=15;

	while (len-- && (table->short_description[len+1]==' '))
		;

	event.short_description.append( (char*) &table->short_description[0], len);

	if (available==3)
		return 1;

	return 0;
}

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

//	return timegm(&t);
	return my_mktime(&t)-timezone;
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
	res+="</service_name></ServiceDescriptor>";
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
	res+="</CAIdentifier>";
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
	res+="</LinkageDescriptor>";
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
	res+="</NVODReferenceDescriptor>";
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
	res+="</TimeShiftedServiceDescriptor>";
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
	res+="</TimeShiftedEventDescriptor>";
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
	res+="</StreamIdentifierDescriptor>";
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
	res+="<CA_PID>"+qHex(CA_PID)+"</CA_PID></CADescriptor>";
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
	res+="</NetworkNameDescriptor>";
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
	res+=eString().sprintf("<frequency>%d</frequency>\n", frequency);
	res+=eString().sprintf("<FEC_outer>%d</FEC_outer>", FEC_outer);
	res+=eString().sprintf("<modulation>QAM%d</modulation>", 8<<modulation);
	res+=eString().sprintf("<symbol_rate>%d</symbol_rate>", symbol_rate);
	res+=eString().sprintf("<FEC_inner>%d</FEC_inner>", FEC_inner);
	res+="</CableDeliverySystemDescriptor>";
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
	case 0: res+="horizontal\n"; break;
	case 1: res+="vertical\n"; break;
	case 2: res+="left\n"; break;
	case 3: res+="right"; break;
  }
	res+=eString().sprintf("</polarisation><modulation>%d</modulation>", modulation);
	res+=eString().sprintf("<symbol_rate>%d</symbol_rate>", symbol_rate);
	res+=eString().sprintf("<FEC_inner>%d/%d</FEC_inner></SatelliteDeliverySystemDescriptor>", FEC_inner, FEC_inner+1);
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
	res+="</ServiceListDescriptor>";
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

	if (len && (data[ptr]<0x20))			// ignore charset
	{
		ptr++;
		len--;
	}
	
	int table=5;
	if (!memcmp(language_code, "gre", 3))
		table=3;
	if (!memcmp(language_code, "ru", 2))
		table=1;

	event_name=convertDVBUTF8((unsigned char*)data+ptr, len, table);
	// filter newlines in ARD ShortEventDescriptor event_name
	size_t pos = event_name.find("\xc2\x8a");
	if ( pos != eString::npos )
	{
		event_name[pos]=':';
		event_name[pos+1]=' ';
	}
	ptr+=len;

	len=data[ptr++];

	if (len && (data[ptr]<0x20))			// ignore charset
	{
		ptr++;
		len--;
	}

	text=convertDVBUTF8((unsigned char*) data+ptr, len, table);
}

#ifdef SUPPORT_XML
eString ShortEventDescriptor::toString()
{
	eString res="<ShortEventDescriptor>";
	res+="<event_name>"+event_name+"</event_name>";
	res+="<text>"+text+"</text>";
	res+="</ShortEventDescriptor>";
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
	res+=eString().sprintf("<audio_type>%d</audio_type></ISO639LangugageDescriptor>", audio_type);
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
	res+="</AC3Descriptor>";
	return res;
}
#endif

BouquetNameDescriptor::BouquetNameDescriptor(descr_gen_t *descr)
	:Descriptor(descr)
{
	__u8 *data=(__u8*)descr;
	int len=descr->descriptor_length;
	data+=2;
	name="";
	while (len--)
		name+=*data++;
}

#ifdef SUPPORT_XML
eString BouquetNameDescriptor::toString()
{
	eString res="<BouquetNameDescriptor>";
	res+="<name>"+name+"</name></BouquetNameDescriptor>";
	return res;
}
#endif

ExtendedEventDescriptor::ExtendedEventDescriptor(descr_gen_t *descr)
	:Descriptor(descr)
{
	struct eit_extended_descriptor_struct *evt=(struct eit_extended_descriptor_struct *)descr;
	descriptor_number = evt->descriptor_number;
	last_descriptor_number = evt->last_descriptor_number;
	item_description_length = evt->item_description_length;
	language_code[0]=evt->iso_639_2_language_code_1;
	language_code[1]=evt->iso_639_2_language_code_2;
	language_code[2]=evt->iso_639_2_language_code_3;

	int table=5;
	if (!memcmp(language_code, "gre", 3))
		table=3;
	if (!memcmp(language_code, "ru", 2))
		table=1;

	int ptr = sizeof(struct eit_extended_descriptor_struct);
	__u8* data = (__u8*) descr;
	item_description=convertDVBUTF8((unsigned char*)data+ptr, item_description_length, table);
}

#ifdef SUPPORT_XML
eString ExtendedEventDescriptor::toString()
{
	eString res="<ExtendedEventDescriptor>";
	res+=eString().sprintf("<language_code>%c%c%c</language_code>", language_code[0], language_code[1], language_code[2]);
	res+=eString().sprintf("<descriptor>%i</descriptor><last_descriptor_number>%i</last_descriptor_number>\n", descriptor_number, last_descriptor_number);
	res+="<description>"+item_description+"</description></ExtendedEventDescriptor>";
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
	__u8 *p=(__u8*)(descr+1);
	text="";
	while (len--)
		text+=*p++;
}

#ifdef SUPPORT_XML
eString ComponentDescriptor::toString()
{
	eString res="<ComponentDescriptor>";
	res+=eString().sprintf("<stream_content>%d</stream_content>", stream_content);
	res+=eString().sprintf("<component_type>%d</component_type>", component_type);
	res+=eString().sprintf("<component_tag>%d</component_tag>\n", component_tag);
	res+="<text>"+text+"</text></ComponentDescriptor>";
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
	res+="<!-- don't ask --></ContentDescriptor>";
	return res;
}	
#endif

LesRadiosDescriptor::LesRadiosDescriptor(descr_lesradios_struct *descr)
	:Descriptor((descr_gen_t*)descr)
{
	int len=descr->descriptor_length+2;
	id=descr->id;
	len-=sizeof(descr_lesradios_struct);
	char *lname=(char*)(descr+1);
	name="";
	while (len--)
		name+=*lname++;
}

#ifdef SUPPORT_XML
eString LesRadiosDescriptor::toString()
{
	eString res;
	res="<LesRadioDescriptor>";
	res+=eString().sprintf("<id>%d</id>", id);
	res+="<name>";
	res+=name;
	res+="</name></LesRadioDescriptor>";
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
	res+="</ParentalRatingDescriptor>";
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
	res+="</additional_identification_info></RegistrationDescriptor>";
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
		events.push_back(new EITEvent((eit_event_struct*)(data+ptr)));
		ptr+=HILO(((eit_event_struct*)(data+ptr))->descriptors_loop_length)+EIT_LOOP_SIZE;
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
	event.event_name="";
	event.flags=HILO(table->flags);
	int len=30;
	while (len-- && (table->event_name[len+1]==' '))
		;

	event.event_name.append( (char*) &table->event_name[0], len);

	len=15;
	event.short_description="";

	while (len-- && (table->short_description[len+1]==' '))
		;

	event.short_description.append( (char*) &table->short_description[0], len);

	if (available==3)
		return 1;

	return 0;
}

#ifndef __si_h
#define __si_h

#include <vector>

#define SUPPORT_XML
#include <lib/dvb/esection.h>
#include <lib/base/estring.h>
#include <lib/base/eerror.h>
#include <lib/base/eptrlist.h>
#include <lib/dvb/lowlevel/sdt.h>
#include <lib/dvb/lowlevel/descr.h>
#include <lib/dvb/lowlevel/ca.h>
#include <lib/dvb/lowlevel/pmt.h>
#include <lib/dvb/lowlevel/nit.h>
#include <lib/dvb/lowlevel/eit.h>
#include <lib/dvb/lowlevel/bat.h>

time_t parseDVBtime(__u8 t1, __u8 t2, __u8 t3, __u8 t4, __u8 t5);
int fromBCD(int bcd);

class Descriptor
{
		// better this fixed length or heap memory? don't know :/
  __u8 data[256];
  int len;
public:
	inline Descriptor(descr_gen_t *descr)
	{
		len = descr->descriptor_length;
		memcpy(data, descr, len);
	};
	inline virtual ~Descriptor(){};

	static Descriptor *create(descr_gen_t *data);
	int Tag() { return data[0]; }

#ifdef SUPPORT_XML	
	eString toXML();
	/*
		<descriptor>
			<raw>...</raw>
			<parsed>
				<ComponentDescriptor>
					...
				</ComponentDescriptor>
			</parsed>
		</descriptor>
	*/
  virtual eString toString()=0;
#endif
};

class UnknownDescriptor: public Descriptor
{
public:
  UnknownDescriptor(descr_gen_t *descr);
  ~UnknownDescriptor();                                     	
  eString toString();
};

class ServiceDescriptor: public Descriptor
{
public:
  int service_type;
  eString service_provider, service_name;
  static const int CTag() { return DESCR_SERVICE; }
  ServiceDescriptor(sdt_service_desc *descr);
  ~ServiceDescriptor();
  eString toString();
};

class CAIdentifierDescriptor: public Descriptor
{
public:
  __u16 *CA_system_id;
  int CA_system_ids;
  CAIdentifierDescriptor(descr_gen_t *descr);
  ~CAIdentifierDescriptor();
  eString toString();
};

class LinkageDescriptor: public Descriptor
{
public:
  int transport_stream_id;
  int original_network_id;
  int service_id;
  int linkage_type;

  __u8 *private_data;
  int priv_len;

  int handover_type;
  int origin_type;

  int network_id;
  int initial_service_id;

  LinkageDescriptor(descr_linkage_struct *descr);
  ~LinkageDescriptor();
  eString toString();
};

class NVODReferenceEntry
{
public:
	bool operator==( const NVODReferenceEntry &e )
	{
		return e.transport_stream_id == transport_stream_id
				&& e.original_network_id == original_network_id
				&& e.service_id == service_id;
	}
  __u16 transport_stream_id, original_network_id, service_id;
  NVODReferenceEntry(__u16 transport_stream_id, __u16 original_network_id, __u16 service_id);
  ~NVODReferenceEntry();
};

class NVODReferenceDescriptor: public Descriptor
{
public:
  NVODReferenceDescriptor(descr_gen_t *descr);
  ~NVODReferenceDescriptor();
  eString toString();

  ePtrList<NVODReferenceEntry> entries;
};

class TimeShiftedServiceDescriptor: public Descriptor
{
public:
  int reference_service_id;
  TimeShiftedServiceDescriptor(descr_time_shifted_service_struct *descr);
  eString toString();
};

class TimeShiftedEventDescriptor: public Descriptor
{
public:
  int reference_service_id;
  int reference_event_id;
  TimeShiftedEventDescriptor(descr_time_shifted_event_struct *descr);
  eString toString();
};


class StreamIdentifierDescriptor: public Descriptor
{
public:
  int component_tag;
  StreamIdentifierDescriptor(descr_stream_identifier_struct *descr);
  eString toString();
};

class CADescriptor: public Descriptor
{
public:
  __u16 CA_system_ID, CA_PID;
  __u8 *data;
  CADescriptor(ca_descr_t *descr);
  ~CADescriptor();
  eString toString();
};

class NetworkNameDescriptor: public Descriptor
{
public:
	eString network_name;
  NetworkNameDescriptor(descr_gen_t *descr);
  ~NetworkNameDescriptor();
  eString toString();
};

class CableDeliverySystemDescriptor: public Descriptor
{
public:
  __u32 frequency;
  int FEC_outer, modulation, symbol_rate, FEC_inner;
  CableDeliverySystemDescriptor(descr_cable_delivery_system_struct *descr);
  ~CableDeliverySystemDescriptor();
  eString toString();
};

class SatelliteDeliverySystemDescriptor: public Descriptor
{
public:
  __u32 frequency;
  __u16 orbital_position;
  int west_east_flag;
  int polarisation;
  int modulation;
  __u32 symbol_rate;
  int FEC_inner;
  SatelliteDeliverySystemDescriptor(descr_satellite_delivery_system_struct *descr);
  ~SatelliteDeliverySystemDescriptor();
  eString toString();
};

class TerrestrialDeliverySystemDescriptor: public Descriptor
{
public:
  __u32 centre_frequency;
  int bandwidth;
  int constellation;
  int hierarchy_information;
  int code_rate_hp_stream;
  int code_rate_lp_stream;
  int guard_interval;
  int transmission_mode;
  int other_frequency_flag;
  TerrestrialDeliverySystemDescriptor(descr_terrestrial_delivery_system_struct *descr);
  ~TerrestrialDeliverySystemDescriptor();
  eString toString();
};

class ServiceListDescriptorEntry
{
public:
  ServiceListDescriptorEntry(__u16 service_id, __u8 service_type);
  ~ServiceListDescriptorEntry();

  __u16 service_id;
  __u8 service_type;
};

class ServiceListDescriptor: public Descriptor
{
public:
  ServiceListDescriptor(descr_gen_t *descr);
  ~ServiceListDescriptor();
  eString toString();

  ePtrList<ServiceListDescriptorEntry> entries;
};

class ShortEventDescriptor: public Descriptor
{
public:
	ShortEventDescriptor(descr_gen_t *descr);
	ShortEventDescriptor(): Descriptor((descr_gen_t*)"\x4d") { };
	eString toString();
	char language_code[3];
	eString event_name;
	eString text;
};

class ISO639LanguageDescriptor: public Descriptor
{
public:
	ISO639LanguageDescriptor(descr_gen_t *descr);
	eString toString();
	char language_code[3];
	int audio_type;
};

class AC3Descriptor: public Descriptor
{
public:
	AC3Descriptor(descr_gen_t *descr);
	eString toString();
	int AC3_type, bsid, mainid, asvc;
};

class BouquetNameDescriptor: public Descriptor
{
public:
	BouquetNameDescriptor(descr_gen_t *descr);
	eString toString();
	eString name;
};

class ItemEntry
{
public:
	eString item_description;
	eString item;
	ItemEntry(eString &item_description, eString &item);
	~ItemEntry();
};

class ExtendedEventDescriptor: public Descriptor
{
public:
	ExtendedEventDescriptor(descr_gen_t *descr);
	eString toString();
	int descriptor_number;
	int last_descriptor_number;
	char language_code[3];
	ePtrList< ItemEntry > items;
	eString text;
};

class ComponentDescriptor: public Descriptor
{
public:
	ComponentDescriptor(descr_component_struct *descr);
	eString toString();

	int stream_content, component_type, component_tag;
	char language_code[3];
	eString text;
};

class ContentDescriptor: public Descriptor
{
public:
	ContentDescriptor(descr_gen_t *descr);
	ePtrList< descr_content_entry_struct > contentList;	
	eString toString();
};

class LesRadiosDescriptor: public Descriptor
{
public:
	LesRadiosDescriptor(descr_lesradios_struct *descr);
	eString toString();
	
	int id;
	eString name;
};

class MHWDataDescriptor: public Descriptor
{
public:
	MHWDataDescriptor(descr_mhw_data_struct *desrc);
	eString toString();
	
	char type[8];
};

class ParentalRatingDescriptor: public Descriptor
{
public:
	ParentalRatingDescriptor(descr_gen_struct *descr);
	eString toString();
	std::map< eString, int > entryMap; // Country Code : age
};

class RegistrationDescriptor: public Descriptor
{
public:
	RegistrationDescriptor(descr_gen_struct *descr);
	eString toString();
	char format_identifier[4];
	eString additional_identification_info;
};

class PATEntry
{
public:
	PATEntry(int program_number, int program_map_PID): program_number(program_number), program_map_PID(program_map_PID)
	{
	}
	int program_number;
	int program_map_PID;
};

class PAT: public eTable
{
protected:
	int data(__u8 *data);
public:
	PAT();

	PATEntry *searchService(int service_id)
	{
		for (ePtrList<PATEntry>::iterator i(entries); i != entries.end(); ++i)
			if (i->program_number==service_id)
				return *i;
		return 0;
	}

	int transport_stream_id;
	ePtrList<PATEntry> entries;

	__u8 *getRAW();
};

class SDTEntry
{
public:
	SDTEntry(sdt_descr_t *descr);
	
	int service_id;
	int EIT_schedule_flag;
	int EIT_present_following_flag;
	int running_status;
	int free_CA_mode;
	ePtrList< Descriptor > descriptors;
};

class SDT: public eTable
{
protected:
	int data(__u8 *data);
public:
	enum { typeActual=0, typeOther=1 };
	SDT(int type=typeActual);

	int transport_stream_id, original_network_id;
	ePtrList<SDTEntry> entries;
};

class PMTEntry
{
public:
	PMTEntry(pmt_info_t* info);
	int stream_type;
	int elementary_PID;
	ePtrList< Descriptor > ES_info;
};

class PMT: public eTable
{
protected:
	int data(__u8 *data);
public:
	PMT(int pid, int service_id, int version=-1);
	~PMT();

	int program_number, PCR_PID, pid, version_number;

	eTable *createNext();
	ePtrList< Descriptor > program_info;
	ePtrList<__u8> program_infoPlain;

	ePtrList<PMTEntry> streams;
	ePtrList<__u8> streamsPlain;

	__u8* getRAW();
};

class NITEntry
{
public:
	NITEntry(nit_ts_t* ts);

	__u16 transport_stream_id, original_network_id;
	ePtrList< Descriptor > transport_descriptor;
};

class NIT: public eTable
{
protected:
	int data(__u8 *data);
public:
	enum
	{
		typeActual=0, typeOther
	};
	NIT(int pid, int type=0);
	int network_id;
	ePtrList<NITEntry> entries;
	ePtrList< Descriptor > network_descriptor;
};

class EITEvent
{
public:
	EITEvent(const eit_event_struct *event);
	EITEvent();
	int event_id;
	time_t start_time;
	int duration;
	int running_status;
	int free_CA_mode;
	ePtrList< Descriptor > descriptor;
};

class EIT: public eTable
{
protected:
	int data(__u8 *data);
public:
	enum
	{
		tsActual=0, tsOther, tsFaked
	};
	enum
	{
		typeNowNext=0, typeSchedule
	};
	
	EIT(int type, int service_id=-1, int ts=tsActual, int version=-1);
	EIT( const EIT* eit );
	EIT();
	~EIT();
	eTable *createNext();
	
	int type, ts, service_id, version_number, current_next_indicator, transport_stream_id, original_network_id;
	ePtrList<EITEvent> events;
	ePtrList<__u8> eventsPlain;
};

class TDT: public eTable
{
protected:
	int data(__u8 *data);
public:
	TDT();
	
	time_t UTC_time;
};

class BATEntry
{
public:
	BATEntry(bat_loop_struct *data);
	int transport_stream_id, original_network_id;
	ePtrList< Descriptor > transport_descriptors;
};

class BAT: public eTable
{
protected:
	int data(__u8 *data);
public:
	BAT();
	
	int bouquet_id;
	ePtrList< Descriptor > bouquet_descriptors;
	ePtrList<BATEntry> entries;
};

class MHWEITEvent
{
public:
	int service_id;
	int starttime;
	int duration;
	eString event_name;
	eString short_description;
	eString extended_description;
	int flags;
};

class MHWEIT: public eSection
{
	int sectionRead(__u8 *data);
	int available;
	void sectionFinish(int);
public:
	Signal1<void, int> ready;
	MHWEIT(int pid, int service_id);
	std::vector<MHWEITEvent> events;
};

#endif

#ifndef __si_h
#define __si_h

#include "esection.h"
#include <qlist.h>
#include "lowlevel/sdt.h"
#include "lowlevel/descr.h"
#include "lowlevel/ca.h"
#include "lowlevel/pmt.h"
#include "lowlevel/nit.h"
#include "lowlevel/eit.h"
#include "lowlevel/bat.h"

class Descriptor: public QObject
{
	Q_OBJECT
public:
	static Descriptor *create(descr_gen_t *data);
	Descriptor(int tag);
	int Tag() { return tag; }
	virtual QString toString()=0;
	virtual ~Descriptor();
	
	int tag;
};

class UnknownDescriptor: public Descriptor
{
public:
  __u8 *data;
  int len;
  UnknownDescriptor(descr_gen_t *descr);
  ~UnknownDescriptor();
  QString toString();
};

class ServiceDescriptor: public Descriptor
{
public:
  int service_type; 
  char *service_provider, *service_name;
  static const int CTag() { return DESCR_SERVICE; }
  ServiceDescriptor(sdt_service_desc *descr);
  ~ServiceDescriptor();
  QString toString();
};

class CAIdentifierDescriptor: public Descriptor
{
public:
  __u16 *CA_system_id;
  int CA_system_ids;
  CAIdentifierDescriptor(descr_gen_t *descr);
  ~CAIdentifierDescriptor();
  QString toString();
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
  QString toString();
};

class NVODReferenceEntry
{
public:
  __u16 transport_stream_id, original_network_id, service_id;
  NVODReferenceEntry(__u16 transport_stream_id, __u16 original_network_id, __u16 service_id);
  ~NVODReferenceEntry();
};

class NVODReferenceDescriptor: public Descriptor
{
public:
  NVODReferenceDescriptor(descr_gen_t *descr);
  ~NVODReferenceDescriptor();
  QString toString();
  
  QList<NVODReferenceEntry> entries;
};

class TimeShiftedServiceDescriptor: public Descriptor
{
public:
  int reference_service_id;
  TimeShiftedServiceDescriptor(descr_time_shifted_service_struct *descr);
  QString toString();
};

class StreamIdentifierDescriptor: public Descriptor
{
public:
  int component_tag;
  StreamIdentifierDescriptor(descr_stream_identifier_struct *descr);
  QString toString();
};

class CADescriptor: public Descriptor
{
public:
  __u16 CA_system_ID, CA_PID;
  __u8 *data;
  CADescriptor(ca_descr_t *descr);
  ~CADescriptor();
  QString toString();
};

class NetworkNameDescriptor: public Descriptor
{
public:
	char *network_name;
  NetworkNameDescriptor(descr_gen_t *descr);
  ~NetworkNameDescriptor();
  QString toString();
};

class CableDeliverySystemDescriptor: public Descriptor
{
public:
  __u32 frequency;
  int FEC_outer, modulation, symbol_rate, FEC_inner;
  CableDeliverySystemDescriptor(descr_cable_delivery_system_struct *descr);
  ~CableDeliverySystemDescriptor();
  QString toString();
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
  QString toString();
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
  QString toString();
  
  QList<ServiceListDescriptorEntry> entries;
};

class ShortEventDescriptor: public Descriptor
{
public:
	ShortEventDescriptor(descr_gen_t *descr);
	QString toString();
	char language_code[3];
	QString event_name;
	QString text;
};

class ISO639LanguageDescriptor: public Descriptor
{
public:
	ISO639LanguageDescriptor(descr_gen_t *descr);
	QString toString();
	char language_code[3];
	int audio_type;
};

class AC3Descriptor: public Descriptor
{
public:
	AC3Descriptor(descr_gen_t *descr);
	QString toString();
	int AC3_type, bsid, mainid, asvc;
};

class BouquetNameDescriptor: public Descriptor
{
public:
	BouquetNameDescriptor(descr_gen_t *descr);
	QString toString();
	QString name;
};

class ExtendedEventDescriptor: public Descriptor
{
public:
	ExtendedEventDescriptor(descr_gen_t *descr);
	QString toString();
	int descriptor_number;
	int last_descriptor_number;
	char language_code[3];
	int item_description_length;
	QString item_description;
};

class ComponentDescriptor: public Descriptor
{
public:
	ComponentDescriptor(descr_component_struct *descr);
	QString toString();

	int stream_content, component_type, component_tag;
	char language_code[3];
	QString text;
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
	Q_OBJECT
protected:
	int data(__u8 *data);
public:
	PAT();

	PATEntry *searchService(int service_id)
	{
		for (QListIterator<PATEntry> i(entries); i.current(); ++i)
			if (i.current()->program_number==service_id)
				return i.current();
		return 0;
	}

	int transport_stream_id;
	QList<PATEntry> entries;
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
	QList<Descriptor> descriptors;
};

class SDT: public eTable
{
	Q_OBJECT
protected:
	int data(__u8 *data);
public:
	enum { typeActual=0, typeOther=1 };
	SDT(int type=typeActual);

	int transport_stream_id, original_network_id;
	QList<SDTEntry> entries;
};

class PMTEntry
{
public:
	PMTEntry(pmt_info_t* info);
	int stream_type;
	int elementary_PID;
	QList<Descriptor> ES_info;
};

class PMT: public eTable
{
protected:
  int data(__u8 *data);
public:
  PMT(int pid, int service_id);
  PMT::~PMT() { } ;

  int program_number;
  int PCR_PID;
  int pid;
  QList<Descriptor> program_info;
  QList<PMTEntry> streams;
};

class NITEntry
{
public:
  NITEntry(nit_ts_t* ts);

	__u16 transport_stream_id, original_network_id;
	QList<Descriptor> transport_descriptor;
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
	QList<NITEntry> entries;
	QList<Descriptor> network_descriptor;
};

class EITEvent
{
public:
	EITEvent(eit_event_struct *event);
	int event_id;
	time_t start_time;
	int duration;
	int running_status;
	int free_CA_mode;
	QList<Descriptor> descriptor;
};

class EIT: public eTable
{
protected:
	int data(__u8 *data);
public:
	enum
	{
		tsActual=0, tsOther
	};
	enum
	{
		typeNowNext=0, typeSchedule
	};
	
	EIT(int type=typeNowNext, int service_id=-1, int ts=tsActual, int version=-1);
	eTable *createNext();
	
	int type, ts, service_id, version_number, current_next_indicator, transport_stream_id, original_network_id;
	QList<EITEvent> events;
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
	QList<Descriptor> transport_descriptors;
};

class BAT: public eTable
{
protected:
	int data(__u8 *data);
public:
	BAT();
	
	int bouquet_id;
	QList<Descriptor> bouquet_descriptors;
	QList<BATEntry> entries;
};

#endif

#include <dvbsi++/network_name_descriptor.h>
#include <dvbsi++/service_list_descriptor.h>
#include <dvbsi++/stuffing_descriptor.h>
#include <dvbsi++/satellite_delivery_system_descriptor.h>
#include <dvbsi++/cable_delivery_system_descriptor.h>
#include <dvbsi++/vbi_data_descriptor.h>
#include <dvbsi++/vbi_teletext_descriptor.h>
#include <dvbsi++/bouquet_name_descriptor.h>
#include <dvbsi++/service_descriptor.h>
#include <dvbsi++/country_availability_descriptor.h>
#include <dvbsi++/linkage_descriptor.h>
#include <dvbsi++/nvod_reference_descriptor.h>
#include <dvbsi++/time_shifted_service_descriptor.h>
#include <dvbsi++/short_event_descriptor.h>
#include <dvbsi++/extended_event_descriptor.h>
//#include <dvbsi++/time_shifted_event_descriptor.h>
#include <dvbsi++/component_descriptor.h>
#include <dvbsi++/mosaic_descriptor.h>
#include <dvbsi++/stream_identifier_descriptor.h>
#include <dvbsi++/ca_identifier_descriptor.h>
#include <dvbsi++/content_descriptor.h>
#include <dvbsi++/parental_rating_descriptor.h>
#include <dvbsi++/teletext_descriptor.h>
#include <dvbsi++/telephone_descriptor.h>
#include <dvbsi++/local_time_offset_descriptor.h>
#include <dvbsi++/subtitling_descriptor.h>
#include <dvbsi++/terrestrial_delivery_system_descriptor.h>
#include <dvbsi++/multilingual_network_name_descriptor.h>
#include <dvbsi++/multilingual_bouquet_name_descriptor.h>
#include <dvbsi++/multilingual_service_name_descriptor.h>
#include <dvbsi++/multilingual_component_descriptor.h>
#include <dvbsi++/private_data_specifier_descriptor.h>
#include <dvbsi++/service_move_descriptor.h>
//#include <dvbsi++/short_smoothing_buffer_descriptor.h>
#include <dvbsi++/frequency_list_descriptor.h>
//#include <dvbsi++/partial_transport_stream_descriptor.h>
#include <dvbsi++/data_broadcast_descriptor.h>
#include <dvbsi++/data_broadcast_id_descriptor.h>
//#include <dvbsi++/transport_stream_descriptor.h>
//#include <dvbsi++/dsng_descriptor.h>
#include <dvbsi++/pdc_descriptor.h>
#include <dvbsi++/ac3_descriptor.h>
#include <dvbsi++/ancillary_data_descriptor.h>
#include <dvbsi++/cell_list_descriptor.h>
#include <dvbsi++/cell_frequency_link_descriptor.h>
#include <dvbsi++/announcement_support_descriptor.h>
#include <dvbsi++/application_signalling_descriptor.h>
//#include <dvbsi++/adaptation_field_data_descriptor.h>
//#include <dvbsi++/service_identifier_descriptor.h>
//#include <dvbsi++/service_availability_descriptor.h>

/* Constant */

const char bytes[] = { 0x5A, 0xA5, 0x55, 0xAA };
const std::string stringTest = "STRING_TEST";
const std::string eng = "eng";
const std::string fra = "fra";
const std::string deu = "deu";
const std::string stringTestEng = "STRING_TEST_ENG";
const std::string stringTestFra = "STRING_TEST_FRA";
const std::string stringTestDeu = "STRING_TEST_DEU";

/* General purpose descriptor */

char nullDescriptor[] =
{
	0xFF, // Dummy tag
	0x00  // Length
};

char oneByteDescriptor[] =
{
	0xFF, // Dummy tag
	0x01, // Length
	0x5A  // Byte
};

char twoBytesDescriptor[] =
{
	0xFF, // Dummy tag
	0x01, // Length
	0x5A, // Byte
	0xA5  // Byte
};

char threeBytesDescriptor[] =
{
	0xFF, // Dummy tag
	0x01, // Length
	0x5A, // Byte
	0xA5, // Byte
	0x55  // Byte
};

char fourBytesDescriptor[] =
{
	0xFF, // Dummy tag
	0x01, // Length
	0x5A, // Byte
	0xA5, // Byte
	0x55, // Byte
	0xAA  // Byte
};

char stringDescriptor[] =
{
	0xFF, // Dummy tag
	0x0B, // Length
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54 // String = STRING_TEST
};

char multilingualStringDescriptor[] =
{
	0xFF, // Dummy tag
	0x39, // Length
	0x65, 0x6E, 0x67, // Iso639LangageCode = eng
	0x0F, // StringLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, 0x5F, 0x45, 0x4E, 0x47, // String = STRING_TEST_ENG
	0x66, 0x72, 0x61, // Iso639LangageCode = fra
	0x0F, // StringLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, 0x5F, 0x46, 0x52, 0x41, // String = STRING_TEST_FRA
	0x64, 0x65, 0x75, // Iso639LangageCode = deu
	0x0F, // StringLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, 0x5F, 0x44, 0x45, 0x55  // String = STRING_TEST_DEU
};


/* SI descriptors */

/* networkNameDescriptor */
char *networkNameDescriptor1 = stringDescriptor;
char *networkNameDescriptor2 = nullDescriptor;

/* serviceListDescriptor */
char serviceListDescriptor1[] =
{
	0x41, // Tag
	0x09, // Length
	0x12, 0x34, // ServiceId
	0x56, // ServiceType
	0x78, 0x9A, // ServiceId
	0xBC, // ServiceType
	0xDE, 0xF0, // ServiceId
	0x12, // ServiceType
};
char *serviceListDescriptor2 = nullDescriptor;

/* stuffingDescriptor */
char *stuffingDescriptor1 = fourBytesDescriptor;
char *stuffingDescriptor2 = nullDescriptor;

/* satelliteDeliverySystemDescriptor */
char satelliteDeliverySystemDescriptor1[] =
{
	0x43, // Tag
	0x0B, // Length
	0x12, 0x34, 0x56, 0x78, // Frequency
	0x9A, 0xBC, // Orbital
	0xDE, // WEFlag, Polarisation, Modulation
	0x10, 0x12, 0x34, 0x56, // SymbolRate, FecInner
};

/* cableDeliverySystemDescriptor */
char cableDeliverySystemDescriptor1[] =
{
	0x44, // Tag
	0x0B, // Length
	0x12, 0x34, 0x56, 0x78, // Frequency
	0x9A, 0xBC, // Reserved, FecOuter
	0xDE, // Modulation
	0x10, 0x12, 0x34, 0x56, // SymbolRate, FecInner
};

/* vbiDataDescriptor */
char vbiDataDescriptor1[] =
{
	0x45, // Tag
	0x1A, // Length

	0x01, // DataServiceId
	0x03, // DataServiceLength
	0x12, 0x34, 0x56, // FieldParity, LineOffset (x3)

    0x02, // DataServiceId
	0x02, // DataServiceLength
	0x78, 0x9A, // FieldParity, LineOffset (x2)

	0x04, // DataServiceId
	0x01, // DataServiceLength
	0xBC, // FieldParity, LineOffset (x1)

    0x05, // DataServiceId
	0x00, // DataServiceLength

	0x06, // DataServiceId
	0x01, // DataServiceLength
	0xDE, // FieldParity, LineOffset (x1)

	0x07, // DataServiceId
	0x02, // DataServiceLength
	0xF0, 0x12, // FieldParity, LineOffset (x2)

	0x55, // DataServiceId
	0x03, // DataServiceLength
	0x34, 0x56, 0x78, // FieldParity, LineOffset (x3)
};
char *vbiDataDescriptor2 = nullDescriptor;

/* vbiTeletextDescriptor */
char vbiTeletextDescriptor1[] =
{
	0x46, // Tag
	0x0F, // Length

	0x65, 0x6E, 0x67, // Iso639LangageCode = eng
	0x12, // TeletextType, TeletextMagazineNumber
	0x34, // TeletextMagazineNumber

	0x66, 0x72, 0x61, // Iso639LangageCode = fra
	0x56, // TeletextType, TeletextMagazineNumber
	0x78, // TeletextMagazineNumber

	0x64, 0x65, 0x75, // Iso639LangageCode = deu
	0x9A, // TeletextType, TeletextMagazineNumber
	0xBC, // TeletextMagazineNumber
};
char *vbiTeletextDescriptor2 = nullDescriptor;

/* bouquetNameDescriptor = stringDescriptor */
char *bouquetNameDescriptor1 = stringDescriptor;
char *bouquetNameDescriptor2 = nullDescriptor;

/* serviceDescriptor */
char serviceDescriptor1[] =
{
	0x48, // Tag
	0x19, // Length
	0x01, // ServiceType
	0x0B, // ServiceProviderNameLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // ServiceProviderName = STRING_TEST
    0x0B, // ServiceNameLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // ServiceName = STRING_TEST
};
char serviceDescriptor2[] =
{
	0x48, // Tag
	0x0E, // Length
	0x02, // ServiceType
	0x00, // ServiceProviderNameLength
	0x0B, // ServiceNameLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // ServiceName = STRING_TEST
};
char serviceDescriptor3[] =
{
	0x48, // Tag
	0x0E, // Length
	0x03, // ServiceType
	0x0B, // ServiceProviderNameLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // ServiceProviderName = STRING_TEST
	0x00, // ServiceNameLength
};
char serviceDescriptor4[] =
{
	0x48, // Tag
	0x03, // Length
	0x04, // ServiceType
	0x00, // ServiceProviderNameLength
	0x00, // ServiceNameLength
};

/* countryAvailabilityDescriptor */
char countryAvailabilityDescriptor1[] =
{
	0x49, // Tag
	0x0A, // Length
	0x80, // CountryAvailabilityFlag
	0x65, 0x6E, 0x67, // Iso639LangageCode = eng
	0x66, 0x72, 0x61, // Iso639LangageCode = fra
	0x64, 0x65, 0x75, // Iso639LangageCode = deu
};
char countryAvailabilityDescriptor2[] =
{
	0x49, // Tag
	0x01, // Length
	0x00, // CountryAvailabilityFlag
};

/* linkageDescriptor */
char linkageDescriptor1[] =
{
	0x4A, // Tag
	0x0C, // Length
	0x12, 0x34, // TransportStreamId
	0x56, 0x78, // OriginalNetworkId
	0x9A, 0xBC, // ServiceId
	0x08, // LinkageType
	0x10, // HandOverType, OriginType
	0x12, 0x34, // NetworkId
	0x56, 0x78, // InitialServiceId
};
char linkageDescriptor2[] =
{
	0x4A, // Tag
	0x0A, // Length
	0x12, 0x34, // TransportStreamId
	0x56, 0x78, // OriginalNetworkId
	0x9A, 0xBC, // ServiceId
	0x08, // LinkageType
	0xF0, // HandOverType, OriginType
	0x56, 0x78, // InitialServiceId
};
char linkageDescriptor3[] =
{
	0x4A, // Tag
	0x0A, // Length
	0x12, 0x34, // TransportStreamId
	0x56, 0x78, // OriginalNetworkId
	0x9A, 0xBC, // ServiceId
	0x08, // LinkageType
	0x11, // HandOverType, OriginType
	0x12, 0x34, // NetworkId
};
char linkageDescriptor4[] =
{
	0x4A, // Tag
	0x08, // Length
	0x12, 0x34, // TransportStreamId
	0x56, 0x78, // OriginalNetworkId
	0x9A, 0xBC, // ServiceId
	0x08, // LinkageType
	0xF1, // HandOverType, OriginType
};

/* nvodReferenceDescriptor */
char nvodReferenceDescriptor1[] =
{
	0x4B, // Tag
	0x12, // Length
	0x12, 0x34, // TransportStreamId
	0x56, 0x78, // OriginalNetworkId
	0x9A, 0xBC, // ServiceId
	0xDE, 0xF0, // TransportStreamId
	0x12, 0x34, // OriginalNetworkId
	0x56, 0x78, // ServiceId
	0x9A, 0xBC, // TransportStreamId
	0xDE, 0xF0, // OriginalNetworkId
	0x12, 0x34, // ServiceId
};
char nvodReferenceDescriptor2[] =
{
    0x4B, // Tag
	0x00, // Length
};

/* timeShiftedServiceDescriptor = twoBytesDescriptor */
char *timeShiftedServiceDescriptor1 = twoBytesDescriptor;

/* shortEventDescriptor */
char shortEventDescriptor1[] =
{
	0x4D, // Tag
	0x1B, // Length
	0x65, 0x6E, 0x67, // Iso639LangageCode = eng
	0x0B, // EventNameLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // EventName = STRING_TEST
    0x0B, // TextLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // Text = STRING_TEST
};
char shortEventDescriptor2[] =
{
	0x4D, // Tag
	0x10, // Length
	0x65, 0x6E, 0x67, // Iso639LangageCode = eng
	0x00, // EventNameLength
	0x0B, // TextLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // EventName = STRING_TEST
};
char shortEventDescriptor3[] =
{
	0x4D, // Tag
	0x10, // Length
	0x65, 0x6E, 0x67, // Iso639LangageCode = eng
	0x0B, // EventNameLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // Text = STRING_TEST
	0x00, // TextLength
};
char shortEventDescriptor4[] =
{
	0x4D, // Tag
	0x05, // Length
	0x65, 0x6E, 0x67, // Iso639LangageCode = eng
	0x00, // EventNameLength
	0x00, // TextLength
};

/* extendedEventDescriptor */
char extendedEventDescriptor1[] =
{
	0x4E, // Tag
	0x59, // Length
	0x01, // DescriptorNumber, LastDescriptorNumber
	0x65, 0x6E, 0x67, // Iso639LangageCode = eng
	0x48, // LengthOfItems
	0x0B, // ItemDescriptionLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // ItemDescription = STRING_TEST
	0x0B, // ItemLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // Item = STRING_TEST
	0x0B, // ItemDescriptionLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // ItemDescription = STRING_TEST
	0x0B, // ItemLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // Item = STRING_TEST
	0x0B, // ItemDescriptionLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // ItemDescription = STRING_TEST
	0x0B, // ItemLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // Item = STRING_TEST
	0x0B, // TextLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // Text = STRING_TEST
};
char extendedEventDescriptor2[] =
{
	0x4E, // Tag
	0x4E, // Length
	0x01, // DescriptorNumber, LastDescriptorNumber
	0x65, 0x6E, 0x67, // Iso639LangageCode = eng
	0x48, // LengthOfItems
	0x0B, // ItemDescriptionLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // ItemDescription = STRING_TEST
	0x0B, // ItemLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // Item = STRING_TEST
	0x0B, // ItemDescriptionLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // ItemDescription = STRING_TEST
	0x0B, // ItemLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // Item = STRING_TEST
	0x0B, // ItemDescriptionLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // ItemDescription = STRING_TEST
	0x0B, // ItemLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // Item = STRING_TEST
    0x00, // TextLength
};
char extendedEventDescriptor3[] =
{
	0x4E, // Tag
	0x11, // Length
	0x01, // DescriptorNumber, LastDescriptorNumber
	0x65, 0x6E, 0x67, // Iso639LangageCode = eng
	0x00, // LengthOfItems
	0x0B, // TextLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // Text = STRING_TEST
};
char extendedEventDescriptor4[] =
{
	0x4E, // Tag
	0x06, // Length
	0x01, // DescriptorNumber, LastDescriptorNumber
	0x65, 0x6E, 0x67, // Iso639LangageCode = eng
	0x00, // LengthOfItems
	0x00, // TextLength
};

/* timeShiftedEventDescriptor */
char *timeShiftedEventDescriptor1 = fourBytesDescriptor;

/* componentDescriptor */
char componentDescriptor1[] =
{
	0x50, // Tag
	0x11, // Length
	0xF1, // StreamContent
	0x12, // ComponentType
	0x23, // ComponentTag
	0x65, 0x6E, 0x67, // Iso639LangageCode = eng
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // Text = STRING_TEST
};
char componentDescriptor2[] =
{
	0x50, // Tag
	0x06, // Length
	0xF1, // StreamContent
	0x12, // ComponentType
	0x23, // ComponentTag
	0x65, 0x6E, 0x67, // Iso639LangageCode = eng
};

/* mosaicDescriptor */
char mosaicDescriptor1[] = {
	0x51, // Tag
	0x2A, // Length
	0x12, // MosaicEntryPoint, NumberOfHorizontalElementaryCells, NumberOfVerticalElementaryCells

	0x34, 0x56, // LogicalCellId, LogicalCellPresentationInfo
	0x03, // ElementaryCellFieldLength
	0x78, // ElementaryCellId
	0x9A, // ElementaryCellId
	0xBC, // ElementaryCellId
	0x01, // CellLinkageInfo
	0xF0, 0x12, // BouquetId

	0x34, 0x56, // LogicalCellId, LogicalCellPresentationInfo
	0x00, // ElementaryCellFieldLength
	0x02, // CellLinkageInfo
	0x78, 0x9A, // OriginalNetworkId
	0xBC, 0xDE, // TransportStreamId
	0xF0, 0x12, // ServiceId

	0x34, 0x56, // LogicalCellId, LogicalCellPresentationInfo
	0x00, // ElementaryCellFieldLength
	0x03, // CellLinkageInfo
	0x78, 0x9A, // OriginalNetworkId
	0xBC, 0xDE, // TransportStreamId
	0xF0, 0x12, // ServiceId

	0x34, 0x56, // LogicalCellId, LogicalCellPresentationInfo
	0x00, // ElementaryCellFieldLength
	0x04, // CellLinkageInfo
	0x78, 0x9A, // OriginalNetworkId
	0xBC, 0xDE, // TransportStreamId
	0xF0, 0x12, // ServiceId
	0x34, 0x56, // EventId
};

/* streamIdentifierDescriptor */
char *streamIdentifierDescriptor1 = oneByteDescriptor;

/* caIdentifierDescriptor */
char caIdentifierDescriptor1[] =
{
	0x53, // Tag
	0x06, // Length
	0x12, 0x34, // CaIdentifier
	0x56, 0x78, // CaIdentifier
	0x9A, 0xBC, // CaIdentifier
};
char *caIdentifierDescriptor2 = nullDescriptor;

/* contentDescriptor */
char contentDescriptor1[] =
{
	0x54, // Tag
	0x06, // Length
	0x12, 0x34, // Nibble
	0x56, 0x78, // Nibble
	0x9A, 0xBC, // Nibble
};
char *contentDescriptor2 = nullDescriptor;

/* parentalRatingDescriptor */
char parentalRatingDescriptor1[] =
{
	0x55, // Tag
	0x0C, // Length
	0x65, 0x6E, 0x67, // CountryCode = eng
	0x12, // Rating
	0x66, 0x72, 0x61, // CountryCode = fra
	0x34, // Rating
	0x64, 0x65, 0x75, // CountryCode = deu
	0x56, // Rating
};
char *parentalRatingDescriptor2 = nullDescriptor;

/* teletextDescriptor */
char teletextDescriptor1[] =
{
	0x56, // Tag
	0x0F, // Length

	0x65, 0x6E, 0x67, // Iso639LangageCode = eng
	0x12, // TeletextType, TeletextMagazineNumber
	0x34, // TeletextMagazineNumber

	0x66, 0x72, 0x61, // Iso639LangageCode = fra
	0x56, // TeletextType, TeletextMagazineNumber
	0x78, // TeletextMagazineNumber

	0x64, 0x65, 0x75, // Iso639LangageCode = deu
	0x9A, // TeletextType, TeletextMagazineNumber
	0xBC, // TeletextMagazineNumber
};
char *teletextDescriptor2 = nullDescriptor;

/* telephoneDescriptor */
char telephoneDescriptor1[] =
{
	0x57, // Tag
	0x0F, // Length
	0x12, // ForeignAvailability, ConnectionType
	0x46, // CountryPrefixLength, InternationalAreaCodeLength, OperatorCodeLength
	0x16, // NationalAreaCode, CoreNumberLength
	0x33, 0x34, // CountryPrefixLength
	0x35, // InternationalAreaCode
	0x36, 0x37, // OperatorCode
	0x38, // NationalAreaCode
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, // CoreNumber
};

/* localTimeOffsetDescriptor */
char localTimeOffsetDescriptor1[] =
{
	0x58, // Tag
	0x27, // Length
	0x65, 0x6E, 0x67, // CountryCode = eng
	0x12, // CountryRegionId, LocalTimeOffsetPriority
	0x34, 0x56, // LocalTimeOffset
	0x78, 0x9A, 0x12, 0x34, 0x56, // TimeOfChange
	0x78, 0x90, // NextTimeOffset

	0x66, 0x72, 0x61, // CountryCode = fra
	0x12, // CountryRegionId, LocalTimeOffsetPriority
	0x34, 0x56, // LocalTimeOffset
	0x78, 0x9A, 0x12, 0x34, 0x56, // TimeOfChange
	0x78, 0x90, // NextTimeOffset

	0x64, 0x65, 0x75, // CountryCode = deu
	0x12, // CountryRegionId, LocalTimeOffsetPriority
	0x34, 0x56, // LocalTimeOffset
	0x78, 0x9A, 0x12, 0x34, 0x56, // TimeOfChange
	0x78, 0x90, // NextTimeOffset
};

/* subtitlingDescriptor */
char subtitlingDescriptor1[] =
{
	0x59, // Tag
	0x18, // Length
	0x65, 0x6E, 0x67, // Iso639LangageCode = eng
	0x12, // SubtitlingType
	0x34, 0x56, // CompositionPageId
	0x78, 0x9A, // AncillaryPageId

	0x66, 0x72, 0x61, // Iso639LangageCode = fra
	0xBC, // SubtitlingType
	0xDE, 0xF0, // CompositionPageId
	0x12, 0x34, // AncillaryPageId

	0x64, 0x65, 0x75, // Iso639LangageCode = deu
	0x56, // SubtitlingType
	0x78, 0x9A, // CompositionPageId
	0xBC, 0xDE, // AncillaryPageId
};

/* terrestrialDeliverySystemDescriptor */
char terrestrialDeliverySystemDescriptor1[] =
{
	0x5A, // Tag
	0x0B, // Length
	0x12, 0x34, 0x56, 0x78, // CentreFrequency
	0x9A, // Bandwidth
	0xBC, // Constellation, HierarchyInformation, CodeRateHPStream
	0xDE, // CodeRateLPStream, GuardInterval, TransmissionMode, OtherFrequencyFlag
	0xF0, 0x12, 0x34, 0x56, // Reserved
};

/* multilingualNetworkNameDescriptor */
char *multilingualNetworkNameDescriptor1 = multilingualStringDescriptor;
char *multilingualNetworkNameDescriptor2 = nullDescriptor;

/* multilingualBouquetNameDescriptor */
char *multilingualBouquetNameDescriptor1 = multilingualStringDescriptor;
char *multilingualBouquetNameDescriptor2 = nullDescriptor;

/* multilingualServiceNameDescriptor */
char multilingualServiceNameDescriptor1[] =
{
	0x5D, // Tag
	0x5D, // Length

	0x65, 0x6E, 0x67, // Iso639LangageCode = eng
	0x0B, // ServiceProvideNameLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // ServiceProvider = STRING_TEST
	0x0F, // ServiceNameLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, 0x5F, 0x45, 0x4E, 0x47, // ServiceName = STRING_TEST_ENG

	0x66, 0x72, 0x61, // Iso639LangageCode = fra
	0x0B, // ServiceProvideNameLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // ServiceProvider = STRING_TEST
	0x0F, // ServiceNameLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, 0x5F, 0x46, 0x52, 0x41, // ServiceName = STRING_TEST_FRA

	0x64, 0x65, 0x75, // Iso639LangageCode = deu
	0x0B, // ServiceProvideNameLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, // ServiceProvider = STRING_TEST
	0x0F, // ServiceNameLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, 0x5F, 0x44, 0x45, 0x55  // ServiceName = STRING_TEST_DEU
};
char *multilingualServiceNameDescriptor2 = nullDescriptor;

/* multilingualComponentDescriptor */
char multilingualComponentDescriptor1[] =
{
	0x5E, // Tag
	0x3A, // Length
	0x12, // ComponentTag
	0x65, 0x6E, 0x67, // Iso639LangageCode = eng
	0x0F, // StringLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, 0x5F, 0x45, 0x4E, 0x47, // String = STRING_TEST_ENG
	0x66, 0x72, 0x61, // Iso639LangageCode = fra
	0x0F, // StringLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, 0x5F, 0x46, 0x52, 0x41, // String = STRING_TEST_FRA
	0x64, 0x65, 0x75, // Iso639LangageCode = deu
	0x0F, // StringLength
	0x53, 0x54, 0x52, 0x49, 0x4E, 0x47, 0x5F, 0x54, 0x45, 0x53, 0x54, 0x5F, 0x44, 0x45, 0x55  // String = STRING_TEST_DEU
};
char multilingualComponentDescriptor2[] =
{
	0x5E, // Tag
	0x01, // Length
	0x12, // ComponentTag
};

/* privateDataSpecifierDescriptor */
char *privateDataSpecifierDescriptor1 = fourBytesDescriptor;

/* serviceMoveDescriptor */
char serviceMoveDescriptor1[] =
{
	0x60, // Tag
	0x06, // Length
	0x12, 0x34, // NewOriginalNetworkId
	0x56, 0x78, // NewTransportStreamId
	0x9A, 0xBC, // NewServiceId
};

/* shortSmoothingBufferDescriptor */
// TODO

/* frequencyListDescriptor */
char frequencyListDescriptor1[] =
{
	0x62, // Tag
	0x0D, // Length
	0x12, // CodingType
	0x34, 0x56, 0x78, 0x9A, // CentreFrequency
	0xBC, 0xDE, 0xF0, 0x12, // CentreFrequency
	0x34, 0x56, 0x78, 0x9A, // CentreFrequency
};
char frequencyListDescriptor2[] =
{
	0x62, // Tag
	0x01, // Length
	0x12, // CodingType
};

/* partialTransportStreamDescriptor */
// TODO

/* dataBroadcastDescriptor */
// TODO

/* dataBroadcastIdDescriptor */
// TODO

/* transportStreamDescriptor */
char *transportStreamDescriptor1 = stringDescriptor;
char *transportStreamDescriptor2 = nullDescriptor;

/* dsngDescriptor */
char *dsngDescriptor1 = stringDescriptor;
char *dsngDescriptor2 = nullDescriptor;

/* pdcDescriptor */
char *pdcDescriptor1 = threeBytesDescriptor;

/* ac3Descriptor */
// TODO

/* ancillaryDataDescriptor */
char *ancillaryDataDescriptor1 = oneByteDescriptor;

/* cellListDescriptor */
// TODO

/* cellFequencyLinkDescriptor */
// TODO

/* announcementSupportDescriptor */
// TODO

/* applicationSignalingDescriptor */
// TODO

/* adaptationFielDataDescriptor */
char *adaptationFielDataDescriptor = oneByteDescriptor;

/* serviceIdentifierDescriptor */
// TODO

/* serviceAvailabilityDescriptor */
// TODO

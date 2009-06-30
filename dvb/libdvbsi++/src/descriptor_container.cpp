/*
 * $Id: descriptor_container.cpp,v 1.16 2009/06/30 12:03:03 mws Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/aac_descriptor.h>
#include <dvbsi++/ac3_descriptor.h>
#include <dvbsi++/adaptation_field_data_descriptor.h>
#include <dvbsi++/ancillary_data_descriptor.h>
#include <dvbsi++/announcement_support_descriptor.h>
#include <dvbsi++/application_descriptor.h>
#include <dvbsi++/application_icons_descriptor.h>
#include <dvbsi++/application_name_descriptor.h>
#include <dvbsi++/application_signalling_descriptor.h>
#include <dvbsi++/application_storage_descriptor.h>
#include <dvbsi++/audio_stream_descriptor.h>
#include <dvbsi++/bouquet_name_descriptor.h>
#include <dvbsi++/ca_descriptor.h>
#include <dvbsi++/ca_identifier_descriptor.h>
#include <dvbsi++/cable_delivery_system_descriptor.h>
#include <dvbsi++/caching_priority_descriptor.h>
#include <dvbsi++/carousel_identifier_descriptor.h>
#include <dvbsi++/cell_frequency_link_descriptor.h>
#include <dvbsi++/cell_list_descriptor.h>
#include <dvbsi++/crc32_descriptor.h>
#include <dvbsi++/component_descriptor.h>
#include <dvbsi++/compressed_module_descriptor.h>
#include <dvbsi++/content_descriptor.h>
#include <dvbsi++/content_identifier_descriptor.h>
#include <dvbsi++/content_type_descriptor.h>
#include <dvbsi++/country_availability_descriptor.h>
#include <dvbsi++/cp_descriptor.h>
#include <dvbsi++/cp_identifier_descriptor.h>
#include <dvbsi++/cpcm_delivery_signalling_descriptor.h>
#include <dvbsi++/data_broadcast_descriptor.h>
#include <dvbsi++/data_broadcast_id_descriptor.h>
#include <dvbsi++/default_authority_descriptor.h>
#include <dvbsi++/delegated_application_descriptor.h>
#include <dvbsi++/descriptor_container.h>
#include <dvbsi++/descriptor_tag.h>
#include <dvbsi++/dii_location_descriptor.h>
#include <dvbsi++/dsng_descriptor.h>
#include <dvbsi++/dts_descriptor.h>
#include <dvbsi++/dvb_html_application_boundary_descriptor.h>
#include <dvbsi++/dvb_html_application_descriptor.h>
#include <dvbsi++/dvb_html_application_location_descriptor.h>
#include <dvbsi++/dvb_j_application_descriptor.h>
#include <dvbsi++/dvb_j_application_location_descriptor.h>
#include <dvbsi++/ecm_repetition_rate_descriptor.h>
#include <dvbsi++/enhanced_ac3_descriptor.h>
#include <dvbsi++/est_download_time_descriptor.h>
#include <dvbsi++/extended_event_descriptor.h>
#include <dvbsi++/extension_descriptor.h>
#include <dvbsi++/external_application_authorisation_descriptor.h>
#include <dvbsi++/frequency_list_descriptor.h>
#include <dvbsi++/fta_content_management_descriptor.h>
#include <dvbsi++/group_link_descriptor.h>
#include <dvbsi++/image_icon_descriptor.h>
#include <dvbsi++/info_descriptor.h>
#include <dvbsi++/ip_signaling_descriptor.h>
#include <dvbsi++/iso639_language_descriptor.h>
#include <dvbsi++/label_descriptor.h>
#include <dvbsi++/linkage_descriptor.h>
#include <dvbsi++/local_time_offset_descriptor.h>
#include <dvbsi++/location_descriptor.h>
#include <dvbsi++/module_link_descriptor.h>
#include <dvbsi++/mosaic_descriptor.h>
#include <dvbsi++/multilingual_bouquet_name_descriptor.h>
#include <dvbsi++/multilingual_component_descriptor.h>
#include <dvbsi++/multilingual_network_name_descriptor.h>
#include <dvbsi++/multilingual_service_name_descriptor.h>
#include <dvbsi++/name_descriptor.h>
#include <dvbsi++/network_name_descriptor.h>
#include <dvbsi++/nvod_reference_descriptor.h>
#include <dvbsi++/parental_rating_descriptor.h>
#include <dvbsi++/pdc_descriptor.h>
#include <dvbsi++/plugin_descriptor.h>
#include <dvbsi++/prefetch_descriptor.h>
#include <dvbsi++/private_data_specifier_descriptor.h>
#include <dvbsi++/registration_descriptor.h>
#include <dvbsi++/related_content_descriptor.h>
#include <dvbsi++/satellite_delivery_system_descriptor.h>
#include <dvbsi++/s2_satellite_delivery_system_descriptor.h>
#include <dvbsi++/scrambling_descriptor.h>
#include <dvbsi++/service_availability_descriptor.h>
#include <dvbsi++/service_descriptor.h>
#include <dvbsi++/service_identifier_descriptor.h>
#include <dvbsi++/service_list_descriptor.h>
#include <dvbsi++/service_move_descriptor.h>
#include <dvbsi++/short_event_descriptor.h>
#include <dvbsi++/short_smoothing_buffer_descriptor.h>
#include <dvbsi++/stream_identifier_descriptor.h>
#include <dvbsi++/stuffing_descriptor.h>
#include <dvbsi++/subtitling_descriptor.h>
#include <dvbsi++/target_background_grid_descriptor.h>
#include <dvbsi++/telephone_descriptor.h>
#include <dvbsi++/teletext_descriptor.h>
#include <dvbsi++/terrestrial_delivery_system_descriptor.h>
#include <dvbsi++/time_shifted_service_descriptor.h>
#include <dvbsi++/time_slice_fec_identifier_descriptor.h>
#include <dvbsi++/transport_protocol_descriptor.h>
#include <dvbsi++/transport_stream_descriptor.h>
#include <dvbsi++/tva_id_descriptor.h>
#include <dvbsi++/type_descriptor.h>
#include <dvbsi++/vbi_data_descriptor.h>
#include <dvbsi++/vbi_teletext_descriptor.h>
#include <dvbsi++/video_stream_descriptor.h>
#include <dvbsi++/video_window_descriptor.h>
#include <dvbsi++/xait_location_descriptor.h>

DescriptorContainer::~DescriptorContainer(void)
{
	for (DescriptorIterator i = descriptorList.begin(); i != descriptorList.end(); ++i)
		delete *i;
}

void DescriptorContainer::descriptor(const uint8_t * const buffer, const enum DescriptorScope scope, bool back)
{
	Descriptor *d;

	switch (scope) {
	case SCOPE_SI:
		d = descriptorSi(buffer, back);
		break;
	case SCOPE_CAROUSEL:
		d = descriptorCarousel(buffer, back);
		break;
	case SCOPE_MHP:
		d = descriptorMhp(buffer, back);
		break;
	default:
		/* ignore invalid scope */
		return;
	}

	if (!d->isValid())
		delete d;
	else if (back)
		descriptorList.push_back(d);
	else
		descriptorList.push_front(d);
}

Descriptor *DescriptorContainer::descriptorSi(const uint8_t * const buffer, bool back)
{
	switch (buffer[0]) {
	case VIDEO_STREAM_DESCRIPTOR:
		return new VideoStreamDescriptor(buffer);
	case AUDIO_STREAM_DESCRIPTOR:
		return new AudioStreamDescriptor(buffer);
	case REGISTRATION_DESCRIPTOR:
		return new RegistrationDescriptor(buffer);
#if 0
	case DATA_STREAM_ALIGNMENT_DESCRIPTOR:
		return new DataStreamAlignmentDescriptor(buffer);
#endif
	case TARGET_BACKGROUND_GRID_DESCRIPTOR:
		return new TargetBackgroundGridDescriptor(buffer);
	case VIDEO_WINDOW_DESCRIPTOR:
		return new VideoWindowDescriptor(buffer);
	case CA_DESCRIPTOR:
		return new CaDescriptor(buffer);
	case ISO_639_LANGUAGE_DESCRIPTOR:
		return new Iso639LanguageDescriptor(buffer);
#if 0
	case SYSTEM_CLOCK_DESCRIPTOR:
		return new SystemClockDescriptor(buffer);
	case MULTIPLEX_BUFFER_UTILIZATION_DESCRIPTOR:
		return new MultiplexBufferUtilizationDescriptor(buffer);
	case COPYRIGHT_DESCRIPTOR:
		return new CopyrightDescriptor(buffer);
	case MAXIMUM_BITRATE_DESCRIPTOR:
		return new MaximumBitrateDescriptor(buffer);
	case PRIVATE_DATA_INDICATOR_DESCRIPTOR:
		return new PrivateDataIndicatorDescriptor(buffer);
	case SMOOTHING_BUFFER_DESCRIPTOR:
		return new SmoothingBufferDescriptor(buffer);
	case STD_DESCRIPTOR:
		return new StdDescriptor(buffer);
	case IBP_DESCRIPTOR:
		return new IbpDescriptor(buffer);
#endif
	case CAROUSEL_IDENTIFIER_DESCRIPTOR:
		return new CarouselIdentifierDescriptor(buffer);
	case NETWORK_NAME_DESCRIPTOR:
		return new NetworkNameDescriptor(buffer);
	case SERVICE_LIST_DESCRIPTOR:
		return new ServiceListDescriptor(buffer);
	case STUFFING_DESCRIPTOR:
		return new StuffingDescriptor(buffer);
	case SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR:
		return new SatelliteDeliverySystemDescriptor(buffer);
	case CABLE_DELIVERY_SYSTEM_DESCRIPTOR:
		return new CableDeliverySystemDescriptor(buffer);
	case VBI_DATA_DESCRIPTOR:
		return new VbiDataDescriptor(buffer);
	case VBI_TELETEXT_DESCRIPTOR:
		return new VbiTeletextDescriptor(buffer);
	case BOUQUET_NAME_DESCRIPTOR:
		return new BouquetNameDescriptor(buffer);
	case SERVICE_DESCRIPTOR:
		return new ServiceDescriptor(buffer);
	case COUNTRY_AVAILABILITY_DESCRIPTOR:
		return new CountryAvailabilityDescriptor(buffer);
	case LINKAGE_DESCRIPTOR:
		return new LinkageDescriptor(buffer);
	case NVOD_REFERENCE_DESCRIPTOR:
		return new NvodReferenceDescriptor(buffer);
	case TIME_SHIFTED_SERVICE_DESCRIPTOR:
		return new TimeShiftedServiceDescriptor(buffer);
	case SHORT_EVENT_DESCRIPTOR:
		return new ShortEventDescriptor(buffer);
	case EXTENDED_EVENT_DESCRIPTOR:
		return new ExtendedEventDescriptor(buffer);
#if 0
	case TIME_SHIFTED_EVENT_DESCRIPTOR:
		return new TimeShiftedEventDescriptor(buffer);
#endif
	case COMPONENT_DESCRIPTOR:
		return new ComponentDescriptor(buffer);
	case MOSAIC_DESCRIPTOR:
		return new MosaicDescriptor(buffer);
	case STREAM_IDENTIFIER_DESCRIPTOR:
		return new StreamIdentifierDescriptor(buffer);
	case CA_IDENTIFIER_DESCRIPTOR:
		return new CaIdentifierDescriptor(buffer);
	case CONTENT_DESCRIPTOR:
		return new ContentDescriptor(buffer);
	case PARENTAL_RATING_DESCRIPTOR:
		return new ParentalRatingDescriptor(buffer);
	case TELETEXT_DESCRIPTOR:
		return new TeletextDescriptor(buffer);
	case TELEPHONE_DESCRIPTOR:
		return new TelephoneDescriptor(buffer);
	case LOCAL_TIME_OFFSET_DESCRIPTOR:
		return new LocalTimeOffsetDescriptor(buffer);
	case SUBTITLING_DESCRIPTOR:
		return new SubtitlingDescriptor(buffer);
	case TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR:
		return new TerrestrialDeliverySystemDescriptor(buffer);
	case MULTILINGUAL_NETWORK_NAME_DESCRIPTOR:
		return new MultilingualNetworkNameDescriptor(buffer);
	case MULTILINGUAL_BOUQUET_NAME_DESCRIPTOR:
		return new MultilingualBouquetNameDescriptor(buffer);
	case MULTILINGUAL_SERVICE_NAME_DESCRIPTOR:
		return new MultilingualServiceNameDescriptor(buffer);
	case MULTILINGUAL_COMPONENT_DESCRIPTOR:
		return new MultilingualComponentDescriptor(buffer);
	case PRIVATE_DATA_SPECIFIER_DESCRIPTOR:
		return new PrivateDataSpecifierDescriptor(buffer);
	case SERVICE_MOVE_DESCRIPTOR:
		return new ServiceMoveDescriptor(buffer);
	case SHORT_SMOOTHING_BUFFER_DESCRIPTOR:
		return new ShortSmoothingBufferDescriptor(buffer);
	case FREQUENCY_LIST_DESCRIPTOR:
		return new FrequencyListDescriptor(buffer);
#if 0
	case PARTIAL_TRANSPORT_STREAM_DESCRIPTOR:
		return new PartialTransportStreamDescriptor(buffer);
#endif
	case DATA_BROADCAST_DESCRIPTOR:
		return new DataBroadcastDescriptor(buffer);
	case SCRAMBLING_DESCRIPTOR:
		return new ScramblingDescriptor(buffer);
	case DATA_BROADCAST_ID_DESCRIPTOR:
		return new DataBroadcastIdDescriptor(buffer);
	case TRANSPORT_STREAM_DESCRIPTOR:
		return new TransportStreamDescriptor(buffer);
	case DSNG_DESCRIPTOR:
		return new DSNGDescriptor(buffer);
	case PDC_DESCRIPTOR:
		return new PdcDescriptor(buffer);
	case AC3_DESCRIPTOR:
		return new Ac3Descriptor(buffer);
	case ANCILLARY_DATA_DESCRIPTOR:
		return new AncillaryDataDescriptor(buffer);
	case CELL_LIST_DESCRIPTOR:
		return new CellListDescriptor(buffer);
	case CELL_FREQUENCY_LINK_DESCRIPTOR:
		return new CellFrequencyLinkDescriptor(buffer);
	case ANNOUNCEMENT_SUPPORT_DESCRIPTOR:
		return new AnnouncementSupportDescriptor(buffer);
	case APPLICATION_SIGNALLING_DESCRIPTOR:
		return new ApplicationSignallingDescriptor(buffer);
	case ADAPTATION_FIELD_DATA_DESCRIPTOR:
		return new AdaptationFieldDataDescriptor(buffer);
	case SERVICE_IDENTIFIER_DESCRIPTOR:
		return new ServiceIdentifierDescriptor(buffer);
	case SERVICE_AVAILABILITY_DESCRIPTOR:
		return new ServiceAvailabilityDescriptor(buffer);
	case DEFAULT_AUTHORITY_DESCRIPTOR:
		return new DefaultAuthorityDescriptor(buffer);
	case RELATED_CONTENT_DESCRIPTOR:
		return new RelatedContentDescriptor(buffer);
	case TVA_ID_DESCRIPTOR:
		return new TVAIdDescriptor(buffer);
	case CONTENT_IDENTIFIER_DESCRIPTOR:
		return new ContentIdentifierDescriptor(buffer);
	case TIME_SLICE_FEC_IDENTIFIER_DESCRIPTOR:
		return new TimeSliceFecIdentifierDescriptor(buffer);
	case ECM_REPETITION_RATE_DESCRIPTOR:
		return new ECMRepetitionRateDescriptor(buffer);
	case S2_SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR:
		return new S2SatelliteDeliverySystemDescriptor(buffer);
	case ENHANCED_AC3_DESCRIPTOR:
		return new EnhancedAC3Descriptor(buffer);
	case DTS_DESCRIPTOR:
		return new DTSDescriptor(buffer);
	case AAC_DESCRIPTOR:
		return new AACDescriptor(buffer);
	case XAIT_LOCATION_DESCRIPTOR:
		return new XaitLocationDescriptor(buffer);
	case FTA_CONTENT_MANAGEMENT_DESCRIPTOR:
		return new FtaContentManagementDescriptor(buffer);
	case EXTENSION_DESCRIPTOR:
		return descriptorSiExtended(buffer);
	default:
		return new Descriptor(buffer);
	}
}

Descriptor *DescriptorContainer::descriptorSiExtended(const uint8_t * const buffer, bool back)
{
	switch (buffer[2]) {
	case IMAGE_ICON_DESCRIPTOR:
		return new ImageIconDescriptor(buffer);
	case CPCM_DELIVERY_SIGNALLING_DESCRIPTOR:
		return new CpcmDeliverySignallingDescriptor(buffer);
	case CP_DESCRIPTOR:
		return new CpDescriptor(buffer);
	case CP_IDENTIFIER_DESCRIPTOR:
		return new CpIdentifierDescriptor(buffer);
	default:
		return new ExtensionDescriptor(buffer);
	}
}


Descriptor *DescriptorContainer::descriptorCarousel(const uint8_t * const buffer, bool back)
{
	switch (buffer[0]) {
	case TYPE_DESCRIPTOR:
		return new TypeDescriptor(buffer);

	case NAME_DESCRIPTOR:
		return new NameDescriptor(buffer);

	case INFO_DESCRIPTOR:
		return new InfoDescriptor(buffer);

	case MODULE_LINK_DESCRIPTOR:
		return new ModuleLinkDescriptor(buffer);

	case CRC32_DESCRIPTOR:
		return new Crc32Descriptor(buffer);

	case LOCATION_DESCRIPTOR:
		return new LocationDescriptor(buffer);

	case EST_DOWNLOAD_TIME_DESCRIPTOR:
		return new EstDownloadTimeDescriptor(buffer);

	case GROUP_LINK_DESCRIPTOR:
		return new GroupLinkDescriptor(buffer);

	case COMPRESSED_MODULE_DESCRIPTOR:
		return new CompressedModuleDescriptor(buffer);

	case LABEL_DESCRIPTOR:
		return new LabelDescriptor(buffer);

	case CACHING_PRIORITY_DESCRIPTOR:
		return new CachingPriorityDescriptor(buffer);

	case CONTENT_TYPE_DESCRIPTOR:
		return new ContentTypeDescriptor(buffer);

	case PRIVATE_DATA_SPECIFIER_DESCRIPTOR:
		return new PrivateDataSpecifierDescriptor(buffer);

	default:
		return new Descriptor(buffer);
	}
}

Descriptor *DescriptorContainer::descriptorMhp(const uint8_t * const buffer, bool back)
{
	switch (buffer[0]) {
	case APPLICATION_DESCRIPTOR:
		return new ApplicationDescriptor(buffer);

	case APPLICATION_NAME_DESCRIPTOR:
		return new ApplicationNameDescriptor(buffer);

	case TRANSPORT_PROTOCOL_DESCRIPTOR:
		return new TransportProtocolDescriptor(buffer);

	case DVB_J_APPLICATION_DESCRIPTOR:
		return new DvbJApplicationDescriptor(buffer);

	case DVB_J_APPLICATION_LOCATION_DESCRIPTOR:
		return new DvbJApplicationLocationDescriptor(buffer);

	case EXTERNAL_APPLICATION_AUTHORISATION_DESCRIPTOR:
		return new ExternalApplicationAuthorisationDescriptor(buffer);

	case DVB_HTML_APPLICATION_DESCRIPTOR:
		return new DvbHtmlApplicationDescriptor(buffer);

	case DVB_HTML_APPLICATION_LOCATION_DESCRIPTOR:
		return new DvbHtmlApplicationLocationDescriptor(buffer);

	case DVB_HTML_APPLICATION_BOUNDARY_DESCRIPTOR:
		return new DvbHtmlApplicationBoundaryDescriptor(buffer);

	case APPLICATION_ICONS_DESCRIPTOR:
		return new ApplicationIconsDescriptor(buffer);

	case PREFETCH_DESCRIPTOR:
		return new PrefetchDescriptor(buffer);

	case DII_LOCATION_DESCRIPTOR:
		return new DiiLocationDescriptor(buffer);

	case DELEGATED_APPLICATION_DESCRIPTOR:
		return new DelegatedApplicationDescriptor(buffer);

	case PLUGIN_DESCRIPTOR:
		return new PluginDescriptor(buffer);

	case APPLICATION_STORAGE_DESCRIPTOR:
		return new ApplicationStorageDescriptor(buffer);

	case IP_SIGNALING_DESCRIPTOR:
		return new IpSignalingDescriptor(buffer);

	case PRIVATE_DATA_SPECIFIER_DESCRIPTOR:
		return new PrivateDataSpecifierDescriptor(buffer);

	default:
		return new Descriptor(buffer);
	}
}

const DescriptorList *DescriptorContainer::getDescriptors(void) const
{
	return &descriptorList;
}

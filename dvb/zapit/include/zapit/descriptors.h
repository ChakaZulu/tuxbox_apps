/*
 * $Id: descriptors.h,v 1.9 2002/04/10 18:36:21 obi Exp $
 */

#ifndef __descriptors_h__
#define __descriptors_h__

#include <stdint.h>
#include <ost/frontend.h>

CodeRate getFEC(uint8_t FEC_inner);
Modulation getModulation (uint8_t modulation);

uint8_t ca_descriptor(uint8_t *buffer, uint16_t ca_system_id, uint16_t* ca_pid);
uint8_t stuffing_desc(uint8_t *buffer);
uint8_t linkage_desc(uint8_t *buffer);
uint8_t priv_data_desc(uint8_t *buffer);
uint8_t network_name_desc(uint8_t *buffer);
uint8_t service_list_desc(uint8_t *buffer);
uint8_t cable_deliv_system_desc(uint8_t *buffer, uint16_t transport_stream_id, uint16_t original_network_id);
uint8_t sat_deliv_system_desc(uint8_t *buffer, uint16_t transport_stream_id, uint16_t original_network_id, uint8_t DiSEqC);
uint8_t terr_deliv_system_desc(uint8_t *buffer);
uint8_t multilingual_network_name_desc(uint8_t *buffer);
uint8_t freq_list_desc(uint8_t *buffer);
uint8_t cell_list_desc(uint8_t *buffer);
uint8_t announcement_support_desc(uint8_t *buffer);
uint8_t service_name_desc(uint8_t *buffer, uint16_t service_id, uint16_t transport_stream_id, uint16_t onid, bool scan_mode);
uint8_t bouquet_name_desc(uint8_t *buffer);
uint8_t country_availability_desc(uint8_t *buffer);
uint8_t nvod_ref_desc(uint8_t *buffer, uint16_t transport_stream_id, bool scan_mode);
uint8_t time_shift_service_desc(uint8_t *buffer);
uint8_t mosaic_desc(uint8_t *buffer);
uint8_t ca_ident_desc(uint8_t *buffer);
uint8_t telephone_desc(uint8_t *buffer);
uint8_t multilingual_service_name_desc(uint8_t *buffer);
uint8_t data_broadcast_desc(uint8_t *buffer);
uint8_t network_name_desc(uint8_t *buffer);
uint8_t cell_freq_list_desc(uint8_t *buffer);

#endif /* __descriptors_h__ */

#ifndef SISERVICES_HPP
#define SISERVICES_HPP
//
// $Id: SIservices.hpp,v 1.2 2001/06/11 01:15:16 fnbrd Exp $
//
// classes SIservices and SIservices (dbox-II-project)
//
//    Homepage: http://dbox2.elxsi.de
//
//    Copyright (C) 2001 fnbrd (fnbrd@gmx.de)
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Log: SIservices.hpp,v $
// Revision 1.2  2001/06/11 01:15:16  fnbrd
// NVOD reference descriptors und Service-Typ
//
// Revision 1.1  2001/05/16 15:23:47  fnbrd
// Alles neu macht der Mai.
//
//

// forward references
class SIservice;
class SIevent;

struct sdt_service {
  unsigned short service_id : 16;
  // 2 Byte
  unsigned char reserved_future_use : 6;
  unsigned char EIT_schedule_flag : 1;
  unsigned char EIT_present_following_flag : 1;
  // 3 Bytes
  unsigned char running_status : 3;
  unsigned char free_CA_mode : 1;
  unsigned short descriptors_loop_length : 12;
} __attribute__ ((packed)) ; // 5 Bytes

class SInvodReference
{
  public:
    SInvodReference(unsigned short transport_stream_id, unsigned short original_network_id, unsigned short service_id) {
      transportStreamID=transport_stream_id;
      originalNetworkID=original_network_id;
      serviceID=service_id;
    }
    SInvodReference(const SInvodReference &ref) {
      transportStreamID=ref.transportStreamID;
      originalNetworkID=ref.originalNetworkID;
      serviceID=ref.serviceID;
    }
    bool operator < (const SInvodReference& ref) const {
      // nach Service-ID sortieren
      return serviceID < ref.serviceID;
    }
    void dump(void) const {
      printf("NVOD Ref. Service-ID: %hu\n", serviceID);
      printf("NVOD Ref. Transport-Stream-ID: %hu\n", transportStreamID);
      printf("NVOD Ref. Original-Network-ID: %hu\n", originalNetworkID);
    }
    unsigned short serviceID;
    unsigned short transportStreamID;
    unsigned short originalNetworkID;
};

// Fuer for_each
struct printSInvodReference : public unary_function<SInvodReference, void>
{
  void operator() (const SInvodReference &ref) { ref.dump();}
};

typedef set <SInvodReference, less<SInvodReference> > SInvodReferences;

class SIservice {
  public:
    SIservice(const struct sdt_service *s) {
      serviceID=s->service_id;
      serviceTyp=0;
      flags.EIT_schedule_flag=s->EIT_schedule_flag;
      flags.EIT_present_following_flag=s->EIT_present_following_flag;
      flags.running_status=s->running_status;
      flags.free_CA_mode=s->free_CA_mode;
    }
    // Um einen service zum Suchen zu erstellen
    SIservice(unsigned short sid) {
      serviceID=sid;
      serviceTyp=0;
      memset(&flags, 0, sizeof(flags));
    }
    // Std-Copy
    SIservice(const SIservice &s) {
      serviceID=s.serviceID;
      serviceTyp=s.serviceTyp;
      providerName=s.providerName;
      serviceName=s.serviceName;
      flags=s.flags;
      nvods=s.nvods;
    }
    unsigned short serviceID;
    unsigned char serviceTyp;
    SInvodReferences nvods;
    string serviceName; // Name aus dem Service-Descriptor
    string providerName; // Name aus dem Service-Descriptor
    int eitScheduleFlag(void) {return (int)flags.EIT_schedule_flag;}
    int eitPresentFollowingFlag(void) {return (int)flags.EIT_present_following_flag;}
    int runningStatus(void) {return (int)flags.running_status;}
    int freeCAmode(void) {return (int)flags.free_CA_mode;}
    // Der Operator zum sortieren
    bool operator < (const SIservice& s) const {
      // nach Service-ID sortieren
      return serviceID < s.serviceID;
    }
    void dump(void) const {
      printf("Service-ID: %hu\n", serviceID);
      printf("Service-Typ: %hhu\n", serviceTyp);
      if(providerName.length())
        printf("Provider-Name: %s\n", providerName.c_str());
      if(serviceName.length())
        printf("Service-Name: %s\n", serviceName.c_str());
      for_each(nvods.begin(), nvods.end(), printSInvodReference());
      printf("\n");
    }
  protected:
    struct {
      unsigned char EIT_schedule_flag : 1;
      unsigned char EIT_present_following_flag : 1;
      unsigned char running_status : 3;
      unsigned char free_CA_mode : 1;
    } flags;
};

// Fuer for_each
struct printSIservice : public unary_function<SIservice, void>
{
  void operator() (const SIservice &s) { s.dump();}
};

typedef set <SIservice, less<SIservice> > SIservices;

#endif // SISERVICES_HPP
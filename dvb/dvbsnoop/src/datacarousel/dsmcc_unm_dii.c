/*
$Id: dsmcc_unm_dii.c,v 1.2 2004/02/15 20:46:09 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)

 -- dsmcc Download Info Indication
 -- to be called from U-N-Message



$Log: dsmcc_unm_dii.c,v $
Revision 1.2  2004/02/15 20:46:09  rasc
DSM-CC  data/object carousell continued   (DSI, DII, DDB, DCancel)

Revision 1.1  2004/02/15 18:58:27  rasc
DSM-CC  data/object carousell continued   (DSI, DII, DDB, DCancel)





*/




#include "dvbsnoop.h"
#include "dsmcc_unm_dii.h"
#include "dsmcc_misc.h"
#include "misc/output.h"
#include "misc/hexprint.h"





/*
 * ISO/IEC 13818-6
 * dsmcc  Download Info Indication
 */


int dsmcc_DownloadInfoIndication (int v, u_char *b, u_int len)
{
   int   	len_org = len;
   int		n_modules;
   int 		len2;
   int		x;



	// already read  dsmcc_MessageHeader (v, b, len, &dmh);

	outBit_Sx_NL (v,"downloadId: ",		b   ,  0, 32);
	outBit_Sx_NL (v,"blockSize: ",		b+ 4,  0, 16);
	outBit_Sx_NL (v,"windowSize: ",		b+ 6,  0,  8);
	outBit_Sx_NL (v,"ackPeriod: ",		b+ 7,  0,  8);
	outBit_Sx_NL (v,"tCDownloadWindow: ",	b+ 8,  0, 32);
	outBit_Sx_NL (v,"tCDownloadScenario: ",	b+12,  0, 32);
	b += 16;
	len -= 16;

	x = dsmcc_CompatibilityDescriptor (b);
	b += x;
	len -= x;

	n_modules = outBit_Sx_NL (v,"numberOfModules: ",	b,  0, 16);
	b += 2;
	len -= 2;


	while (n_modules > 0) {
		out_NL (v);
		out_nl (v, "Module:");
		indent (+1);

		outBit_Sx_NL (v,"moduleId: ",		b,  0, 16);
		outBit_Sx_NL (v,"moduleSize: "	,	b, 16, 32);
		outBit_Sx_NL (v,"moduleVersion: ",	b, 48,  8);
		len2 = outBit_Sx_NL (v,"moduleInfoLength: ",	b, 56,  8);
		b += 8;
		len -= 8;

		print_databytes (v, "moduleInfoBytes: ", b, len2);
		b += len2;
		len -= len2;

		indent (-1);
		n_modules--;
	}
	out_NL (v);


	len2 = outBit_Sx_NL (v,"privateDataLength: ",	b,  0, 16);
	print_databytes (v, "privat Data: ", b+2, len2);   // $$$ TODO ???
	// b += 2 + len2;
	// len -= 2 + len2;


	return len_org;
}








// EN 301 192
// 8.1.3 DownloadInfoIndication message
//
// The DownloadInfoIndication message contains the description of the
// modules within a group as well as some general parameters of the
// data carousel (such as downloadId and blockSize). Each module is
// described by a number of attributes. The attributes moduleId,
// moduleSize, and moduleVersion are defined as fields in the
// DownloadInfoIndication message by DSM-CC (see ISO/IEC 13818-6 [5]).
// Other module attributes shall be carried as descriptors as defined
// below. The moduleId range of 0xFFF0-0xFFFF is reserved for DAVIC
// compliant applications.  The semantics of the DownloadInfoIndication
// message for DVB data carousels are as follows:
//
// compatibilityDescriptor(): this structure shall only contain the
// compatibilityDescriptorLength field of the compatibilityDescriptor()
// as defined in DSM-CC (see ISO/IEC 13818-6 [5]). It shall be set to
// the value of 0x0000.
//
// moduleInfoLength: this field defines the length in bytes of the
// moduleInfo field for the described module.
//
// moduleInfoByte: these fields shall convey a list of descriptors
// which each define one or more attributes of the described module,
// except when the moduleId is within the range of 0xFFF0-0xFFFF. In
// this case, the moduleInfoByte structure contains the ModuleInfo
// structure as defined by DAVIC with the privateDataByte field of that
// structure as a loop of descriptors.
//
// privateDataLength: this field defines the length in bytes of the
// privateDataByte field.
//
// privateDataByte: these fields are user defined.





// $$$ TODO


// TS 102 812 --  B.2.2.2 DownloadInfoIndication
// The DownloadInfoIndication is a message that describes a set of modules and gives the necessary parameters to locate the module and retrieve it.

// B.2.2.4 ModuleInfo
// The moduleInfo structure is placed in the moduleInfo 
//
// BIOP::ModuleInfo::Taps
// The .rst tap shall have the "use"value 0x0017 (BIOP_OBJECT_USE).The
// id and selector .elds are not used and the MHP terminal may ignore them.
// The MHP terminal may ignore possible other taps in the list.
// DVB
// BIOP::ModuleInfo::
// UserInfo
// The userInfo .eld contains a loop of descriptors.These are speci .ed in the
// DVB Data Broadcasting standard and/or this speci .cation.The MHP
// terminal shall support the compressed_module_descriptor (tag 0x09)used
// to signal that the module is transmitted in compressed form.The userInfo
// .eld may also contain a caching_priority_descriptor and one or more label_
// descriptors.  // DVB /This // spec.


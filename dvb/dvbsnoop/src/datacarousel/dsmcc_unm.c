/*
$Id: dsmcc_unm.c,v 1.1 2004/02/15 01:02:10 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)

 -- dsmcc User Network  Message
 -- ISO/IEC 13818-6:2000   7. User-Network Download



$Log: dsmcc_unm.c,v $
Revision 1.1  2004/02/15 01:02:10  rasc
DSM-CC  DDB (DownloadDataBlock Message)
DSM-CC  U-N-Message  started
Carousel Descriptors completed





*/




#include "dvbsnoop.h"
#include "dsmcc_unm.h"
#include "dsmcc_misc.h"
#include "misc/output.h"
#include "misc/hexprint.h"






/*
 * ISO/IEC 13818-6
 * user Network Message
 */


int dsmcc_UserNetworkMessage (int v, u_char *b, int len)
{
   int   x;
   int   msg_len, msgId, dsmcctype;
   int   len_org = len;



	x = dsmcc_MessageHeader (v, b, len, &msg_len, &dsmcctype, &msgId);
	b += x;
	len -= x;

	switch (msgId) {
		case DownloadInfoIndication:
 // $$$ TODO
			print_databytes (v, "$$$1 Data bytes: ", b, len);
			break;

		case DownloadCancel:
 // $$$ TODO
			print_databytes (v, "$$$2 Data bytes: ", b, len);
			break;

		case DownloadServerInitiate:
 // $$$ TODO
			print_databytes (v, "$$$3 Data bytes: ", b, len);
			break;

		default:
			out_nl (v,"unsupported messageId (please report!)");
			print_databytes (v, "Data bytes: ", b, len);
			break;
	}


	return len_org;
}


/* $$$ TODO


DownloadInfoResponse(), DownloadInfoIndication() {
dsmccMessageHeader()
downloadId 4
blockSize 2
windowSize 1
ackPeriod 1
tCDownloadWindow 4
tCDownloadScenario 4
compatibilityDescriptor()
numberOfModules 2
for(i=0;i< numberOfModules;i++) {
moduleId 2
moduleSize 4
moduleVersion 1
moduleInfoLength 1
for(i=0;i< moduleInfoLength;i++) {
moduleInfoByte
}
1
}
privateDataLength 2
for(i=0;i< privateDataLength;i++) {
privateDataByte
}
1
}




Table 6-1 Data structure of DownloadInfoIndication message
Syntax Bits Mnemonic
DownloadInfoIndication() {
dsmccMessageHeader()
downloaded 32 uimsbf
blockSize 16 uimsbf
windowSize 8 uimsbf
ackPeriod 8 uimsbf
tCDownloadWindow 32 uimsbf
tCDownloadScenario 32 uimsbf
compatibilityDescriptor()
numberOfModules 16 uimsbf
for(i=0;i< numberOfModules;i++) {
moduleId 16 uimsbf
moduleSize 32 uimsbf
moduleVersion 8 uimsbf
moduleInfoLength 8 uimsbf
for(i=0;i< moduleInfoLength;i++) {
moduleInfoByte 8 uimsbf
privateDataLength 16 uimsbf
for(i=0;i< privateDataLength;i++) {
privateDataByte 8 uimsbf
Semantics of DII fields:


*/


/* $$$ TODO


For the two-layer carousel:
- For DownloadServerInitiate messages the 2 least significant bytes of the transactionId shall be in the
range 0x0000-0x0001.
- For DownloadInfoIndication messages the 2 least significant bytes of the transactionId shall be in the
range 0x0002-0xFFFF.
For the one-layer carousel:
- For DownloadInfoIndication messages the 2 least significant bytes of the transactionId shall be in the
range 0x0000-0x0001.









8.1.2 DownloadServerInitiate message

The DownloadServerInitiate message is used to build a SuperGroup. The semantics for DVB data carousels are as
follows:
serverId: this field shall be set to 20 bytes with the value of 0xFF.
compatibilityDescriptor(): this structure shall only contain the compatibilityDescriptorLength field of the
compatibilityDescriptor() as defined in DSM-CC (see ISO/IEC 13818-6 [5]). It shall be set to the value of 0x0000. The
privateDataByte fields shall contain the GroupInfoIndication structure as defined in table 37.
privateDataLength: this field defines the length in bytes of the following GroupInfoIndication structure.
privateDataByte: these fields shall convey the GroupInfoIndication structure as defined in table 37.











Semantics of the GroupInfoIndication structure:
numberOfGroups: this is a 16-bit field that indicates the number of groups described in the loop following this field.
groupId: this is a 32-bit field which shall be equal to transactionId of the DownloadInfoIndication message that
describes the group.
groupSize: this is a 32-bit field that shall indicate the cumulative size in bytes of all the modules in the group.
groupCompatibility: the GroupCompatibility structure is equal to the CompatibilityDescriptor structure of
DSM-CC (see ISO/IEC 13818-6 [5]).
groupInfoLength: this is a 16-bit field indicating the length in bytes of the descriptor loop to follow.
groupInfoByte: these fields shall convey a list of descriptors which each define one or more attributes. The descriptors
included in the loop shall describe the characteristics of the group.
privateDataLength: this field defines the length in bytes of the following privateDataByte fields.
privateDataByte: these fields are user defined.
8.1.3 DownloadInfoIndication message
The DownloadInfoIndication message contains the description of the modules within a group as well as some general
parameters of the data carousel (such as downloadId and blockSize). Each module is described by a number of
attributes. The attributes moduleId, moduleSize, and moduleVersion are defined as fields in the
DownloadInfoIndication message by DSM-CC (see ISO/IEC 13818-6 [5]). Other module attributes shall be carried as
descriptors as defined below. The moduleId range of 0xFFF0-0xFFFF is reserved for DAVIC compliant applications.
The semantics of the DownloadInfoIndication message for DVB data carousels are as follows:
compatibilityDescriptor(): this structure shall only contain the compatibilityDescriptorLength field of the
compatibilityDescriptor() as defined in DSM-CC (see ISO/IEC 13818-6 [5]). It shall be set to the value of 0x0000.
moduleInfoLength: this field defines the length in bytes of the moduleInfo field for the described module.
moduleInfoByte: these fields shall convey a list of descriptors which each define one or more attributes of the
described module, except when the moduleId is within the range of 0xFFF0-0xFFFF. In this case, the moduleInfoByte
structure contains the ModuleInfo structure as defined by DAVIC with the privateDataByte field of that structure as a
loop of descriptors.
privateDataLength: this field defines the length in bytes of the privateDataByte field.
privateDataByte: these fields are user defined.






8.1.5 DownloadCancel
The DownloadCancel message may be used to indicate to the receivers that the data carousel has aborted the periodic
transmission of the modules. DownloadCancel messages may be sent at either the group or the super group level. They
are conveyed in the payload of MPEG-2 Transport Stream packets as specified in the DSM-CC specification
(see ISO/IEC 13818-6 [5]).
privateDataLength: this field defines the length in bytes of the privateDataByte fields.
privateDataByte: these fields are user defined.










*/




/*


7.3.6 DownloadServerInitiate
The DownloadServerInitiate message shall be sent from the Download Server to the Client. In the flow-controlled
download scenario, it is a request to the Client to initiate a download by sending the DownloadInfoRequest message. In
the non-flow-controlled download scenario and the data carousel scenario, the DownloadServerInitiate message may be
used to inform the Client about the connection on which the DownloadInfoIndication messages are located.
Table 7-12 DownloadServerInitiate message
Syntax Num. of Bytes
DownloadServerInitiate() {
dsmccMessageHeader()
serverId 20
compatibilityDescriptor()
privateDataLength 2
for(i=0;i<privateDataLength;i++) {
privateDataByte
}
1
}
The serverId is the globally unique OSI NSAP address of the Download Server to which the Client sends a
DownloadInfoRequest, if appropriate. Note the OSI NSAP format enables the use of many different types of lower level
network addresses. Therefore, this field is used for the same purpose even when the Download protocol is used outside
the context of a User-Network session.
The compatibilityDescriptor as defined in clause 6. The Download Server may use this structure to indicate for which
Clients the message is appropriate. This field is used by the Client to determine whether subsequent actions are
appropriate. This structure may also be used to inform Clients that they should listen for the DownloadInfoIndication
message or send a DownloadInfoRequest.
The privateDataLength field defines the length in bytes of the following privateDataByte fields.
The data in the privateDataByte field is carried from the Download Server to the Client transparently. For example,
this message could provide implementation-specific information on why the Download Server wishes the Client to
initiate a download. Alternatively, this field may contain information about where the associated
DownloadInfoIndication messages are located.



*/







typedef struct {
     unsigned char Packet;
     unsigned char Status;
     unsigned char SPktBuf;
     unsigned char Stream;
     unsigned StreamPacket;
} PacketHeaderType;
         

// MTU betraegt standartmaessig 1500 und kann mit "ifconfig" nicht erhoeht 
// werden. Abzueglich 8 Byte UDP-Header und 24/20 Byte IP-Header => 1468
// Im Experiment wurde 1472 festgestellt.
#define DATA_PER_PACKET 1472
#define NET_DATA_PER_PACKET (DATA_PER_PACKET-sizeof(PacketHeaderType))


#define MAX_PID_NUM 9   /* 1 Video + 8 Audio */
#define MAX_SPKT_BUF_NUM 36
#define SPKT_BUF_PACKET_NUM 256  
#define SPKT_BUF_SIZE (SPKT_BUF_PACKET_NUM * DATA_PER_PACKET)


#define AUDIO_BUF_PACKET_NUM 20
#define AV_BUF_FACTOR 7
#define DMX_BUF_FACTOR 5

#define STRING_SIZE 1000


/*
   Copyright (c) 2003 Harald Maiss
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 

*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sched.h>
#include <pthread.h>
#include <errno.h>

#include <linux/dvb/dmx.h>
#include <zapit/client/zapitclient.h>

#include "udpstreampes.h"

#define WATCHDOG_TRESHOLD 3

typedef struct {
   unsigned char *Buf[2];
   unsigned ReadBuf;
   unsigned ReadPkt;
   unsigned WriteBuf;
   unsigned WritePos;
   unsigned BufPacketNum;
   int Stopped;
   int fd;
   struct dmx_pes_filter_params Filter;
   void *Ptr;
   pthread_t Thread;
   pid_t ProcessID;
} StreamType;

StreamType Stream[MAX_PID_NUM];
unsigned StreamNum;
int StreamStop;   
pid_t mainProcessID;

struct {
   int Socket;
   unsigned Packet;
   unsigned Watchdog;
   pthread_t Thread;
   struct sockaddr_in Addr;
   socklen_t AddrLen;    
   unsigned Port;
   pid_t ProcessID;
} Send;

struct {
  unsigned char *Buf[MAX_SPKT_BUF_NUM];
  unsigned char ReSend[MAX_SPKT_BUF_NUM][SPKT_BUF_PACKET_NUM];
  unsigned char ReSendStatus[MAX_SPKT_BUF_NUM];
  int WritePkt;
  int WriteBuf;
  int ReadPkt;
  int ReadBuf;
  int BufSize;
} SPkt;

int NextSPktBuf[MAX_SPKT_BUF_NUM];

void * UdpSender( void * Ptr );

void RestartUdpSender()
{            
   pthread_attr_t ThreadAttr;
   struct sched_param SchedParam;

   if ( Send.Socket != -1 ) {
      if( -1 == close( Send.Socket ) ) {
         perror("ERROR: RestartUdpSender() - close");
         fflush(stderr);
      }
   }
   Send.Socket = socket(AF_INET, SOCK_DGRAM, 0);
   if (Send.Socket == -1) {
      perror("ERROR: RestartUdpSender() - socket");
      fprintf(stderr, "EXIT\n");
      fflush(stderr);
      exit(-1);
   }
   if( -1 == connect( Send.Socket, (struct sockaddr*)&(Send.Addr), 
                                                    Send.AddrLen ) ) {
      perror("ERROR: RestartUdpSender() - connect");
      fprintf(stderr, "EXIT\n");
      fflush(stderr);
      exit(-1);
   }   
   
   SchedParam.sched_priority = 10;
   pthread_attr_init(&ThreadAttr);
   pthread_attr_setschedpolicy(&ThreadAttr, SCHED_FIFO );
   pthread_attr_setschedparam(&ThreadAttr, &SchedParam);

   // bei Polling fuehrt die hohe Proritaet zu DmxBufferOverflows
   if ( -1 == pthread_create(&(Send.Thread), 0, // &ThreadAttr,
                                                  UdpSender, 0 ) ) {
      perror("ERROR: RestartUdpSender() - pthread_create");
      fprintf(stderr, "EXIT\n");
      fflush(stderr);
      exit(-1);
   } 
}

void * UdpSender( void * Ptr )
{
   int i;
   unsigned u;
   unsigned char *ReadPtr;

   (void)Ptr;

 //  fd_set wfds;
 //  struct timespec ts;
 //  FD_ZERO(&wfds);
 //  FD_SET(Send.Socket, &wfds);
   
 //  ts.tv_sec = 0;
 //  ts.tv_nsec = 2400000;   // 2 * (1500 * 8 / 10 MBit/s) * 1E9   

   Send.ProcessID = getpid();
   printf("INFO: UdpSender() - PID%i R%i W%i\n", 
              Send.ProcessID, SPkt.ReadBuf, SPkt.WriteBuf);  
   fflush(stdout);
   
   while(true) {
      while (SPkt.ReadBuf != SPkt.WriteBuf ) {
         ReadPtr = SPkt.Buf[SPkt.ReadBuf] + Send.Packet * DATA_PER_PACKET;
         for (i=Send.Packet; i<SPKT_BUF_PACKET_NUM; i++) {
            Send.Watchdog = 0;  
            Send.Packet = i;
            // MSG_DONTWAIT ist notwendig, weil bei blocking I/O send()
            // frueher oder spaeter haengen bleibt -> Watchdog 
            if ( -1 == write (Send.Socket, ReadPtr, 
                            DATA_PER_PACKET ) ) {
//            while ( -1 == send (Send.Socket, ReadPtr, 
//                            DATA_PER_PACKET, MSG_DONTWAIT ) ) {
//               if( errno != EAGAIN ) {
   //               pselect(1, NULL, &wfds, NULL, &ts, NULL);
   //            } else {
                  Send.Watchdog = WATCHDOG_TRESHOLD;
                  perror("ERROR: UdpSender() - send");
                  fflush(stderr);
                  pthread_exit(0);
 //              } 
            }
            if ( ((PacketHeaderType*)ReadPtr)->Status == 2 ) {
               for ( u=0; u< StreamNum; u++ ) free( SPkt.Buf[u] );
               close( Send.Socket );
               StreamStop = 1;
               pthread_exit(0);
            }
            ReadPtr += DATA_PER_PACKET;
         }
         Send.Packet = 0;
         SPkt.ReadBuf = NextSPktBuf[SPkt.ReadBuf];
      }
      Send.Watchdog = 0;  
      usleep(30000);  // 10% * 256*1500*8/10Mbit/s   
   }
}

void * DmxReader( void * Ptr )
{
   unsigned BufLen, BufSize;
   int  RetVal;
   StreamType *Stream;        
   Stream = (StreamType*)Ptr;
   BufSize = (Stream->BufPacketNum) * NET_DATA_PER_PACKET;


   Stream->fd = open("/dev/dvb/adapter0/demux0", O_RDWR);
   if (-1 == Stream->fd) {
      perror("ERROR: main() - dmx open");
      fprintf(stderr, "EXIT\n");
      fflush(stderr);
      exit(-1);
   }

   if ( -1 == ioctl(Stream->fd, DMX_SET_BUFFER_SIZE, // 1024*1024) ) { 
       Stream->BufPacketNum * NET_DATA_PER_PACKET * DMX_BUF_FACTOR) ) { 
      perror("ERROR: main() - dmx set buffer ioctl");
      fprintf(stderr, "EXIT\n");
      fflush(stderr);
      exit(-1);
   }

   Stream->Filter.input=DMX_IN_FRONTEND;
   Stream->Filter.output=DMX_OUT_TAP;
   Stream->Filter.pes_type=DMX_PES_OTHER;
   Stream->Filter.flags=DMX_IMMEDIATE_START;

   if (-1==ioctl(Stream->fd, DMX_SET_PES_FILTER, &(Stream->Filter)) ) {
      perror("ERROR: main() - dmx set filter ioctl");
      fprintf(stderr, "EXIT\n");
      fflush(stderr);
      exit(-1);
   }


    
   if ( -1==ioctl(Stream->fd, DMX_START, 0) ) {
      perror("ERROR: DmxReader() - dmx start ioctl");
      fprintf(stderr, "EXIT\n");
      fflush(stderr);
      exit(-1);
   } 

   // DMX-Buffer von alten Daten leeren
   // Diese Daten den "avia_gt_dmx: queue 3 overflow (count: 1)" am Start,
   // wenn man mehrere Male hintereinander streamt. Kritisch ist vor allem
   // MP2-Audio. Die Altdaten wuerden zu einem Wingrab-Abbruch fuehren.
   //for (i=0; i<DMX_BUF_FACTOR+1; i++) 
    // read( Stream->fd, Stream->Buf[0], BufSize);


   printf("INFO: DmxReader() - Pid %x %u %i %i\n", Stream->Filter.pid,
         BufSize, Stream->WriteBuf, Stream->ReadBuf);
   fflush(stdout); 

   BufLen = 0;
   while( !StreamStop ) {
      RetVal = read( Stream->fd, 
	             Stream->Buf[Stream->WriteBuf]+BufLen,
	             BufSize - BufLen );
	  
      if (RetVal == -1) {
         perror("ERROR: DmxReader() - read");
         fprintf(stderr, "EXIT (pid %x)\n", Stream->Filter.pid);
         fflush(stderr);
         exit(-1);
      }
      BufLen += RetVal;
      if ( BufLen < BufSize ) continue; 
	    	    
      if ( Stream->WriteBuf != Stream->ReadBuf ) {
         usleep( 300000 );
         if ( Stream->WriteBuf != Stream->ReadBuf ) {
            fprintf(stderr, "ERROR: DmxReader() - buffer overflow Pid %x %i %i\n",
                 Stream->Filter.pid, Stream->WriteBuf, Stream->ReadBuf);
            fflush(stderr);
            continue;
            //exit(-1);
         }
      }
      if (Stream->WriteBuf == 0) Stream->WriteBuf = 1;
                            else Stream->WriteBuf = 0;  

      BufLen = 0;
   }	 
   if ( -1 == ioctl(Stream->fd, DMX_STOP, 0) ) {
      perror("ERROR: DmxReader() - dmx stop ioctl");
      fprintf(stderr, "Pid %x\n", Stream->Filter.pid);
      fflush(stderr);
   } 	 
   close( Stream->fd );
   free( Stream->Buf[0] );
   free( Stream->Buf[1] );
   Stream->Stopped = 1;
   pthread_exit(0);
}

void ReadLine( char String[] )
{
   char c, *StrPtr;

   StrPtr = String;
   while (StrPtr-String < STRING_SIZE-1 ) {
      if ( 1 == read(STDIN_FILENO, &c, 1) ) {
         if ((*StrPtr++=c)=='\n') break;
      }   
   }
   *StrPtr++ = 0;
}


void * TcpReceiver( void * Ptr )
{
   char TcpString[STRING_SIZE], PacketString[STRING_SIZE];
   unsigned SPktBuf, u;

   (void)Ptr;

   while(true) {
      ReadLine( TcpString );
      if( !strncmp(TcpString, "RESEND", 6)) {
         if ( 2 != sscanf(TcpString, "RESEND %u %s", &SPktBuf, PacketString) ) {
            fprintf(stderr, "ERROR: TcpReceiver - sscanf RESEND\n");
            continue;
         } 
         for( u=0; u<SPKT_BUF_PACKET_NUM; u++) {
            if (PacketString[u] == 'y') {
               SPkt.ReSend[SPktBuf][u] = 1;
            } else {
               SPkt.ReSend[SPktBuf][u] = 0;
            }
         }
         SPkt.ReSendStatus[SPktBuf] = 1;
      } else if ( !strncmp(TcpString, "STOP", 4 )) {
         StreamStop=1;    
         break;
      } else {
         fprintf(stderr, "ERROR: TcpReader - illegal command\n");
         fflush(stderr);
      }
   }
   pthread_exit(0);
}

unsigned char *WritePtr, *ReadPtr;
PacketHeaderType *PacketHeader;

void CheckNextSPktWriteBuf()
{

                  if (SPkt.WritePkt == SPKT_BUF_PACKET_NUM ) {
                     PacketHeader->Status = 1; 
                     // Trigger fuer TCP-Kommunikation - SPkt-Ende 
                                            
                     if (NextSPktBuf[SPkt.WriteBuf] == SPkt.ReadBuf) {
                        fprintf(stderr, "ERROR: main() - SPkt buffer overflow\n");
                        fflush(stderr);
                        // exit(-1);
                     } else {
                        SPkt.WriteBuf = NextSPktBuf[SPkt.WriteBuf];
                     }
                     SPkt.WritePkt = 0;
                     WritePtr = SPkt.Buf[SPkt.WriteBuf];
              
                     if (Send.Watchdog >= WATCHDOG_TRESHOLD ) {
                        fprintf(stderr, "ERROR: main() - Send.Watchdog kill %i\n",
                                                         Send.ProcessID);
                        fflush(stderr);
                        pthread_cancel( Send.Thread );
                        Send.Watchdog = 0;
                        RestartUdpSender();
                     } else {
                        Send.Watchdog++;
                     }
                  }
}
   
int main ()
{
   unsigned Bouquet;
   unsigned Channel;
   unsigned BufNum;
   int ExtraPidNum;
   unsigned ExtraPid[MAX_PID_NUM];
   char ExtraAVString[MAX_PID_NUM+1], AVString[MAX_PID_NUM+1];
   unsigned StoppedDmxReaders, EmptyStreamBuffers;
   char CmdString[100];
   char TcpString[STRING_SIZE];

   pthread_t TcpReceiverThread;
   pthread_attr_t ThreadAttr;
   struct sched_param SchedParam;
 
   int RetVal, i;
   unsigned u, v;

   mainProcessID = getpid();

   // ****************************************************
   // * INIT
   // ****************************************************
   // * - Programm umschalten
   // * - PID's ermitteln
   // * - IP-Adresse, Port, MTU ermitteln
   // ****************************************************

   // Eingabe parsen

   ReadLine( TcpString );
   RetVal = sscanf( TcpString, "%s %u %u %u %u %s %x %x %x %x %x %x %x %x %x",
                  CmdString, &(Send.Port), &BufNum, 
                  &Bouquet, &Channel, ExtraAVString,
                  &(ExtraPid[0]), &(ExtraPid[1]), &(ExtraPid[2]),
                  &(ExtraPid[3]), &(ExtraPid[4]), &(ExtraPid[5]),
                  &(ExtraPid[6]), &(ExtraPid[7]), &(ExtraPid[8]) );
   if ( RetVal < 4 || RetVal > 12 || Send.Port <= 1023 ) {
      fprintf(stderr, "ERROR: main() - illegal arguments\nEXIT\n");
      fflush(stderr);
      exit(-1);
   }
   ExtraPidNum = RetVal-6;  // -1 => keine zus. PID's (wg. AVString)

   if ( BufNum > MAX_SPKT_BUF_NUM || BufNum < 5 ) {
      fprintf(stderr, "ERROR: main() - BufNum too large/small\nEXIT\n");
      fflush(stderr);
      exit(-1);
   }
   for (u=0; u<BufNum-1; u++) NextSPktBuf[u] = u+1;
   NextSPktBuf[BufNum-1] = 0;

   if ( Bouquet == 0 ) {
      StreamNum = 0;
   } else { 
      // Programm umschalten
      CZapitClient zapit;
      CZapitClient::responseGetPIDs pids;
 
      if ( !strcmp(CmdString, "VIDEO" ) ) {
         zapit.setMode(CZapitClient::MODE_TV);
      } else if ( !strcmp(CmdString, "AUDIO" ) ) {
         zapit.setMode(CZapitClient::MODE_RADIO);
      } else {
         fprintf(stderr, "ERROR: main() - illegal command\nEXIT\n");
         fflush(stderr);
         exit(-1);
      }
 
      zapit.zapTo(Bouquet-1,Channel);
      zapit.getPIDS(pids);
   
      // Pid's ermitteln
      StreamNum = 0;
      if (pids.PIDs.vpid) {
          Stream[StreamNum].Filter.pid = pids.PIDs.vpid;
          AVString[StreamNum] = 'v';
          Stream[StreamNum++].BufPacketNum = 
                 AV_BUF_FACTOR * AUDIO_BUF_PACKET_NUM;
      }
      for ( u=0; u<pids.APIDs.size(); u++) { 
         Stream[StreamNum].Filter.pid = pids.APIDs[u].pid;
         AVString[StreamNum] = 'a';
         Stream[StreamNum++].BufPacketNum = AUDIO_BUF_PACKET_NUM;
      }
   }

   for ( i=0; i<ExtraPidNum; i++) { 
      Stream[StreamNum].Filter.pid = ExtraPid[i];
      switch( ExtraAVString[i] ) {
         case 'v':
            AVString[StreamNum] = 'v';
            Stream[StreamNum++].BufPacketNum = 
               AV_BUF_FACTOR * AUDIO_BUF_PACKET_NUM;
            break;
         case 'a':
            AVString[StreamNum] = 'a';
            Stream[StreamNum++].BufPacketNum = AUDIO_BUF_PACKET_NUM;
            break;
        default:          
            fprintf(stderr, "ERROR: main() - illegal extra AV string \n");
            fflush(stderr);
            break;
      }
   }
   if (StreamNum == 0) {
      fprintf(stderr, "ERORR: main() - StreamNum == 0\nEXIT\n");
      fflush(stderr);
      exit(-1);
   }

   AVString[StreamNum] = 0;
   
 

   // Adresse fuer Send.Socket ermitteln
   Send.AddrLen = sizeof(struct sockaddr_in);
   if ( -1 == getpeername(STDIN_FILENO, 
                (struct sockaddr *)&(Send.Addr), &(Send.AddrLen)) ) {
      perror("ERROR: main() - getpeername");
      fprintf(stderr, "EXIT\n");
      fflush(stderr);
      exit(-1);
   }
   Send.Addr.sin_family = AF_INET;
   Send.Addr.sin_port   = htons( (unsigned short)(Send.Port) );
   printf("INFO: IP %x Port %u\n", Send.Addr.sin_addr.s_addr, Send.Port);
   fflush(stdout);

   // Antwort an Client
   printf("PID %s %i", AVString, StreamNum);
   for (u=0; u<StreamNum; u++) printf(" %x", Stream[u].Filter.pid);
   printf("\n");
   fflush(stdout);

   // START-Befehl abwarten
   ReadLine( TcpString );

   if ( !strncmp(TcpString, "STOP", 4) ) {   // ZapMode
      printf("INFO: zap mode\nEXIT\n" );
      fflush(stdout);
      exit(0);

   } else if ( strncmp(TcpString, "START", 5) ) {
      fprintf(stderr, "ERROR: main() - START command expected\nEXIT\n");
      fflush(stderr);
      exit(-1);
   }

   // *****************************************************
   // * Start streaming
   // *****************************************************

   // Super Packet Buffer initialisieren
   for (u=0; u<BufNum; u++) {
      SPkt.ReSendStatus[u] = 0;
      SPkt.Buf[u] = (unsigned char*) malloc( SPKT_BUF_SIZE );
      if ( SPkt.Buf[u] == 0 ) {
         fprintf(stderr, "ERROR: main() - malloc SPkt.Buf\nEXIT\n");
         fflush(stderr);
         exit(-1);
      }
   }
   SPkt.WritePkt = 0;
   SPkt.WriteBuf = 0;
   SPkt.ReadPkt = 0;
   SPkt.ReadBuf = 0;

   // UdpSender einrichten
   Send.Socket = -1;
   Send.Packet = 0;
   RestartUdpSender();

   // DmxReader vorbereiten
   StreamStop = 0;
   for (u=0; u<StreamNum; u++) {
      Stream[u].ReadBuf = 0;
      Stream[u].WriteBuf = 0;
      Stream[u].ReadPkt = 0;
      Stream[u].Stopped = 0;

      Stream[u].Ptr = (void*)&(Stream[u]);
      for( v=0; v<2; v++) {
         Stream[u].Buf[v] = (unsigned char*)malloc( 
               Stream[u].BufPacketNum * NET_DATA_PER_PACKET );
         if ( Stream[u].Buf[v] == 0 ) {
            fprintf(stderr, "ERROR: main() - malloc Stream.Buf\nEXIT\n");
            fflush(stderr);
            exit(-1);
         }
      }
   }        


   // Tcp Receiver starten -> STOP, RESEND Befehle auswerten
   if ( pthread_create(&TcpReceiverThread, 0, TcpReceiver, 0 ) ) {
      perror("ERROR: main() - TcpReceiver pthread_create");
      fprintf(stderr, "EXIT\n");
      fflush(stderr);
      exit(-1);
   }

   SchedParam.sched_priority = 10;
   pthread_attr_init(&ThreadAttr);
   pthread_attr_setschedpolicy(&ThreadAttr, SCHED_FIFO );
   pthread_attr_setschedparam(&ThreadAttr, &SchedParam);

   // DmxReader Thread's starten
   for (u=0; u<StreamNum; u++) {
      if ( pthread_create(&(Stream[u].Thread), &ThreadAttr, 
                            DmxReader, Stream[u].Ptr ) ) {
         perror("ERROR: main() - DmxReader pthread_create");
         fprintf(stderr, "EXIT\n");
         fflush(stderr);
         exit(-1);
      }
   }
   
   // ****************************************************************
   // Hauptschleife
   // ==============================================================
   // Pakete von den DmxBuf's in SPktBuf's umkopieren und 
   // Headern versehen
   // ****************************************************************
  
   StoppedDmxReaders = 0;
   do { 
      WritePtr = SPkt.Buf[SPkt.WriteBuf] + 
                    SPkt.WritePkt * DATA_PER_PACKET;
      for (u=0; u<BufNum; u++) {
         if ( SPkt.ReSendStatus) {
            for (v=0; v<SPKT_BUF_PACKET_NUM; v++) {
               if ( SPkt.ReSend[u][v] ) {
                  ReadPtr = SPkt.Buf[u] + v * DATA_PER_PACKET;
                  memcpy ( WritePtr, ReadPtr, DATA_PER_PACKET );
                  PacketHeader = (PacketHeaderType*)WritePtr;
                  PacketHeader->Packet = SPkt.WritePkt++;
                  PacketHeader->Status = 0;
                  PacketHeader->SPktBuf = SPkt.WriteBuf;
                  WritePtr += DATA_PER_PACKET;
                  CheckNextSPktWriteBuf( );
              }
              SPkt.ReSend[u][v] = 0;
           }
         }
      }
      EmptyStreamBuffers = 0;
      do {
         for(u=0; u<StreamNum; u++) {
            if ( Stream[u].Stopped ) {
               StoppedDmxReaders++;
               EmptyStreamBuffers++;
               continue;
            }
   
            if ( Stream[u].WriteBuf != Stream[u].ReadBuf ) {
               ReadPtr = Stream[u].Buf[Stream[u].ReadBuf];
         
               for (v=0; v<Stream[u].BufPacketNum; v++) {
                  PacketHeader = (PacketHeaderType*)WritePtr;
                  PacketHeader->Packet = SPkt.WritePkt++;
                  PacketHeader->Status = 0;
                  PacketHeader->SPktBuf = SPkt.WriteBuf;
                  PacketHeader->Stream = u;
                  PacketHeader->StreamPacket = Stream[u].ReadPkt++;
                  WritePtr += sizeof(PacketHeaderType);
                  memcpy ( WritePtr, ReadPtr, NET_DATA_PER_PACKET );
                  ReadPtr += NET_DATA_PER_PACKET;
                  WritePtr += NET_DATA_PER_PACKET;
                  CheckNextSPktWriteBuf( );
               }
               if ( Stream[u].ReadBuf == 0 ) Stream[u].ReadBuf = 1;
                                       else  Stream[u].ReadBuf = 0;
            } else {
               EmptyStreamBuffers++;
            }
         }
      } while ( EmptyStreamBuffers < StreamNum );      
      usleep(15000);  // 10% * 128 * 1468 * 8 / 10 MBit/s 
   } while (StoppedDmxReaders < StreamNum );
   PacketHeader->Status = 2; 
   SPkt.WriteBuf = NextSPktBuf[SPkt.WriteBuf];
   StreamStop = 0;
   while ( !StreamStop ) usleep(15000);
   printf("EXIT\n" );
   fflush(stdout);

   return 0;
}


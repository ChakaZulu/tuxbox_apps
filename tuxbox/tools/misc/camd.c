#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <ost/ca.h>
#include <ost/dmx.h>

int camfd;
/*
  cam init sequence:
  
  reset
  
  !0E
   9F 01 41
   9F 01 51
   A3 FF x15
  !0E
 > 44 00 00 17 00 00 01 e1 0e a7 7b 92 20 16 5d 5b 73 2e 90 60 7e 36 9b 9e d0 ea 38 ac 99 89 e9 26 5b f9 2c 73 b2 bd 38 b1 ea 2a 88 e4 97 38 d0 29 09 b7 f4 21 df 28 2b 27 52 f8 f8 a1 0f 6c 42 c7 cf 32 ca de 78 64 cb 91 
 > 44 01 f4 73 9d 11 0f 88 9a 6c 12 4f e9 d9 c6 85 83 4c fb b8 7a b7 e2 87 50 56 04 2c 62 6c 2d af 3a 7c 0e 4b 04 89 98 62 ee 73 8e 11 05 0d 42 25 fd f8 75 56 9c 09 1c b8 65 de 35 71 92 21 e6 e1 a1 f4 7b 
 >!F1 
   9F 01 61
 > 39
   9F 01 41
  !45 01
  !E1 ... DATA :)


 COMMANDs:
  (ABUS)
   E0 m->evCamReset
   E1 . firmware rev.
   
   45

*/

void _writecam(int cmd, unsigned char *data, int len)
{
  ca_msg_t ca_msg;
  char buffer[128];
  int csum=0, i;
  buffer[0]=0x6E;
  buffer[1]=0x50;
  buffer[2]=(len+1)|((cmd!=0x23)?0x80:0);
  buffer[3]=cmd;
  memcpy(buffer+4, data, len);
  len+=4;
  for (i=0; i<len; i++)
    csum^=buffer[i];
  buffer[len++]=csum;

  /* init ca message */
  ca_msg.slot_num = 0;
  ca_msg.type = 0;

  ca_msg.length = len-1;
  memcpy(ca_msg.msg,buffer+1,len-1);

//  if (write(camfd, buffer+1, len-1)!=len-1)
  if ( ioctl(camfd,CA_SEND_MSG,&ca_msg) != 0 )
  {
  	perror("ioctl");
  }

  printf("%d >",len);
  for (i=0; i<len; i++)
    printf(" %02x", buffer[i]);
  printf("\n");
}

void writecam(unsigned char *data, int len)
{
  _writecam(0x23, data, len);
}

void descramble(int onID, int serviceID, int unknown, int caID, int ecmpid, int vpid, int apid)
{
  unsigned char buffer[20];
  buffer[0]=0x0D;
  buffer[1]=onID>>8;
  buffer[2]=onID&0xFF;
  buffer[3]=serviceID>>8;
  buffer[4]=serviceID&0xFF;
  buffer[5]=unknown>>8;
  buffer[6]=unknown&0xFF;
  buffer[7]=caID>>8;
  buffer[8]=caID&0xFF;
  buffer[9]=ecmpid>>8;
  buffer[10]=ecmpid&0xFF;
  buffer[11]=0x02;
  buffer[12]=vpid>>8;
  buffer[13]=vpid&0xFF;
  buffer[14]=0x80;
  buffer[15]=0;
  buffer[16]=apid>>8;
  buffer[17]=apid&0xFF;
  buffer[18]=0x80;
  buffer[19]=0;
  writecam(buffer, 20);
}

void reset(void)
{
  char buffer[1];
  buffer[0]=0x9;
  writecam(buffer, 1);
}

void init(void)
{
  char buffer[1];
  buffer[0]=0x39;
  writecam(buffer, 1);
}

void init2(void)
{
  char buffer[1];
  buffer[0]=0x29;
  writecam(buffer, 1);
}

void start(int serviceID)
{
  char buffer[3];
  buffer[0]=0x3D;
  buffer[1]=serviceID>>8;
  buffer[2]=serviceID&0xFF;
  writecam(buffer, 3);
}

void setemm(int unknown, int caID, int emmpid)
{
  char buffer[7];
  buffer[0]=0x84;
  buffer[1]=unknown>>8;
  buffer[2]=unknown&0xFF;
  buffer[3]=caID>>8;
  buffer[4]=caID&0xFF;
  buffer[5]=emmpid>>8;
  buffer[6]=emmpid&0xFF;
  writecam(buffer, 7);
}

void set_key(void)
{
  char buffer[128];
  FILE *fp=fopen("caconfig.bin", "rb");
  if (!fp)
  {
    perror("caconfig.bin");
    return;
  }
  fread(buffer+1, 69, 1, fp);
  buffer[0]=0;
  _writecam(0x44, buffer, 70);
  fread(buffer+1, 64, 1, fp);
  buffer[0]=1;
  _writecam(0x44, buffer, 65);

  _writecam(0xF1, 0, 0);
  fclose(fp);
}

int find_emmpid(int ca_system_id) {
  char buffer[1000];
  int fd, r=1000,emm_pid=0,count;
  struct dmxSctFilterParams flt;

  fd=open("/dev/ost/demux0", O_RDONLY);
  if (fd<0)
  {
    perror("/dev/ost/demux0");
    return -fd;
  }

  memset(&flt.filter.filter, 0, DMX_FILTER_SIZE);
  memset(&flt.filter.mask, 0, DMX_FILTER_SIZE);

  flt.pid=1;
  flt.filter.filter[0]=1;

  flt.filter.mask[0]  =0xFF;
  flt.timeout=10000;
  flt.flags=DMX_ONESHOT;

  //flt.flags=0;
  if (ioctl(fd, DMX_SET_FILTER, &flt)<0)
  {
    perror("DMX_SET_FILTER");
    return 1;
  }

  ioctl(fd, DMX_START, 0);
  if ((r=read(fd, buffer, r))<=0)
  {
    perror("read");
    return 1;
  }

  close(fd);

  if (r<=0) return 0;

  r=buffer[2];

  //for(count=0;count<r-1;count++)
  //  printf("%02d: %02X\n",count,buffer[count]);

  count=8;
  while(count<r-1) {
    //printf("CAID %04X EMM: %04X\n",((buffer[count+2]<<8)|buffer[count+3]),((buffer[count+4]<<8)|buffer[count+5])&0x1FFF);
    if ((((buffer[count+2]<<8)|buffer[count+3]) == ca_system_id) && (buffer[count+2] == ((0x18|0x27)&0xD7)))
      return (((buffer[count+4]<<8)|buffer[count+5])&0x1FFF);
      count+=buffer[count+1]+2;
  }
  return 0;
}

int descriptor(char *buffer, int len, int ca_system_id) {
  int count=0;
  int desc,len2,ca_id,ca_pid=0;

  while(count<len) {
    desc=buffer[count++];
    len2=buffer[count++];
    if (desc == 0x09) {
      ca_id=(buffer[count]<<8)|buffer[count+1];
      count+=2;
      if ((ca_id == ca_system_id) && ((ca_id>>8) == ((0x18|0x27)&0xD7)))
	ca_pid=((buffer[count]&0x1F)<<8)|buffer[count+1];
      count+=2;
      count+=(len2-4);
    }
    else count+=len2;
  }
  return ca_pid;
}

int find_ecmpid(int pid,int ca_system_id) {
  char buffer[1000];
  int fd, r=1000,ecm_pid=0;
  struct dmxSctFilterParams flt;

  fd=open("/dev/ost/demux0", O_RDONLY);
  if (fd<0)
  {
    perror("/dev/ost/demux0");
    return -fd;
  }

  memset(&flt.filter.filter, 0, DMX_FILTER_SIZE);
  memset(&flt.filter.mask, 0, DMX_FILTER_SIZE);

  flt.pid=pid;
  flt.filter.filter[0]=2;

  flt.filter.mask[0]  =0xFF;
  flt.timeout=10000;
  flt.flags=DMX_ONESHOT;

  //flt.flags=0;
  if (ioctl(fd, DMX_SET_FILTER, &flt)<0)
  {
    perror("DMX_SET_FILTER");
    return 1;
  }

  ioctl(fd, DMX_START, 0);
  if ((r=read(fd, buffer, r))<=0)
  {
    perror("read");
    return 1;
  }

  {
    int pilen, dp;
    pilen=((buffer[10]&0xF)<<8)|buffer[11];
    dp=12;
    while (dp<(pilen+12)) {
      if (ecm_pid == 0)
	ecm_pid=descriptor(&buffer[12],pilen,ca_system_id);
      if (ecm_pid != 0)
	return ecm_pid;
      dp+=pilen;
    }
    while (dp<r-4)
    {
      int epid, esinfo;
      buffer[dp++];
      //printf("stream type: %x\n", buffer[dp]);
      epid=(buffer[dp++]&0x1F)<<8;
      epid|=buffer[dp++];
      esinfo=(buffer[dp++]&0xF)<<8;
      esinfo|=buffer[dp++];
      if (ecm_pid == 0)
	ecm_pid=descriptor(&buffer[dp],esinfo,ca_system_id);
      if (ecm_pid != 0)
	return ecm_pid;
      dp+=esinfo;
    }
  }
  close(fd);
  return 0;
}

int main(int argc, char **argv)
{
  ca_msg_t ca_msg;
  char cmd=0x03;
  int initok=0, caid=0;
  int SID=9, ONID=0x85, ECMPID=0, EMMPID=0, VPID=0x1FF, APID=0x200,pmt=0;
  
  camfd=open("/dev/ost/ca0", O_RDWR);

  if( camfd <= 0 )
  {
    perror("open ca0");
    return 1;
  }
  
  if (argc>=4)
  {
    sscanf(argv[1], "%x", &VPID);
    sscanf(argv[2], "%x", &APID);
    sscanf(argv[3], "%x", &pmt);
  }
  
  if (argc>=5)
  {
    sscanf(argv[4], "%x", &EMMPID);
  }
  
  reset();

  /* init ca message */
  ca_msg.slot_num = 0;
  ca_msg.type = 0;

//  msg.length = 0;
//  msg.msg


  writecam(&cmd,1);
  while (1)
  {
    char buffer[128];
    int i;
    int len, csum;

//    if (read(camfd, buffer, 4)!=4)

	ca_msg.length = 4;

	if ( ioctl(camfd,CA_GET_MSG,&ca_msg) != 0 )
    {
      perror("ioctl");
      break;
    }

	len = ca_msg.length;

//	printf("read %d bytes\n",len);

	if ( len <= 0 )
	{
	  usleep(500);
	  continue;
	}
	else
	{
		memcpy(buffer,ca_msg.msg,ca_msg.length);
	}

    if ((buffer[0]!=0x6F) || (buffer[1]!=0x50))
    {
      printf("out of sync! %02x %02x %02x %02x\n", buffer[0], buffer[1], buffer[2], buffer[3]);
      break;
    }
    len=buffer[2]&0x7F;

	ca_msg.length = len;

//    if (read(camfd, buffer+4, len)!=len)
	if ( ioctl(camfd,CA_GET_MSG,&ca_msg) != 0 )
    {
      perror("ioctl");
      break;
    }

	if ( ca_msg.length != len )
	{
		perror("length");
		break;
	}

	memcpy(buffer+4,ca_msg.msg,ca_msg.length);

    csum=0;
    for (i=0; i<len+4; i++)
      csum^=buffer[i];
    if (csum)
    {
      printf("checksum failed. packet was:");
      for (i=0; i<len+4; i++)
        printf(" %02x", buffer[i]);
      printf("\n");
      continue;
    }
    printf("<");
    for (i=0; i<len+4; i++)
      printf(" %02x", buffer[i]);
    printf("\n");
    if (buffer[3]==0x23)
    {
      switch ((unsigned)buffer[4])
      {
      case 4:
      case 5:
      case 6:
      case 7:
        printf("keep-alive(%d) %x casys %04x\n", buffer[4], buffer[5], (buffer[6]<<8)|buffer[7]);
        break;
      case 0x89:
	{
	  if (caid == 0) break;
	  printf("status89: %02x\n", buffer[5]);
	  if (EMMPID == 0) {
	    printf("searching EMM-pid for ca_system_ID %04X\n",caid);
	    EMMPID=find_emmpid(caid);
	    if (EMMPID == 0) {
	      printf("no EMM-pid found for ca_system_ID %04X\n",caid);
	      //printf("press enter to exit\n");
	      //getchar();
	      //exit(0);
	    }
	    else
	      printf("EMM-pid found: %04X\n",EMMPID);
	  }
	  if (EMMPID == 0) break;
	  setemm(0x104, caid, EMMPID);
	  if (ECMPID == 0) {
	    printf("searching ECM-pid for ca_system_ID %04X\n",caid);
	    ECMPID=find_ecmpid(pmt,caid);
	    if (ECMPID == 0) {
	      printf("no ECM-pid found for ca_system_ID %04X\n",caid);
	      //printf("press enter to exit\n");
	      //getchar();
	      //exit(0);
	    }
	    else
	      printf("ECM-pid found: %04X\n",ECMPID);
	  }
	  if (ECMPID == 0) break;
	  descramble(ONID, SID, 0x104, caid, ECMPID, APID, VPID);
	  break;
	}
      case 0x83:
	{
	  int newcaid=(buffer[6]<<8)|buffer[7];
	if ((caid != 0) && (newcaid != caid)) {
	  reset();
	  exit(0);
	}
        if (newcaid!=caid)
        {
	  printf("CAID is: %04X\n",newcaid);
          caid=newcaid;
	  reset();
        }
        break;
      }
      case 0x8D:
      {
        printf("descramble_answer\n");
        start(SID);
        break;
      }
      case 0x9F:
      {
        printf("cardslot %02x %02x\n", buffer[5], buffer[6]);
        if (!initok)
        {
          init();
          initok=1;
        }
        if (buffer[6])
          reset();
        break;
      }
      case 0xA3:
      {
        char tb[14];
        memcpy(tb, buffer+5, 13);
        tb[13]=0;
        printf("card-data: %s %02x %02x\n", tb, buffer[18], buffer[19]);
        break;
      }
      case 0xB9:
      {
        printf("statusB9: %02x\n", buffer[5]);
        break;
      }
      case 0xBD:
      {
        printf("status(%02x): service-id: %04x", buffer[7], (buffer[5]<<8)|buffer[6]);
        for (i=4; i<len; i+=3)
          printf(", pid %x:=%x", (buffer[4+i]<<8)|buffer[i+5], buffer[i+6]);
        break;
      }
      default:
        printf("unknown command. packet was:");
        for (i=0; i<len+4; i++)
          printf(" %02x", buffer[i]);
        printf("\n");
      }
    } else if (buffer[3]==0xE0)
    {
      printf("cam init complete event.\n");
      set_key();
    } else if (buffer[3]==0xE1)
    {
      char tb[32];
      memcpy(tb, buffer+4, len-1);
      tb[len-1]=0;
      printf("cam-revision: %s\n", tb, len);
      setemm(0x104, caid, 0x1500);
      init();

    } else 
    {
      printf("unknown command class %02x\n", buffer[3]);
    }
  }
  
  close(camfd);
}

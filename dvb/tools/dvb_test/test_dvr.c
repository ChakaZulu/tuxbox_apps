/* 
 * test.c - Test program for new API
 *
 * Copyright (C) 2000 Ralph  Metzler <ralph@convergence.de>
 *                  & Marcus Metzler <marcus@convergence.de>
                      for convergence integrated media GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend_old.h>
#include <linux/dvb/sec.h>
#include <linux/dvb/video.h>


main()
{
  int ret;
  int len;
  struct secCommand scmd;
  struct secCmdSequence scmds;
  struct dmx_pes_filter_params pesFilterParams; 
  struct dmx_sct_filter_params secFilterParams; 
  FrontendParameters frp;
  uint8_t buf[4096];
  uint64_t length;

  int dvrout=open("qq",O_WRONLY|O_CREAT|O_TRUNC,
		  S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|
		  S_IROTH|S_IWOTH);

  int fd_dvr;
  int fd_frontend;
  int fd_sec;
  int fd_demux;
  int fd_demux2;

  fd_dvr=open("/dev/ost/dvr1", O_RDONLY);
  fd_frontend=open("/dev/ost/frontend", O_RDWR);
  fd_sec=open("/dev/ost/sec1", O_RDWR);
  fd_demux=open("/dev/ost/demux1", O_RDWR|O_NONBLOCK);
  fd_demux2=open("/dev/ost/demux1", O_RDWR|O_NONBLOCK);

  fprintf (stderr,"Opening qq\n");

  fd_dvr=open("/dev/ost/dvr1", O_RDONLY);
#if 0
  fd_frontend=open("/dev/ost/frontend", O_RDWR);
  fd_sec=open("/dev/ost/sec", O_RDWR);
  fd_demux=open("/dev/ost/demux", O_RDWR|O_NONBLOCK);
  fd_demux2=open("/dev/ost/demux", O_RDWR|O_NONBLOCK);
#endif

  //if (ioctl(fd_sec, SEC_SET_VOLTAGE, SEC_VOLTAGE_13) < 0)  return;
  //if (ioctl(fd_sec, SEC_SET_TONE, SEC_TONE_ON) < 0)  return;
#if 0
  scmd.type=0;
  scmd.u.diseqc.addr=0x10;
  scmd.u.diseqc.cmd=0x38;
  scmd.u.diseqc.numParams=1;
  scmd.u.diseqc.params[0]=0xf0;
  
  scmds.voltage=SEC_VOLTAGE_13;
  scmds.miniCommand=SEC_MINI_NONE;
  scmds.continuousTone=SEC_TONE_ON;
  scmds.numCommands=1;
  scmds.commands=&scmd;
  if (ioctl(fd_sec, SEC_SEND_SEQUENCE, &scmds) < 0)  return;
  printf("SEC OK\n");

  frp.Frequency=(12666000-10600000);
  frp.u.qpsk.SymbolRate=22000000;
  frp.u.qpsk.FEC_inner= FEC_AUTO;

  if (ioctl(fd_frontend, FE_SET_FRONTEND, &frp) < 0)  return;
  printf("QPSK OK\n");
#endif  
#if 0
  pesFilterParams.pid = 0xa2;
  pesFilterParams.input = DMX_IN_FRONTEND; 
  pesFilterParams.output = DMX_OUT_TS_TAP; 
  pesFilterParams.pes_type = DMX_PES_VIDEO; 
  pesFilterParams.flags = DMX_IMMEDIATE_START;
  if (ioctl(fd_demux, DMX_SET_PES_FILTER, &pesFilterParams) < 0){
    return(1); 
  }
  printf("Video filter OK\n");

  printf("Audio filter size OK\n");
  pesFilterParams.pid = 0x60; 
  pesFilterParams.input = DMX_IN_FRONTEND; 
  pesFilterParams.output = DMX_OUT_TS_TAP; 
  pesFilterParams.pes_type = DMX_PES_AUDIO; 
  pesFilterParams.flags = DMX_IMMEDIATE_START;
  if (ioctl(fd_demux2, DMX_SET_PES_FILTER, &pesFilterParams) < 0)   return(1); 
  printf("Audio filter OK\n");
#endif
  length = 0;
  while (1) {
    len=read(fd_dvr, buf, 4096);
    if (len>0) len=write (dvrout, buf, len);
    length += len;
    fprintf(stderr,"written %2.2fMB\r",length/1024./1024.);
  }
}


#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include <ost/sec.h>

#define SEC_DEV   "/dev/ost/sec0"
#define CMD_MAXLEN 1024
#define VERSION "0.1a"

void help(char addr) {
  printf("commands:
address [any|polar|elevation]    DiSEqC device to address (30/31/32)
                                 current address: %02X
reset                            reset
sleep                            sleep
awake                            wakeup
standby                          standby
power                            power on
halt                             stop movement
nolimits                         disable limits
status                           get status (needs DiSEqC 2.2 !!)
limit_east                       set east limit
limit_west                       set west limit
east (<value>)                   drive east <value>
west (<value>)                   drive west <value>
store <value>                    store position <value>
goto <value>                     goto position <value>
drive <value>(.<value>)          drive to angular position
recalc (<value> (<value>))       recalculate satellite positions
exit                             leave
\n",addr);
}

int main(int argc, char **argv) {
  int device,count,foo;
  struct secCmdSequence seq;
  struct secCommand cmd;
  struct secDiseqcCmd diseqc;
  char command[CMD_MAXLEN]="";

  printf("skyscan %s (DiSEqC positioner control) (C) by Hunz 2001\n",VERSION);
  printf("type \"help\" to see a list of supported commands\n");

  if((device = open(SEC_DEV, O_RDWR)) < 0) {
    printf("Cannot open SEC device \"%s\"\n",SEC_DEV);
    exit(1);
  }

  diseqc.addr=0x31;
  cmd.type=SEC_CMDTYPE_DISEQC;
  seq.numCommands=1;
  seq.miniCommand=SEC_MINI_NONE;

  while((strcmp(command,"quit") != 0) && (strcmp(command,"exit") != 0)) {
    
    diseqc.cmd=0;

    if (strcmp(command,"help") == 0)
      help(diseqc.addr);
    else if(strncmp(command,"address",7) == 0) {
      for(count=7;(count<strlen(command))&&(command[count]==0x20);count++) {}
      if(strcmp(&command[count],"any") == 0)
	diseqc.addr=0x31;
      else if (strcmp(&command[count],"polar") == 0)
	diseqc.addr=0x32;
      else if (strcmp(&command[count],"elevation") == 0)
	diseqc.addr=0x33;
      else
	printf("illegal address - valid are: any polar elevation\n");
    }
    else if(strcmp(command,"standby") == 0) {
      diseqc.cmd=0x02;
      diseqc.numParams=0;
    }
    else if(strcmp(command,"power") == 0) {
      diseqc.cmd=0x03;
      diseqc.numParams=0;
    }
    else if(strcmp(command,"sleep") == 0) {
      diseqc.cmd=0x30;
      diseqc.numParams=0;
    }
    else if(strcmp(command,"awake") == 0) {
      diseqc.cmd=0x31;
      diseqc.numParams=0;
    }
    else if(strcmp(command,"nolimits") == 0) {
      diseqc.cmd=0x63;
      diseqc.numParams=0;
    }
    else if(strcmp(command,"status") == 0) {
      diseqc.cmd=0x64;
      diseqc.numParams=0;
    }
    else if(strcmp(command,"limit_east") == 0) {
      diseqc.cmd=0x66;
      diseqc.numParams=0;
    }
    else if(strcmp(command,"limit_west") == 0) {
      diseqc.cmd=0x67;
      diseqc.numParams=0;
    }
    else if(strncmp(command,"recalc",6) == 0) {
      for(count=6;(command[count]>=0x30)&&(command[count]<=0x39)&&(count<strlen(command));count++) {}
      if (count < strlen(command)) {
	foo=atoi(&command[count]);
	diseqc.params[0]=foo;
	for(;(command[count]!=0x20)&&(count<strlen(command));count++) {}
	for(;(command[count]==0x20)&&(count<strlen(command));count++) {}
	if(count<strlen(command)) {
	  if((diseqc.params[1]=atoi(&command[count+1])))
	    diseqc.numParams=2;
	  else diseqc.numParams=1;
	}
	else diseqc.numParams=1;
      }
      else diseqc.numParams=0;
      diseqc.cmd=0x6F;
    }
    else if(strncmp(command,"east",4) == 0) {
      for(count=4;(command[count]>=0x30)&&(command[count]<=0x39)&&(count<strlen(command));count++) {}
      if (count < strlen(command)) {
	foo=atoi(&command[count]);
	diseqc.params[0]=foo;
	diseqc.numParams=1;
      }
      else
	diseqc.numParams=0;
      diseqc.cmd=0x68;
    }
    else if(strncmp(command,"west",4) == 0) {
      for(count=4;(command[count]>=0x30)&&(command[count]<=0x39)&&(count<strlen(command));count++) {}
      if (count < strlen(command)) {
	foo=atoi(&command[count]);
	diseqc.params[0]=foo;
	diseqc.numParams=1;
      }
      else
	diseqc.numParams=0;
      diseqc.cmd=0x69;
    }
    else if(strcmp(command,"halt") == 0) {
      diseqc.cmd=0x60;
      diseqc.numParams=0;
    }
    else if(strcmp(command,"reset") == 0) {
      diseqc.cmd=0x0;
      diseqc.numParams=0;
    }
    else if(strncmp(command,"goto",4) == 0) {
      for(count=4;(command[count]>=0x30)&&(command[count]<=0x39)&&(count<strlen(command));count++) {}
      if (count < strlen(command)) {
	foo=atoi(&command[count]);
	diseqc.params[0]=foo;
	diseqc.numParams=1;
	diseqc.cmd=0x6B;
      }
      else printf("no value given\n");
    }
    else if(strncmp(command,"store",5) == 0) {
      for(count=5;(command[count]>=0x30)&&(command[count]<=0x39)&&(count<strlen(command));count++) {}
      if (count < strlen(command)) {
	foo=atoi(&command[count]);
	diseqc.params[0]=foo;
	diseqc.numParams=1;
	diseqc.cmd=0x6A;
      }
      else printf("no value given\n");
    }
    else if(strncmp(command,"drive",5) == 0) {
      for(count=5;(command[count]>=0x30)&&(command[count]<=0x39)&&(count<strlen(command));count++) {}
      if (count < strlen(command)) {
	foo=atoi(&command[count]);
	diseqc.params[0]=foo;
	for(;(command[count]!='.')&&(count<strlen(command));count++) {}
	if (command[count]=='.') {
	  diseqc.params[1]=atoi(&command[count+1]);
	  diseqc.numParams=2;
	}
	else diseqc.numParams=1;
	diseqc.cmd=0x6E;
      }
      else printf("no value given\n");
    }
    else if (strlen(command)>0)
      printf("unknown command\n");

    if (diseqc.cmd > 0) { 
      cmd.u.diseqc=diseqc;      
      seq.commands=&cmd;
      printf("%d\n",ioctl(device,SEC_SEND_SEQUENCE,&seq));
    }

    printf("> ");
    fflush(stdin);
    
    fgets(command,CMD_MAXLEN,stdin);
    command[strlen(command)-1]=0;
    
  }

  close(device);

  return 0;
}

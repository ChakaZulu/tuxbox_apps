#include "remotecontrol.h"



CRemoteControl::CRemoteControl()
{
	memset(&remotemsg, 0, sizeof(remotemsg) );
}

void CRemoteControl::send()
{
	int sock_fd;
	SAI servaddr;
	char rip[]="127.0.0.1";

	sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(1500);
	inet_pton(AF_INET, rip, &servaddr.sin_addr);

	#ifdef HAS_SIN_LEN
 		servaddr.sin_len = sizeof(servaddr); // needed ???
	#endif


	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
	{
  		perror("Couldn't connect to server!");
		return;
	}

	write(sock_fd, &remotemsg, sizeof(remotemsg));
	close(sock_fd);
	memset(&remotemsg, 0, sizeof(remotemsg));
}

void CRemoteControl::zapTo(int, string chnlname )
{
        remotemsg.version=1;
        remotemsg.cmd=3;
        remotemsg.param=0x0100;
        strcpy( remotemsg.param3, chnlname.c_str() );

	send();
}

void  CRemoteControl::shutdown()
{
        remotemsg.version=1;
        remotemsg.cmd=4;

        send();
}

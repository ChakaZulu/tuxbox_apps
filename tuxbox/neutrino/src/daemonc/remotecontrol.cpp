#include "remotecontrol.h"



CRemoteControl::CRemoteControl()
{
	memset(&remotemsg, 0, sizeof(remotemsg) );
}

void CRemoteControl::setZapper(bool zapper)
{
	zapit_mode = zapper;
}


void CRemoteControl::send()
{
	int sock_fd;
	SAI servaddr;
	char rip[]="127.0.0.1";

	sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	
	if (zapit_mode) {
		char *return_buf;
		int bytes_recvd = 0;
	
		servaddr.sin_port=htons(1505);
	inet_pton(AF_INET, rip, &servaddr.sin_addr);

	#ifdef HAS_SIN_LEN
 		servaddr.sin_len = sizeof(servaddr); // needed ???
	#endif


	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
	{
  		perror("Couldn't connect to server!");
		exit(-1);
	}

	write(sock_fd, &remotemsg, sizeof(remotemsg));
	
	
	return_buf = (char*) malloc(4);
	memset(return_buf,0,sizeof(return_buf));
	bytes_recvd = recv(sock_fd, return_buf, 3,0);
	if (bytes_recvd <= 0 ) {
		perror("Nothing could be received by Neutrino\n");
		exit(-1);
	}
	printf("Received %d bytes\n", bytes_recvd);
	
	printf("That was returned: %s\n", return_buf);
	
	switch (atoi(return_buf))
	{
		case 0: printf("Unknown error reported from zapper\n");
			exit(-1);
			break;
		case 1: printf("Zapping by number returned successful\n");
		break;
		case -1: printf("Zapping by number returned UNsuccessful\n");
		break;
		case 2: printf("zapit should be killed now.\n");
		break;
		case -2: printf("zapit could not be killed\n");
		break;
		case 3: printf("Zapping by name returned successful\n");
		break;
		case -3: printf("\n\nHier wäre Platz für ne Fehlerbild-funktion\n\n");
		break;
		case 4: printf("Shutdown Box returned successful\n");
		break;
		case -4: printf("Shutdown Box was not succesful\n");
		break;
		case 5: printf("get Channellist returned successful\n");
			printf("Should not be received in remotecontrol.cpp. Exiting\n");
			exit(-1);
		break;
		case -5: printf("get Channellist returned UNsuccessful\n");
			printf("Should not be received in remotecontrol.cpp. Exiting\n");
			exit(-1);
		break;
		case 6: printf("Changed to radio-mode\n");
		break;
		case -6: printf("Could not change to radio-mode\n");
		break;
		case 7: printf("Changed to TV-mode\n");
		break;
		case -7: printf("Could not change to TV-Mode\n");
		break;
		case 8: printf("Got a pid-description\n");
			printf("should not be done in remotecontrol.cpp.\n");
		break;
		case -8: printf("Could not get a pid-description\n");
			printf("should not be done in remotecontrol.cpp.\n");
		break;
		case 9: printf("Changed apid\n");
		break;
		case -9: printf("Could not change apid\n");
		break;
		default: printf("Unknown return-code\n");
			exit(-1);
		}
		
	
	close(sock_fd);
	
	memset(&remotemsg, 0, sizeof(remotemsg));
	return;
} else {
	
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

}

void CRemoteControl::zapTo(int, string chnlname )
{
        remotemsg.version=1;
        remotemsg.cmd=3;
        remotemsg.param=0x0100;
        strcpy( remotemsg.param3, chnlname.c_str() );

	send();
}

void CRemoteControl::radioMode()
{
	remotemsg.version=1;
	remotemsg.cmd=6;
	
	send();
}

void CRemoteControl::tvMode()
{
	remotemsg.version=1;
	remotemsg.cmd=7;
	
	send();
}


void  CRemoteControl::shutdown()
{
        remotemsg.version=1;
        remotemsg.cmd=4;

        send();
}

#include "remotecontrol.h"



CRemoteControl::CRemoteControl()
{
	memset(&remotemsg, 0, sizeof(remotemsg) );

    pthread_cond_init( &send_cond, NULL );
    pthread_mutex_init( &send_mutex, NULL );

    if (pthread_create (&thrSender, NULL, RemoteControlThread, (void *) this) != 0 )
	{
		perror("CRemoteControl: Create RemoteControlThread failed\n");
	}
}

void CRemoteControl::setZapper(bool zapper)
{
	zapit_mode = zapper;
}


void CRemoteControl::send()
{
    pthread_cond_signal( &send_cond );
    usleep(10);
//    printf("CRemoteControl: after pthread_cond_signal (with %s)\n", remotemsg.param3);
}


void * CRemoteControl::RemoteControlThread (void *arg)
{
	CRemoteControl* RemoteControl = (CRemoteControl*) arg;

    int sock_fd;
	SAI servaddr;
	char rip[]="127.0.0.1";
    bool redo;

	while(1)
	{
//        printf("CRemoteControl: before pthread_cond_wait\n");

        pthread_mutex_trylock( &RemoteControl->send_mutex );
        pthread_cond_wait( &RemoteControl->send_cond, &RemoteControl->send_mutex );
      
//        printf("CRemoteControl: after pthread_cond_wait for %s\n", RemoteControl->remotemsg.param3);

        st_rmsg r_msg;

        do
        {
            pthread_mutex_trylock( &RemoteControl->send_mutex );
            memcpy( &r_msg, &RemoteControl->remotemsg, sizeof(r_msg) );
            pthread_mutex_unlock( &RemoteControl->send_mutex );

            memset(&servaddr,0,sizeof(servaddr));
            servaddr.sin_family=AF_INET;

            #ifdef HAS_SIN_LEN
            servaddr.sin_len = sizeof(servaddr); // needed ???
           	#endif
            inet_pton(AF_INET, rip, &servaddr.sin_addr);
            sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

            if ( RemoteControl->zapit_mode )
            {
                char *return_buf;
                int bytes_recvd = 0;
	
                servaddr.sin_port=htons(1505);
            	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
            	{
              		perror("CRemoteControl::RemoteControlThread - Couldn't connect to serverd zapit!");
//            		exit(-1);
            	}

                write(sock_fd, &r_msg, sizeof(r_msg));
	
                return_buf = (char*) malloc(4);
                memset(return_buf,0,sizeof(return_buf));
                bytes_recvd = recv(sock_fd, return_buf, 3,0);
                if (bytes_recvd <= 0 )
                {
                    perror("CRemoteControl::RemoteControlThread - Nothing could be received from serverd zapit\n");
//                    exit(-1);
                }
//                printf("Received %d bytes\n", bytes_recvd);
//                printf("That was returned: %s\n", return_buf);
	
                switch (atoi(return_buf))
                {
                    case 0: printf("Unknown error reported from zapper\n");
//                            exit(-1);
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
//                            exit(-1);
                            break;
                    case -5: printf("get Channellist returned UNsuccessful\n");
                            printf("Should not be received in remotecontrol.cpp. Exiting\n");
//                            exit(-1);
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
//                            exit(-1);
                }
                usleep(250000);
            }
            else
            {
                servaddr.sin_port=htons(1500);
                if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))!=-1)
                {
//                    printf("CRemoteControl: before write to socket %s\n", r_msg.param3);
                    write(sock_fd, &r_msg, sizeof(r_msg) );

                    usleep(1500000);
                };
            }

            close(sock_fd);

            pthread_mutex_trylock( &RemoteControl->send_mutex );
            redo= memcmp(&r_msg, &RemoteControl->remotemsg, sizeof(r_msg)) != 0;

        } while ( redo );
	}
	return NULL;
}


void CRemoteControl::zapTo(int, string chnlname )
{
    pthread_mutex_lock( &send_mutex );

        remotemsg.version=1;
        remotemsg.cmd=3;
        remotemsg.param=0x0100;
        strcpy( remotemsg.param3, chnlname.c_str() );

    pthread_mutex_unlock( &send_mutex );

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




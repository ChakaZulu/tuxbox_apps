#include "scan.h"
#include "../global.h"

CScanTs::CScanTs()
{
	width = 400;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	height = hheight+5*mheight;
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;
}

bool CScanTs::scanReady(int *ts, int *services)
{
		int sock_fd;
		SAI servaddr;
		char rip[]="127.0.0.1";
		char *return_buf;
		st_rmsg		sendmessage;

		sendmessage.version=1;
		sendmessage.cmd = 'h';

		sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		memset(&servaddr,0,sizeof(servaddr));
		servaddr.sin_family=AF_INET;
		servaddr.sin_port=htons(1505);
		inet_pton(AF_INET, rip, &servaddr.sin_addr);

		#ifdef HAS_SIN_LEN
 			servaddr.sin_len = sizeof(servaddr); // needed ???
		#endif


		if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
		{
 	 		perror("neutrino: connect(zapit)");
			exit(-1);
		}

		write(sock_fd, &sendmessage, sizeof(sendmessage));
		return_buf = (char*) malloc(4);
		memset(return_buf, 0, 4);
		if (recv(sock_fd, return_buf, 3,0) <= 0 ) {
			perror("recv(zapit)");
			exit(-1);
		}
	
		printf("scan: %s", return_buf);
		if (return_buf[0] == '-')
		{
			free(return_buf);
			close(sock_fd);
			return true;
		}
		else
		{
			if (recv(sock_fd, ts, sizeof(int),0) <= 0 ) {
				perror("recv(zapit)");
				exit(-1);
			}
			if (recv(sock_fd, services, sizeof(int),0) <= 0 ) {
				perror("recv(zapit)");
				exit(-1);
			}
			printf("Found transponders: %d\n", *ts);
			printf("Found channels: %d\n", *services);
			free(return_buf);
			close(sock_fd);
			return false;
		}

}

void CScanTs::startScan()
{
		int sock_fd;
		SAI servaddr;
		char rip[]="127.0.0.1";
		char *return_buf;
		st_rmsg		sendmessage;

		sendmessage.version=1;
		sendmessage.cmd = 'g';

		sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		memset(&servaddr,0,sizeof(servaddr));
		servaddr.sin_family=AF_INET;
		servaddr.sin_port=htons(1505);
		inet_pton(AF_INET, rip, &servaddr.sin_addr);

		#ifdef HAS_SIN_LEN
 			servaddr.sin_len = sizeof(servaddr); // needed ???
		#endif


		if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
		{
 	 		perror("neutrino: connect(zapit)");
			exit(-1);
		}

		write(sock_fd, &sendmessage, sizeof(sendmessage));
		return_buf = (char*) malloc(4);
		memset(return_buf, 0, 4);
		if (recv(sock_fd, return_buf, 3,0) <= 0 ) {
			perror("recv(zapit)");
			exit(-1);
		}
	
		printf("startscan: %s", return_buf);
		free(return_buf);
		close(sock_fd);
}


int CScanTs::exec(CMenuTarget* parent, string)
{
	if (parent)
	{
		parent->hide();
	}
	paint();

	int key = g_RCInput->getKey(190);
	if(key != CRCInput::RC_ok)
	{
		hide();
		return CMenuTarget::RETURN_REPAINT;
	}

	startScan();
	
	char buf[100];
	int ts = 0;
	int services = 0;
	while (!scanReady(&ts, &services))
	{
		int ypos=y;
		g_FrameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);
		ypos+= hheight + (mheight >>1);
		
		sprintf(buf, "%s %d", g_Locale->getText("scants.transponders").c_str(), ts);
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);
		ypos+= mheight;

		sprintf(buf, "%s %d", g_Locale->getText("scants.services").c_str(), services);
		g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, buf, COL_MENUCONTENT);
		//g_RCInput->getKey(190);
		sleep(3);
	}


	hide();
	return CMenuTarget::RETURN_REPAINT;
}

void CScanTs::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height);
}


void CScanTs::paint()
{
	int ypos=y;
	g_FrameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10, ypos+ hheight, width, g_Locale->getText("scants.head").c_str(), COL_MENUHEAD);
	g_FrameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT);

	ypos+= hheight + (mheight >>1);
	
	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.info1").c_str(), COL_MENUCONTENT);
	ypos+= mheight;

	g_Fonts->menu->RenderString(x+ 10, ypos+ mheight, width, g_Locale->getText("scants.info2").c_str(), COL_MENUCONTENT);
	ypos+= mheight;

}

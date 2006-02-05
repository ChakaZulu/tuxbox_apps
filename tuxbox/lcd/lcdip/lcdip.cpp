// The -i option was implemented by Barf on 2006-02-05. It creates the file
// /etc/network/interfaces (brutally overwriting any existing
// such). Gateway address, netmask, and broadcast address are filled
// in using heuristics, although the latter two should be right in 99%
// of all cases.

#include <stdio.h>
#include <errno.h>
#include <linux/input.h>

#include "lcddisplay.h"
#include "rc.h"

CLCDDisplay display;

int pt[3]={1, 10, 100};

int main(int argc, char *argv[])
{
	bool write_etc_network_interfaces = false;
	int ip[4]={192,168,0,1};

	if (argc > 1 && strcmp(argv[1], "-i") == 0)
		write_etc_network_interfaces = true;

	display.draw_string(10, 10, "IP eingeben:");
	
	FILE* pFile=fopen("/var/ip.lcdip", "r"); // tum lesen "w" zum schreiben
	if ((pFile == NULL)&&(errno != ENOENT)){
		printf ("error: %d ",errno);
		perror ("/var/ip.lcdip");
		return 1;
	} 
	else if (pFile != NULL){
		int err = fscanf (pFile,"%03d.%03d.%03d.%03d\n",&ip[0],&ip[1],&ip[2],&ip[3]);
		if ((err == EOF) ||( err<4)){
			printf ("error: /var/ip.lcdip too short, using default\n");
			ip[0]=192;ip[1]=168;ip[2]=0;ip[3]=1;
		}
		fclose(pFile);
	}

	int curpos=0;
	int update=1;
	int done=0;
	RCOpen();
	while (!done)
	{
		if (!update)
		{
			int code;
			switch (code=RCGet())
			{
			case KEY_LEFT:
				if (curpos)
				{
					curpos--;
					update=1;
				}
				break;
			case KEY_RIGHT:
				if (curpos<14)
				{
					curpos++;
					update=1;
				}
				break;
			case KEY_UP:
			{
				if ((curpos%4)==3)
					break;
				int i=curpos/4;
				int a=2-(curpos%4);
				int nv=ip[i];
				int s=(nv/pt[a])%10;
				nv-=s*pt[a];
				s++;
				if (s>9)
					s=0;
				nv+=s*pt[a];
				ip[i]=nv; 
				update=1;
				break;
			}

			case KEY_0:
			case KEY_1:
			case KEY_2:
			case KEY_3:
			case KEY_4:
			case KEY_5:
			case KEY_6:
			case KEY_7:
			case KEY_8:
			case KEY_9:
			{
				unsigned char keynumber[10]={KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9};
				int number;
				for (number=0;number<10;number++) if (code==keynumber[number]) break;
				if ((curpos%4)==3)
					break;
				int i=curpos/4;
				int a=2-(curpos%4);
				int nv=ip[i];
				int s=(nv/pt[a])%10;
				nv-=s*pt[a];
				s=number;
				nv+=s*pt[a];
				ip[i]=nv; 
				update=1;
				if (curpos<14)
				{
					curpos++;
					if ((curpos%4)==3)
						curpos++;
				} /* else
					done=1;*/ 
				break;
			}
			case KEY_DOWN:
			{
				if ((curpos%4)==3)
					break;
				int i=curpos/4;
				int a=2-(curpos%4);
				int nv=ip[i];
				int s=(nv/pt[a])%10;
				nv-=s*pt[a];
				s--;
				if (s<0)
					s=9;
				nv+=s*pt[a];
				ip[i]=nv; 
				update=1;
				break;
			}
			case KEY_POWER:
				if (curpos<14){
					curpos++;
					update=1;
					break;
				}
			case KEY_OK:
				done=1;
				update=1;
				break;
			}
		}
		if (update)
		{
			char ips[16];
			sprintf(ips, "%03d.%03d.%03d.%03d", ip[0], ip[1], ip[2], ip[3]);
			if (done)
			{
				if ((ip[0]>255) || (ip[1]>255) || (ip[2]>255) || (ip[3]>255))
				{
					done=0;
					display.draw_string(0, 30, "  INVALID IP!  ");
					curpos = 0;
					display.update();
					int code = RCGet();
					while ((code!=KEY_OK)&&(code!=KEY_POWER));
				}
			}			
			display.draw_string(0, 30, ips);
			if (!done)
				display.draw_rectangle(curpos*8, 30, curpos*8+7, 37, LCD_PIXEL_INV, LCD_PIXEL_INV);
			display.update();
			update=0;
		}
	}
	RCClose();
	
	if (write_etc_network_interfaces) {
		FILE* paFile=fopen("/etc/network/interfaces", "w"); 
		if (paFile == NULL){
			perror ("writing /etc/network/interfaces");
			return 2;
		}

		fprintf (paFile,
			 "auto lo\n"
			 "iface lo inet loopback\n\n"
			 "auto eth0\n"
			 "iface eth0 inet static\n"
			 "address %d.%d.%d.%d\n"
			 "gateway %d.%d.%d.0\n"
			 "netmask 255.255.255.0\n"
			 "broadcast %d.%d.%d.255\n",
			 ip[0],ip[1],ip[2],ip[3],
			 ip[0],ip[1],ip[2],
			 ip[0],ip[1],ip[2]);
		fclose(paFile);
	} else {
	FILE* paFile=fopen("/var/ip.lcdip", "w"); 
		if (paFile == NULL){
			perror ("writing /var/ip.lcdip");
			return 2;
		}

		fprintf (paFile,"%03d.%03d.%03d.%03d\n",ip[0],ip[1],ip[2],ip[3]);
		fclose(paFile);
	}

// if needed: replace with whatever you want to do with the ip...
/*	char exec[100];
	sprintf(exec, "/sbin/ifconfig eth0 %d.%d.%d.%d up", ip[0], ip[1], ip[2], ip[3]);
	if (!(system(exec)>>8))
	{
		display.draw_string(0, 30, "  IP GESETZT!  ");
		display.update();
	} else
	{
		display.draw_string(0, 30, "    FEHLER!    ");
		display.update();
	}
*/
	display.draw_string(0, 30, "    IP OK!     ");
	display.update();
	return 0;
}

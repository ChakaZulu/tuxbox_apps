#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <linux/route.h>

static	void	scanip( char *str, unsigned char *to )
{
	int		val;
	int		c;
	char	*sp;
	int		b=4;

	for( sp=str; b; b-- )
	{
		val=0;
		for(; (*sp != 0) && (*sp != '.'); sp++)
		{
			c = *sp - '0';
			if (( c < 0 ) || ( c>=10))
				break;
			val=(val*10)+c;
		}
		*to=val;
		to++;
		if ( !*sp )
			break;
		sp++;
	}
}
	
int	netSetIP( char *dev, char *ip, char *mask, char *brdcast )
{
	int					fd;
	struct sockaddr_in	*in_addr;
	struct ifreq		*ifr;
	struct ifreq		req;
	struct ifconf		ifc;
	char				buf[2048];
	unsigned char		*addr;
	int					n;
	char				found=0;
	int					rc=-1;
	unsigned char		adr_ip[4];
	unsigned char		adr_mask[4];
	unsigned char		adr_brdcast[4];

	fd=socket(AF_INET,SOCK_DGRAM,0);
	if ( !fd )
		return -1;

	scanip( ip, adr_ip );
	scanip( mask, adr_mask );
	scanip( brdcast, adr_brdcast );

/* alle devices auslesen */
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;

	while( 1 )	/* trick jump */
	{
		if( ioctl(fd,(int)SIOCGIFCONF, (char*)&ifc ) < 0 )
			break;

		for( ifr = ifc.ifc_req, n = ifc.ifc_len / sizeof(struct ifreq);
			--n >= 0;
			ifr++ )
		{
			if ( !strcmp(ifr->ifr_name,dev) )
			{
				found=1;
				break;
			}
		}

		if ( !found )
			break;

		req = *ifr;
		in_addr = (struct sockaddr_in *)&req.ifr_addr;
		addr = (unsigned char*)&in_addr->sin_addr.s_addr;

		memcpy(addr,adr_ip,4);
		if( ioctl(fd,SIOCSIFADDR,&req) == -1 )
			break;

		memcpy(addr,adr_mask,4);
		if( ioctl(fd,SIOCSIFNETMASK,&req) == -1 )
			break;

		memcpy(addr,adr_brdcast,4);
		if( ioctl(fd,SIOCSIFBRDADDR,&req) == -1 )
			break;

		rc=0;
		break;
	}

	close(fd);

	return rc;
}

void	netGetIP( char *dev, char *ip, char *mask, char *brdcast )
{
	int					fd;
	struct sockaddr_in	*in_addr;
	struct ifreq		*ifr;
	struct ifreq		req;
	struct ifconf		ifc;
	char				buf[2048];
	unsigned char		*addr;
	int					n;
	char				found=0;

	*ip=0;
	*mask=0;
	*brdcast=0;

	fd=socket(AF_INET,SOCK_DGRAM,0);
	if ( !fd )
		return;

/* alle devices auslesen */
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;

	while( 1 )	/* trick jump */
	{
		if( ioctl(fd,(int)SIOCGIFCONF, (char*)&ifc ) < 0 )
			break;

		for( ifr = ifc.ifc_req, n = ifc.ifc_len / sizeof(struct ifreq);
			--n >= 0;
			ifr++ )
		{
			if ( !strcmp(ifr->ifr_name,dev) )
			{
				found=1;
				break;
			}
		}

		if ( !found )
			break;

		req = *ifr;
		in_addr = (struct sockaddr_in *)&req.ifr_addr;
		addr=(unsigned char*)&in_addr->sin_addr.s_addr;

		if( ioctl(fd,SIOCGIFADDR,&req) == -1 )
			break;

		sprintf(ip,"%d.%d.%d.%d",addr[0],addr[1],addr[2],addr[3]);

		if( ioctl(fd,SIOCGIFNETMASK,&req) == -1 )
			break;

		sprintf(mask,"%d.%d.%d.%d",addr[0],addr[1],addr[2],addr[3]);

		if( ioctl(fd,SIOCGIFBRDADDR,&req) == -1 )
			break;

		sprintf(brdcast,"%d.%d.%d.%d",addr[0],addr[1],addr[2],addr[3]);

		break;
	}

	close(fd);

	return;
}

void	netSetDefaultRoute( char *gw )
{
	struct rtentry		re;
	struct sockaddr_in	*in_addr;
	unsigned char		*addr;
	int					fd;
	unsigned char		adr_gw[4];

	scanip( gw, adr_gw );

	memset(&re,0,sizeof(struct rtentry));

	in_addr = (struct sockaddr_in *)&re.rt_dst;
	in_addr->sin_family = AF_INET;

	in_addr = (struct sockaddr_in *)&re.rt_gateway;
	in_addr->sin_family = AF_INET;
	addr=(unsigned char*)&in_addr->sin_addr.s_addr;

	in_addr = (struct sockaddr_in *)&re.rt_genmask;
	in_addr->sin_family = AF_INET;

	fd=socket(AF_INET,SOCK_DGRAM,0);
	if ( !fd )
		return;

	re.rt_flags = RTF_GATEWAY | RTF_UP;
	memcpy(addr,adr_gw,4);

	ioctl(fd,SIOCADDRT,&re);

	close(fd);
	return;
}

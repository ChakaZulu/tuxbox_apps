/*
$Log: gameplugins.h,v $
Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef __gameplugin__
#define __gameplugin__

/*
** akt. pluginversion : 1
sie enthaelt :
	int		pluginversion;
	char	name[20];
	char	desc[100];
	int		type;
	char	depend[128];

	unsigned char needfb;
	unsigned char needrc;
	unsigned char needlcd;
*/

struct SPluginInfo
{
	int		pluginversion;
	char	name[20];
	char	desc[100];
	int		type;
	char	depend[128];

	unsigned char needfb;
	unsigned char needrc;
	unsigned char needlcd;
};

//typedef int	(*PluginInfoProc)( struct SPluginInfo *info );
typedef int	(*PluginExecProc)( int fd_fb, int fd_rc, int fd_lcd );

#endif


#ifndef TPLUGIN_H
#define TPLUGIN_H

typedef struct _PluginParam
{
	char				*id;
	char				*val;
	struct _PluginParam	*next;

} PluginParam;

typedef int	(*PluginExec)( PluginParam *par );
/* das dlsym kann auf PluginExec gecastet werden */

/* NOTE : alle Plugins haben uebergangs-weise neue und alte schnittstelle */
/* neues Symbol : plugin_exec */
/* es muessen nur benutzte ids gesetzt werden : nicht genannt = nicht benutzt */

/* fixed ID definitions */
#define	P_ID_FBUFFER	"fd_framebuffer"
#define	P_ID_RCINPUT	"fd_rcinput"
#define	P_ID_LCD		"fd_lcd"
#define	P_ID_NOPIG		"no_pig"			// 1: plugin dont show internal pig
#define P_ID_VTXTPID		"pid_vtxt"

#endif

#ifndef __gamelist__
#define __gamelist__

#include "../driver/framebuffer.h"
#include "../driver/fontrenderer.h"
#include "../driver/rcinput.h"
#include "../daemonc/remotecontrol.h"
#include "../helpers/settings.h"
#include "../plugins/gameplugins.h"
#include "menue.h"
#include "color.h"

#include <string>
#include <vector>

using namespace std;

class CGameList : public CMenuTarget
{
    struct game {
        int         number;
		string      filename;
        string      name;
		string		desc;
    };

	unsigned int		liststart;
	unsigned int		listmaxshow;
	unsigned int		selected;
	int					key;
	string				name;
	vector<game*>	gamelist;

	int					fheight; // Fonthoehe Channellist-Inhalt
	int					theight; // Fonthoehe Channellist-Titel

	int 			width;
	int 			height;
	int 			x;
	int 			y;

	void paintItem(int pos);
	void paint();
	void paintHead();

	public:
    CGameList( string Name );
	~CGameList();

	void hide();
	int exec(CMenuTarget* parent, string actionKey);

};


#endif




#ifndef __bouquetlist__
#define __bouquetlist__

#include "driver/framebuffer.h"
#include "driver/fontrenderer.h"
#include "driver/rcinput.h"
#include "daemonc/remotecontrol.h"
#include "helpers/infoviewer.h"
#include "helpers/settings.h"
#include "eventlist.hpp"
#include "menue.h"
#include "color.h"

#include "channellist.h"

#include <string>
#include <vector>

#include "../channels/lastchannel.h"

using namespace std;

typedef enum bouquetSwitchMode
{
	bsmBouquets,	// pressing OK shows list of all Bouquets
	bsmChannels,	// pressing OK shows list of all channels of active bouquets
	bsmAllChannels	// OK shows lsit of all channels
} BouquetSwitchMode;

class CBouquet {
  public:
	int				key;
	string			name;
	CChannelList*	channelList;

	CBouquet(int Key=-1, const std::string& Name="")
	{
		name = Name;
		channelList = new CChannelList(Key, Name);
	}

	~CBouquet() {
		delete channelList;
	}
};


class CBouquetList
{
	unsigned int		selected;
	unsigned int		tuned;
	unsigned int		liststart;
	unsigned int		listmaxshow;
	unsigned int		numwidth;
	int					fheight; // Fonthoehe Bouquetlist-Inhalt
	int					theight; // Fonthoehe Bouquetlist-Titel

	int					key;
	string				name;

	int		width;
	int		height;
	int		x;
	int		y;

	void paintItem(int pos);
	void paint();
	void paintHead();
	void hide();

  public:
	CBouquetList(int Key=-1, const std::string& Name="");
	~CBouquetList();

	vector<CBouquet*>	Bouquets;

	CChannelList* orgChannelList;
	CBouquet* addBouquet(const std::string& name);
	void setName(const std::string& Name);
	int getActiveBouquetNumber();
	void activateBouquet( int id, bool bShowChannelList = false);
	const std::string& getActiveBouquetName();
	int show();
	bool showChannelList( int nBouquet = -1);
	void adjustToChannel( int nChannelNr);
	void exec( bool bShowChannelList);
};


#endif



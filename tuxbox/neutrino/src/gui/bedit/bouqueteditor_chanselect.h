#ifndef __bouqueteditor_chanselect__
#define __bouqueteditor_chanselect__

#include "driver/framebuffer.h"
#include "driver/fontrenderer.h"
#include "menue.h"
#include <zapitclient.h>

#include <string>
#include <vector>

using namespace std;

	class CBEChannelSelectWidget : public CMenuWidget
	{
		unsigned int		selected;

		unsigned int		liststart;
		unsigned int		listmaxshow;
		unsigned int		numwidth;
		int					fheight;
		int					theight;

		int 				ButtonHeight;
		bool				channelsChanged;

		string caption;
		unsigned int bouquet;
		CZapitClient::channelsMode mode;

		int		width;
		int		height;
		int		x;
		int		y;

		void switchChannel();  // select/deselect channel in current bouquet
		bool isChannelInBouquet( int index);

		void paintItem(int pos);
		void paint();
		void paintHead();
		void paintFoot();
		void hide();

	public:
		CBEChannelSelectWidget( string Caption, unsigned int Bouquet, CZapitClient::channelsMode Mode);

		CZapitClient::BouquetChannelList	Channels;        // list of all channels
		CZapitClient::BouquetChannelList	bouquetChannels; // list of chans that are currently selected
		int exec(CMenuTarget* parent, string actionKey);
		bool hasChanged();
	};

#endif


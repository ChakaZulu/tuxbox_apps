#ifndef __bouqueteditor_channels__
#define __bouqueteditor_channels__

#include "driver/framebuffer.h"
#include "driver/fontrenderer.h"
#include "menue.h"
#include <zapitclient.h>

#include <string>
#include <vector>

using namespace std;

	class CBEChannelWidget : public CMenuWidget
	{
		enum state_
		{
			beDefault,
			beMoving
		} state;

		unsigned int		selected;
		unsigned int		origPosition;
		unsigned int		newPosition;

		unsigned int		liststart;
		unsigned int		listmaxshow;
		unsigned int		numwidth;
		int					fheight;
		int					theight;

		int 				ButtonHeight;
		string				caption;
		bool	channelsChanged;

		unsigned int bouquet;

		int		width;
		int		height;
		int		x;
		int		y;

		void paintItem(int pos);
		void paint();
		void paintHead();
		void paintFoot();
		void hide();

		void deleteChannel();
		void addChannel();
		void beginMoveChannel();
		void finishMoveChannel();
		void cancelMoveChannel();
		void internalMoveChannel( unsigned int fromPosition, unsigned int toPosition);

	public:
		CBEChannelWidget( string Caption, unsigned int Bouquet);

		CZapitClient::BouquetChannelList	Channels;
		int exec(CMenuTarget* parent, string actionKey);
		bool hasChanged();
	};

#endif

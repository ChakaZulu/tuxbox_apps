/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __bouqueteditor_chanselect__
#define __bouqueteditor_chanselect__

#include "driver/framebuffer.h"
#include "gui/widget/menue.h"

#include <string>
#include <vector>

using namespace std;


	
class CBEChannelSelectWidget : public CMenuWidget
{

	private:
		CFrameBuffer		*frameBuffer;
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

		void rcDown();

	public:
		CBEChannelSelectWidget( string Caption, unsigned int Bouquet, CZapitClient::channelsMode Mode);

		CZapitClient::BouquetChannelList	Channels;        // list of all channels
		CZapitClient::BouquetChannelList	bouquetChannels; // list of chans that are currently selected
		int exec(CMenuTarget* parent, string actionKey);
		bool hasChanged();

};

#endif


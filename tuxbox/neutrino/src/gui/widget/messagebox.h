#ifndef __messagebox__
#define __messagebox__

#include "driver/framebuffer.h"
#include "driver/fontrenderer.h"
#include "menue.h"

#include <string>
#include <vector>

using namespace std;

	class CMessageBoxNotifier
	{
	  public:
		virtual void onYes( ) = NULL;
		virtual void onNo( ) = NULL;
	};

	class CMessageBox : CMenuWidget
	{
		int						width;
		int						height;
		int						x;
		int						y;
		int						fheight;
		int						theight;
		string					caption;
		vector<string>			text;
		CMessageBoxNotifier*	notifier;
		int						selected;
		int						showbuttons;

		void paintHead();
		void paintButtons();
		void hide();

		void yes();
		void no();
		void cancel();

	public:
		enum result_
		{
			mbrYes,
			mbrNo,
			mbrCancel,
			mbrBack
		} result;

		enum buttons_
		{
			mbYes= 0x01,
			mbNo = 0x02,
			mbCancel = 0x04,
			mbAll = 0x07,
			mbBack = 0x08
		} buttons;

		CMessageBox( string Caption, string Text, CMessageBoxNotifier* Notifier, int Width = 500, uint Default= mbrYes, uint ShowButtons= mbAll );
		int exec(CMenuTarget* parent, string actionKey);

	};

int ShowMsg ( string Caption, string Text, uint Default, uint ShowButtons );

#endif

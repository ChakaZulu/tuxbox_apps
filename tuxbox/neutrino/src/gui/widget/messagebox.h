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
		string					text;
		CMessageBoxNotifier*	notifier;

		void paint();
		void hide();

		void yes();
		void no();
		void cancel();

	public:
		CMessageBox( string Caption, string Text, CMessageBoxNotifier* Notifier);
		int exec(CMenuTarget* parent, string actionKey);

		enum result_
		{
			mbrYes,
			mbrNo,
			mbrCancel
		} result;
	};

#endif

#ifndef __setup_audio_h
#define __setup_audio_h

#include <lib/gui/ewindow.h>
#include <lib/gui/statusbar.h>

class eButton;
class eCheckbox;

class eZapAudioSetup: public eWindow
{
	eButton *ok, *abort;
	eCheckbox *ac3default;
	eStatusBar *statusbar;
private:
	void ac3defaultChanged( int i );
	void okPressed();
	void abortPressed();
public:
	eZapAudioSetup();
	~eZapAudioSetup();
};

#endif

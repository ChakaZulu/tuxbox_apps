#include <core/gui/emessage.h>

class enigmaVCR : public eMessageBox
{
	public:
		enigmaVCR(eString string, eString caption);
		int eventHandler(const eWidgetEvent &event);
		void volumeUp();
		void volumeDown();
};

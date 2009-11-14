#ifdef ENABLE_KEYBOARD

#include <setup_keyboard.h>

#include <lib/gui/ebutton.h>
#include <lib/gui/combobox.h>
#include <lib/gui/enumber.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/eskin.h>
#include <lib/gui/actions.h>
#include <lib/gui/emessage.h>
#include <lib/gui/statusbar.h>
#include <lib/driver/rc.h>
#include <lib/system/econfig.h>
#include <lib/base/i18n.h>
#include <dirent.h>


class eListBoxEntryKeyboardMapping: public eListBoxEntryText
{
	eString mapping_file;
public:
	eListBoxEntryKeyboardMapping( eListBox<eListBoxEntryText > *lb, eString &text, eString &mapping_file )
		:eListBoxEntryText(lb, text), mapping_file(mapping_file)
	{
	}
	const char *getMappingFile() { return mapping_file.c_str(); }
};

eZapKeyboardSetup::eZapKeyboardSetup()
	:eWindow()
{
	init_eZapKeyboardSetup();
}
void eZapKeyboardSetup::init_eZapKeyboardSetup()
{
	mappings=CreateSkinnedComboBox("mapping",4);

	CONNECT(CreateSkinnedButton("save")->selected, eZapKeyboardSetup::okPressed);

	BuildSkin("KeyboardSetup");

	loadMappings();
}

eZapKeyboardSetup::~eZapKeyboardSetup()
{
}

void eZapKeyboardSetup::okPressed()
{
	// save current selected keyboard mapping
	if ( mappings->getCurrent() )
		eConfig::getInstance()->setKey("/ezap/keyboard/mapping", ((eListBoxEntryKeyboardMapping*)mappings->getCurrent())->getMappingFile() );

	eRCInput::getInstance()->loadKeyboardMapping();

	close(1);
}

void eZapKeyboardSetup::loadMappings()
{
	eListBoxEntryText* selection=0;

	const char *mapPaths[] =
	{
		"/var/keymaps/",
		DATADIR "/keymaps/",
		0
	};

	char *current_map=0;
	if ( eConfig::getInstance()->getKey("/ezap/keyboard/mapping", current_map) )
		current_map = strdup(DATADIR "/keymaps/eng.kmap");

	std::set<eString> parsedMaps;

	for (int i=0; mapPaths[i]; ++i)
	{
		DIR *d=opendir(mapPaths[i]);
		if (!d)
		{
			if (i)
			{
				eDebug("error reading keyboard mapping directory");
				eMessageBox::ShowBox("error reading keyboard mapping directory", "error");
			}
			continue;
		}

		while(struct dirent *e=readdir(d))
		{
			if (i && parsedMaps.find(e->d_name) != parsedMaps.end() )
				// ignore loaded skins in var... (jffs2)
				continue;

			eString fileName=mapPaths[i];
			fileName+=e->d_name;
			if (fileName.find(".info") != eString::npos)
			{
				eSimpleConfigFile config(fileName.c_str());
				eString kmap = mapPaths[i] + config.getInfo("kmap");
				eString name = config.getInfo("name");
				eDebug("kmap = %s, name = %s", kmap.c_str(), name.c_str());
				if (kmap.size() && name.size())
				{
					parsedMaps.insert(e->d_name);
					eListBoxEntryText *s=new eListBoxEntryKeyboardMapping(*mappings, name, kmap);
					if (current_map && kmap == current_map)
						selection=s;
				}
			}
		}
		closedir(d);
	}
	if ( mappings->getCount() )
		mappings->sort();
	if (selection)
		mappings->setCurrent(selection);
	if ( current_map )
		free(current_map);
}

#endif //ENABLE_KEYBOARD

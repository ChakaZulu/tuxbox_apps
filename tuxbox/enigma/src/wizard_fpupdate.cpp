#include <wizard_fpupdate.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/system/dmfp.h>
#include <lib/gui/emessage.h>

int eWizardFPUpdate::run()
{
	if (eDreamboxFP::isUpgradeAvailable())
	{
		{
			eMessageBox msg(
				_("New Software for the frontprocessor is avail.\n"
					"Your Dreambox must upgrade now!\n"
					"DO NOT TURN OFF THE POWER DURING UPDATE !!!."),
				_("New software avail"),
				eMessageBox::btOK|eMessageBox::iconInfo);
			msg.show();
			msg.exec();
			msg.hide();
		}
		{
			eMessageBox msg(
				_("Upgrading frontprocessor...\n"
					"DO NOT TURN OFF THE POWER!\n"
					"THE DREAMBOX WILL AUTOMATICALLY REBOOT!\n"),
				_("Upgrade in process..."), eMessageBox::iconWarning);
			msg.show();
			/* in this case, this is really required - doUpgrade does CLI */
			sleep(1);
			eDreamboxFP::doUpgrade(0xd15ea5e);
		}
	}
	return 0;
}

eAutoInitP0<eWizardFPUpdate> init_eWizardFPUpdate(eAutoInitNumbers::wizard+1, "wizard: fpudate");

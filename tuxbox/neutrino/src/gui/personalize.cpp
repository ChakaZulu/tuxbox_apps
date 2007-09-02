/*
        $Id: personalize.cpp,v 1.3 2007/09/02 16:52:16 dbt Exp $

        Customization Menu - Neutrino-GUI

        Copyright (C) 2007 Speed2206
        and some other guys

        Kommentar:

        This is the customization menu, as originally showcased in
        Oxygen. It is a more advanced version of the 'user levels'
        patch currently available.


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
        Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <global.h>
#include <neutrino.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/screen_max.h>
#include <daemonc/remotecontrol.h>
#include "widget/menue.h"
#include "widget/messagebox.h"
#include "widget/hintbox.h"
#include "widget/lcdcontroler.h"
#include "widget/keychooser.h"
#include "widget/stringinput.h"
#include "widget/stringinput_ext.h"
#include "color.h"
#include "personalize.h"

#define PERSONALIZE_STD_OPTION_COUNT 3
#define PERSONALIZE_EDP_OPTION_COUNT 3
#define PERSONALIZE_EOD_OPTION_COUNT 2
#define PERSONALIZE_YON_OPTION_COUNT 2

const CMenuOptionChooser::keyval PERSONALIZE_STD_OPTIONS[PERSONALIZE_STD_OPTION_COUNT] =
{
{ 0, LOCALE_PERSONALIZE_NOTVISIBLE      },                                      // The option is NOT visible on the menu's
{ 1, LOCALE_PERSONALIZE_VISIBLE         },                                      // The option is visible on the menu's
{ 2, LOCALE_PERSONALIZE_PIN      },                                      // PIN Protect the item on the menu
};
const CMenuOptionChooser::keyval PERSONALIZE_EDP_OPTIONS[PERSONALIZE_EDP_OPTION_COUNT] =
{
{ 0, LOCALE_PERSONALIZE_DISABLED        },                                      // The menu is NOT enabled / accessible
{ 1, LOCALE_PERSONALIZE_ENABLED         },                                      // The menu is enabled / accessible
{ 2, LOCALE_PERSONALIZE_PIN      },                                      // The menu is enabled and protected with PIN
};
const CMenuOptionChooser::keyval PERSONALIZE_EOD_OPTIONS[PERSONALIZE_EOD_OPTION_COUNT] =
{
{ 0, LOCALE_PERSONALIZE_DISABLED        },                                      // The option is NOT enabled / accessible
{ 1, LOCALE_PERSONALIZE_ENABLED         },                                      // The option is enabled / accessible
};
const CMenuOptionChooser::keyval PERSONALIZE_YON_OPTIONS[PERSONALIZE_YON_OPTION_COUNT] =
{
{ 0, LOCALE_PERSONALIZE_NOTPROTECTED    },                                      // The menu/option is NOT protected
{ 1, LOCALE_PERSONALIZE_PINPROTECT      },                                      // The menu/option is protected by a PIN
};



CPersonalizeGui::CPersonalizeGui()
: configfile('\t')
{
		frameBuffer = CFrameBuffer::getInstance();
		width = w_max (710, 100);
		hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
		mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
		height = hheight+13*mheight+ 10;
		x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
		y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;

}

int CPersonalizeGui::exec(CMenuTarget* parent, const std::string & actionKey)
{
        int res = menu_return::RETURN_REPAINT;

        if(actionKey=="mainmenu_options") {                                     // Personalize the Main Menu
                ShowMainMenuOptions();
                return res; }

        if(actionKey=="settings_options") {                                     // Personalize the Settings Menu
                ShowSettingsOptions();
                return res; }

        if (actionKey=="service_options") {                                     // Personalize the Service Menu
                ShowServiceOptions();
                return res; }

        if (parent)             {               parent->hide();         }
        ShowPersonalizationMenu();                                              // Show main Personalization Menu
        return res;
}

void CPersonalizeGui::hide()
{
        frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void CPersonalizeGui::ShowPersonalizationMenu()
{
                        /*      This is the main personalization menu. From here we can go to the other sub-menu's and enable/disable
                                the PIN code feature, as well as determine whether or not the EPG menu/Features menu is accessible. */

                        CMenuWidget* pMenu = new CMenuWidget(LOCALE_PERSONALIZE_HEAD,NEUTRINO_ICON_PROTECTING, width);
                        CPINChangeWidget * pinChangeWidget = new CPINChangeWidget(LOCALE_PERSONALIZE_PINCODE, g_settings.personalize_pincode, 4, LOCALE_PERSONALIZE_PINHINT);

                        pMenu->addItem(GenericMenuSeparator);
                        pMenu->addItem(GenericMenuBack);
						pMenu->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_PERSONALIZE_ACCESS));

                        pMenu->addItem(new CMenuOptionChooser(LOCALE_PERSONALIZE_PINSTATUS, (int *)&g_settings.personalize_pinstatus, PERSONALIZE_YON_OPTIONS, PERSONALIZE_YON_OPTION_COUNT, true, NULL, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
                        pMenu->addItem(new CMenuForwarder(LOCALE_PERSONALIZE_PINCODE, true, g_settings.personalize_pincode, pinChangeWidget, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
						pMenu->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, 	LOCALE_PERSONALIZE_MENUCONFIGURATION));


                        pMenu->addItem(new CMenuForwarder(LOCALE_MAINMENU_HEAD, true, NULL, this, "mainmenu_options", CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));
                        pMenu->addItem(new CMenuForwarder(LOCALE_MAINMENU_SETTINGS, true, NULL, this, "settings_options", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
                        pMenu->addItem(new CMenuForwarder(LOCALE_MAINMENU_SERVICE, true, NULL, this, "service_options", CRCInput::RC_1));

                        pMenu->addItem(GenericMenuSeparatorLine);
                        pMenu->addItem(new CMenuOptionChooser(LOCALE_INFOVIEWER_STREAMINFO, (int *)&g_settings.personalize_bluebutton, PERSONALIZE_EOD_OPTIONS, PERSONALIZE_EOD_OPTION_COUNT, true, NULL, CRCInput::RC_2));
                        pMenu->addItem(new CMenuOptionChooser(LOCALE_INFOVIEWER_EVENTLIST, (int *)&g_settings.personalize_redbutton, PERSONALIZE_EOD_OPTIONS, PERSONALIZE_EOD_OPTION_COUNT, true, NULL, CRCInput::RC_3));

                        pMenu->exec (NULL, "");
                        pMenu->hide ();
                        delete pMenu;
}

void CPersonalizeGui::ShowMainMenuOptions()
{
                        /*      Here we give the user the option to enable, disable, or PIN protect items on the Main Menu.
                                We also provide a means of PIN protecting the menu itself. */

                        int old_tvmode                          = g_settings.personalize_tvmode;
                        int old_radiomode                       = g_settings.personalize_radiomode;
                        int old_scartmode                       = g_settings.personalize_scartmode;
                        int old_games                           = g_settings.personalize_games;
                        int old_audioplayer                     = g_settings.personalize_audioplayer;
                        int old_movieplayer                     = g_settings.personalize_movieplayer;
                        int old_pictureviewer                   = g_settings.personalize_pictureviewer;	
#ifdef ENABLE_UPNP
						int old_upnpbrowser                     = g_settings.personalize_upnpbrowser;
#endif
                        int old_settings                        = g_settings.personalize_settings;
                        int old_service                         = g_settings.personalize_service;
                        int old_sleeptimer                      = g_settings.personalize_sleeptimer;
                        int old_reboot                          = g_settings.personalize_reboot;
                        int old_shutdown                        = g_settings.personalize_shutdown;

                        CMenuWidget* pMMMenu = new CMenuWidget(LOCALE_MAINMENU_HEAD,NEUTRINO_ICON_PROTECTING, width);

                        pMMMenu->addItem(GenericMenuSeparator);
                        pMMMenu->addItem(GenericMenuBack);
						pMMMenu->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_PERSONALIZE_ACCESS));

                        pMMMenu->addItem(new CMenuOptionChooser(LOCALE_MAINMENU_TVMODE, (int *)&g_settings.personalize_tvmode, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
                        pMMMenu->addItem(new CMenuOptionChooser(LOCALE_MAINMENU_RADIOMODE, (int *)&g_settings.personalize_radiomode, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
                        pMMMenu->addItem(new CMenuOptionChooser(LOCALE_MAINMENU_SCARTMODE, (int *)&g_settings.personalize_scartmode, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));
                        pMMMenu->addItem(new CMenuOptionChooser(LOCALE_MAINMENU_GAMES, (int *)&g_settings.personalize_games, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
                        pMMMenu->addItem(GenericMenuSeparator);
                        pMMMenu->addItem(new CMenuOptionChooser(LOCALE_MAINMENU_AUDIOPLAYER, (int *)&g_settings.personalize_audioplayer,PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_1));
                        pMMMenu->addItem(new CMenuOptionChooser(LOCALE_MAINMENU_MOVIEPLAYER, (int *)&g_settings.personalize_movieplayer, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_2));
                        pMMMenu->addItem(new CMenuOptionChooser(LOCALE_MAINMENU_PICTUREVIEWER, (int *)&g_settings.personalize_pictureviewer, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_3));
#ifdef ENABLE_UPNP
                        pMMMenu->addItem(new CMenuOptionChooser(LOCALE_MAINMENU_UPNPBROWSER, (int *)&g_settings.personalize_upnpbrowser, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_4));
#endif
						pMMMenu->addItem(GenericMenuSeparatorLine);
                        pMMMenu->addItem(new CMenuOptionChooser(LOCALE_MAINMENU_SLEEPTIMER, (int *)&g_settings.personalize_sleeptimer, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_5));
                        pMMMenu->addItem(new CMenuOptionChooser(LOCALE_MAINMENU_REBOOT, (int *)&g_settings.personalize_reboot, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_6));
                        pMMMenu->addItem(new CMenuOptionChooser(LOCALE_MAINMENU_SHUTDOWN, (int *)&g_settings.personalize_shutdown, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_standby, NEUTRINO_ICON_BUTTON_POWER));

  						pMMMenu->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_PERSONALIZE_STPROTECT));
                        pMMMenu->addItem(new CMenuOptionChooser(LOCALE_MAINMENU_SETTINGS, (int *)&g_settings.personalize_settings, PERSONALIZE_YON_OPTIONS, PERSONALIZE_YON_OPTION_COUNT, true, NULL, CRCInput::RC_6));
                        pMMMenu->addItem(new CMenuOptionChooser(LOCALE_MAINMENU_SERVICE, (int *)&g_settings.personalize_service, PERSONALIZE_YON_OPTIONS, PERSONALIZE_YON_OPTION_COUNT, true, NULL, CRCInput::RC_7));
                        pMMMenu->addItem(GenericMenuSeparator);
						
                        pMMMenu->exec (NULL, "");
                        pMMMenu->hide ();
                        delete pMMMenu;

                        // Check for changes
                        if (old_tvmode != g_settings.personalize_tvmode || old_radiomode != g_settings.personalize_radiomode
                                                                                                                        || old_scartmode != g_settings.personalize_scartmode
                                                                                                                        || old_games != g_settings.personalize_games
                                                                                                                        || old_audioplayer != g_settings.personalize_audioplayer
                                                                                                                        || old_movieplayer != g_settings.personalize_movieplayer
                                                                                                                        || old_pictureviewer != g_settings.personalize_pictureviewer
																						#ifdef ENABLE_UPNP
																														|| old_upnpbrowser != g_settings.personalize_upnpbrowser
																											#endif
                                                                                                                        || old_settings != g_settings.personalize_settings
                                                                                                                        || old_service != g_settings.personalize_service
                                                                                                                        || old_sleeptimer != g_settings.personalize_sleeptimer
                                                                                                                        || old_reboot != g_settings.personalize_reboot
                                                                                                                        || old_shutdown != g_settings.personalize_shutdown) {

                        if (ShowMsgUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_PERSONALIZE_SAVERESTART), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_PROTECTING) == CMessageBox::mbrYes)
                                { SaveAndRestart(); }
                        }

}

void CPersonalizeGui::ShowSettingsOptions()
{
                        /*      Here we give the user the option to enable, disable, or PIN protect items on the Settings Menu.
                                We also provide a means of PIN protecting the menu itself. */

                        int old_stprotect                               = g_settings.personalize_settings;
                        int old_video                                   = g_settings.personalize_video;
                        int old_audio                                   = g_settings.personalize_audio;
                        int old_youth                                   = g_settings.personalize_youth;
                        int old_network                                 = g_settings.personalize_network;
                        int old_recording                               = g_settings.personalize_recording;
                        int old_streaming                               = g_settings.personalize_streaming;
                        int old_language                                = g_settings.personalize_language;
                        int old_colors                                  = g_settings.personalize_colors;
                        int old_lcd                                             = g_settings.personalize_lcd;
                        int old_keybinding                              = g_settings.personalize_keybinding;
                        int old_audpic                                  = g_settings.personalize_audpic;
                        int old_driver                                  = g_settings.personalize_driver;
                        int old_misc                                    = g_settings.personalize_misc;

                        CMenuWidget* pSTMenu = new CMenuWidget(LOCALE_MAINMENU_SETTINGS,NEUTRINO_ICON_PROTECTING, width);

                        pSTMenu->addItem(GenericMenuSeparator);
                        pSTMenu->addItem(GenericMenuBack);
                        pSTMenu->addItem(GenericMenuSeparatorLine);

                        pSTMenu->addItem(new CMenuOptionChooser(LOCALE_PERSONALIZE_SETUPMENUWITHPIN, (int *)&g_settings.personalize_settings, PERSONALIZE_YON_OPTIONS, PERSONALIZE_YON_OPTION_COUNT, true, NULL));
						pSTMenu->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_PERSONALIZE_ACCESS));

                        pSTMenu->addItem(new CMenuOptionChooser(LOCALE_MAINSETTINGS_VIDEO, (int *)&g_settings.personalize_video, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_1));
                        pSTMenu->addItem(new CMenuOptionChooser(LOCALE_MAINSETTINGS_AUDIO, (int *)&g_settings.personalize_audio, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_2));
                        pSTMenu->addItem(new CMenuOptionChooser(LOCALE_PARENTALLOCK_PARENTALLOCK, (int *)&g_settings.personalize_youth, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_3));
                        pSTMenu->addItem(new CMenuOptionChooser(LOCALE_MAINSETTINGS_NETWORK, (int *)&g_settings.personalize_network, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_4));
                        pSTMenu->addItem(new CMenuOptionChooser(LOCALE_MAINSETTINGS_RECORDING, (int *)&g_settings.personalize_recording, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_5));
                        pSTMenu->addItem(new CMenuOptionChooser(LOCALE_MAINSETTINGS_STREAMING, (int *)&g_settings.personalize_streaming, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_6));
                        pSTMenu->addItem(new CMenuOptionChooser(LOCALE_MAINSETTINGS_LANGUAGE, (int *)&g_settings.personalize_language, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_7));
                        pSTMenu->addItem(new CMenuOptionChooser(LOCALE_MAINSETTINGS_COLORS, (int *)&g_settings.personalize_colors, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_8));
                        pSTMenu->addItem(new CMenuOptionChooser(LOCALE_MAINSETTINGS_LCD, (int *)&g_settings.personalize_lcd, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_9));
                        pSTMenu->addItem(new CMenuOptionChooser(LOCALE_MAINSETTINGS_KEYBINDING, (int *)&g_settings.personalize_keybinding, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_0));
                        pSTMenu->addItem(new CMenuOptionChooser(LOCALE_AUDIOPLAYERPICSETTINGS_GENERAL, (int *)&g_settings.personalize_audpic, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
                        pSTMenu->addItem(new CMenuOptionChooser(LOCALE_MAINSETTINGS_DRIVER, (int *)&g_settings.personalize_driver, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
                        pSTMenu->addItem(new CMenuOptionChooser(LOCALE_MAINSETTINGS_MISC, (int *)&g_settings.personalize_misc, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));

                        pSTMenu->exec (NULL, "");
                        pSTMenu->hide ();
                        delete pSTMenu;

                        // Check for changes
                        if (old_stprotect != g_settings.personalize_settings
                                                                                                                                || old_video != g_settings.personalize_video
                                                                                                                                || old_audio != g_settings.personalize_audio
                                                                                                                                || old_youth != g_settings.personalize_youth
                                                                                                                                || old_network != g_settings.personalize_network
                                                                                                                                || old_recording != g_settings.personalize_recording
                                                                                                                                || old_streaming != g_settings.personalize_streaming
                                                                                                                                || old_language != g_settings.personalize_language
                                                                                                                                || old_colors != g_settings.personalize_colors
                                                                                                                                || old_lcd != g_settings.personalize_lcd
                                                                                                                                || old_keybinding != g_settings.personalize_keybinding
                                                                                                                                || old_audpic != g_settings.personalize_audpic
                                                                                                                                || old_driver != g_settings.personalize_driver
                                                                                                                                || old_misc != g_settings.personalize_misc) {

                        if (ShowMsgUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_PERSONALIZE_SAVERESTART), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_PROTECTING) == CMessageBox::mbrYes)
                                { SaveAndRestart(); }
                        }
}

void CPersonalizeGui::ShowServiceOptions()
{
                        /*      Here we give the user the option to enable, disable, or PIN protect items on the Service Menu.
                                We also provide a means of PIN protecting the menu itself. */

                        int old_svprotect                               = g_settings.personalize_service;
                        int old_bouqueteditor                   = g_settings.personalize_bouqueteditor;
                        int old_scants                                  = g_settings.personalize_scants;
                        int old_reload                                  = g_settings.personalize_reload;
                        int old_getplugins                              = g_settings.personalize_getplugins;
                        int old_restart                                 = g_settings.personalize_restart;
                        int old_ucodecheck                              = g_settings.personalize_ucodecheck;
                        int old_imageinfo                               = g_settings.personalize_imageinfo;
                        int old_update                                  = g_settings.personalize_update;

                        CMenuWidget* pSMMenu = new CMenuWidget(LOCALE_MAINMENU_SERVICE,NEUTRINO_ICON_PROTECTING, width);

                        pSMMenu->addItem(GenericMenuSeparator);
                        pSMMenu->addItem(GenericMenuBack);
                        pSMMenu->addItem(GenericMenuSeparatorLine);

                        pSMMenu->addItem(new CMenuOptionChooser(LOCALE_PERSONALIZE_SVPROTECT, (int *)&g_settings.personalize_service, PERSONALIZE_YON_OPTIONS, PERSONALIZE_YON_OPTION_COUNT, true, NULL));
						pSMMenu->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_PERSONALIZE_ACCESS));

                        pSMMenu->addItem(new CMenuOptionChooser(LOCALE_BOUQUETEDITOR_NAME, (int *)&g_settings.personalize_bouqueteditor, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
                        pSMMenu->addItem(new CMenuOptionChooser(LOCALE_SERVICEMENU_SCANTS, (int *)&g_settings.personalize_scants, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
                        pSMMenu->addItem(GenericMenuSeparatorLine);
                        pSMMenu->addItem(new CMenuOptionChooser(LOCALE_SERVICEMENU_RELOAD, (int *)&g_settings.personalize_reload, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_1));
                        pSMMenu->addItem(new CMenuOptionChooser(LOCALE_SERVICEMENU_GETPLUGINS, (int *)&g_settings.personalize_getplugins, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_2));
                        pSMMenu->addItem(new CMenuOptionChooser(LOCALE_SERVICEMENU_RESTART, (int *)&g_settings.personalize_restart, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_3));
                        pSMMenu->addItem(new CMenuOptionChooser(LOCALE_SERVICEMENU_UCODECHECK, (int *)&g_settings.personalize_ucodecheck, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_4));

                        pSMMenu->addItem(GenericMenuSeparatorLine);
                        pSMMenu->addItem(new CMenuOptionChooser(LOCALE_SERVICEMENU_IMAGEINFO, (int *)&g_settings.personalize_imageinfo, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));
                        pSMMenu->addItem(new CMenuOptionChooser(LOCALE_SERVICEMENU_UPDATE, (int *)&g_settings.personalize_update, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, true, NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));

                        pSMMenu->exec (NULL, "");
                        pSMMenu->hide ();
                        delete pSMMenu;

                        // Check for changes
                        if (old_svprotect != g_settings.personalize_service
                                                                                                                        || old_bouqueteditor != g_settings.personalize_bouqueteditor
                                                                                                                        || old_scants != g_settings.personalize_scants
                                                                                                                        || old_reload != g_settings.personalize_reload
                                                                                                                        || old_getplugins != g_settings.personalize_getplugins
                                                                                                                        || old_restart != g_settings.personalize_restart
                                                                                                                        || old_ucodecheck != g_settings.personalize_ucodecheck
                                                                                                                        || old_imageinfo != g_settings.personalize_imageinfo
                                                                                                                        || old_update != g_settings.personalize_update) {

                        if (ShowMsgUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_PERSONALIZE_SAVERESTART), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_PROTECTING) == CMessageBox::mbrYes)
                                { SaveAndRestart(); }
                        }

}

void CPersonalizeGui::SaveAndRestart()
{
                // Save the settings and restart Neutrino, if user wants to!
                CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_MAINSETTINGS_SAVESETTINGSNOW_HINT)); // UTF-8
                hintBox->paint();
                CNeutrinoApp::getInstance()->saveSetup();
                hintBox->hide();
                delete hintBox;

                CNeutrinoApp::getInstance()->exec(NULL, "restart");
}

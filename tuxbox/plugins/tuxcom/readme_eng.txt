TuxCom:

History:
---------

11.08.2004 Version 1.4a
 - support of usb-keyboards (needs kernel-module hid.ko from BoxMan)
 - read .ftp-files even when created by windows
 - BugFix: inserting new line in empty file in editor
 - minor bugfixes in Editor
 - many bugfixes in ftp-client
 - changes in keyboard routine
 - BugFix: wrong display after pressing red button (clear) while editing
 - BugFix: crash when leaving plugin with open ftp-connection

25.07.2004 Version 1.4
 - Taskmanager added (on Info-Button)
 - scrolling back/forward possible when executing commands or scripts
 - scrolling back/forward in viewer not limited to 100k anymore
 - remember current selected file on plugin exit
 - Support for DMM-Keyboard installed
 - delay for pressed button 
 - Bugfix: workaround for button-press bug from enigma
 - create link (Button 0): display current filename as name.
 
21.06.2004 Version 1.3
 - FTP-Client added
 - minor bugfixes in editor
 - text input: jumping to next character when pressing another Number.
 - text input: removing last character when at end of line and pressing volume-
 - toggle between 4:3 and 16:9-mode with dream-button
 - Viewer:scrolling possible as in editor (for files up to 100k size)

05.06.2004 Version 1.2a
 - BugFix: missing characters in text input added.
 - text input in "sms-style" included

29.05.2004 Version 1.2
 - support for reading and extracting from "tar", "tar.Z", "tar.gz" and "tar.bz2" archives
   does not work with many Archives in Original-Image 1..07.4 ( BusyBox-Version to old :( )
 - display current line in editor
 - using tuxtxt-position for display
 - big font when editing a line
 - change scrolling through characters in edit mode to match enigma standard (switch up/down)
 - Version of plugin available on Info-Button
 - confirm-messagebox when overwriting existing files.

08.05.2004 Version 1.1a
 - BugFix: No more spaces at the end of renamed files

02.05.2004 version 1.1
 - changed some colors
 - added german language
 - possibility to keep buttons pressed (up/down, left/right, volume+/-, green button)
 - 3 states of transparency
 - set markers on files -> possibility to copy/move/delete multiple files
 - Key for transparency now mute (green button needed for setting file marker)
  
03.04.2004 version 1.0 : 
   first public version


Sources:
--------
I took code partially from T-Hydron's script-plugin (http://t-hydron.verkoyen.be/)
and LazyT's TuxTxt (from the CDK) 

Requirements:
-------------
 - A Dreambox 7000-S ( not tested on other Models )
 - Firmware Version 1.07.x or higher ( not tested on older Versions )

Installation:
-------------
As any plugin, just copy tuxcom.so and tuxcom.cfg to /var/tuxbox/plugins

if the font-file 'pakenham.ttf' is not in /share/fonts/
please copy it to /var/tuxbox/config/enigma/fonts/




Keys:
---------------

left/right		choose left/right window
up/down			select prev/next entry in current window
volume -/+		one page up/down in current window
ok			execute selected file / change dir in current window / open archive for reading
1			view/edit properties (rights) of selected file 
2			rename selected file 
3			view selected file 
4			edit selected file 
5			copy selected file from current window to other window
6			move selected file from current window to other window
7			create new directory in current window
8			delete selected file 
9			create new file in current window
0			create symbolic link to selected file in current window in directory of other window	
red			enter linux command
green			toggle file marker
yellow			toggle sorting of entries in current window
blue			refresh display
mute			toggle transparency
dream			toggle between 4:3 and 16:9-mode
info			Taskmanager / version info

in messageboxes:

left/right		change selection
ok			confirm selection
red/green/yellow	change selection

in textinput:

left/right		change selected stringposition
up/down			change character
ok			confirm changes
volume +		insert new character
volume -		remove character
red			clear input
yellow			change between uppercase/lowercase
0..9			select character in "sms-style" (as in enigma textinput)

in properties:

up/down			change selection
ok			toggle right 
red			confirm selection
green			cancel selection

in Editor:

left/right		Page back/forward
up/down 		Line up/down
ok			edit line
volume +		jump to first line
volume -		jump to last line
red			delete line
green			insert line

in Viewer:

ok/right		view next page
left/right		Page back/forward
up/down 		Line up/down
volume +		jump to first line
volume -		jump to last line

in Taskmanager:

ok/right		view next page
left/right		Page back/forward
up/down 		Line up/down
volume +		jump to first line
volume -		jump to last line
red			kill process

in all dialogs: 

lame			exit dialog



colors:
------------
background: 
black : directory is readonly
blue  : directory is read/write

filename:
white : entry is directory
orange: entry is link
yellow: entry is executable
gray  : entry is writable
green : entry is readable


Using the FTP-Client:
-----------------------
1.) create file with ending .ftp
2.) edit this file:
possible entries:
host=<ftp-adress>	(required, e.g.: host=ftp.gnu.org)
user=<username> 	(optional)
pass=<password> 	(optional)
port=<ftpport>  	(optional, default 21)
dir=<directory>		(optional, default /)
3.) select file and press OK . 
you are connected to the specified adress.

buttons for usb-keyboard:
-------------------------
lame		Esc
volume+/-	PgUp/PgDn
OK		Enter
red		F5
green		F6
yellow		F7
blue		F8
dream		F9
info		F10
mute		F11

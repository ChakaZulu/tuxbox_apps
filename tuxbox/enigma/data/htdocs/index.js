function NewWindow(mypage, myname, w, h, scroll, timeout)
{
	var winl = (screen.width - w) / 2;
	var wint = (screen.height - h) / 2;
	winprops = 'height='+h+', width='+w+', top='+wint+', left='+winl+', scrollbars='+scroll+', resizable'
	win = window.open(mypage, myname, winprops)
	if (parseInt(navigator.appVersion) >= 4) { win.window.focus(); }
	if (timeout > 0) { win.window.setTimeout("close()", timeout); }
}

function reload()
{
	document.location.reload();
}

function setVol(xy)
{
	win=window.open("/setVolume?volume="+xy, "switchStatus","width=50,height=20,left=0,top=0");
	win.focus();
	parent.setTimeout("reload()", 100);
}

function setVid(xy)
{
	win=window.open("/setVideo?position="+xy, "switchStatus", "width=50,height=20,left=0,top=0");
	win.focus();
	parent.setTimeout("reload()", 100);
}	

function Mute(xy)
{
	win=window.open("/setVolume?mute="+xy, "switchStatus","width=1,height=1,left=0,top=0");
	win.focus();
	parent.setTimeout("reload()", 100);
}

function switchChannel(xy)
{
	win=window.open("?path="+xy, "switchStatus","width=1,height=1,left=0,top=0");
	win.focus();
	parent.setTimeout("reload()", 3000);
}

function deleteMovie(xy)
{
	win=window.open("/cgi-bin/deleteMovie?ref="+xy, "switchStatus","width=1,height=1,left=0,top=0");
	parent.setTimeout("reload()", 3000);
}

function openEPG(xy)
{
	NewWindow('/getcurrentepg2?ref='+xy, 'EPG', '600', '700', 'yes');
}

function selectAudio()
{
	NewWindow('/cgi-bin/selectAudio', 'AudioSelect', '200', '100', 'no');
}

function openMultiEPG(xy)
{
	NewWindow('/getMultiEPG?ref='+xy, 'MultiEPG', screen.width, screen.height, 'yes');
}

function admin(xy)
{
	NewWindow(xy+'&requester=webif', 'admin', '200', '100', 'no', '3000');
}

function vcontrol(xy)
{
	win=window.open("/cgi-bin/videocontrol?command="+xy, "vcontrol", "width=50, height=20, left=0, top=0");
	win.focus();
}

function openSI()
{
	NewWindow("/cgi-bin/streaminfo?requester=webif", "si", "300", "250", "no");
}

function cleanupTimerList()
{
	win=window.open("/cleanupTimerList", "switchStatus", "width=1, height=1, left=0, top=0");
	win.focus();
	parent.setTimeout("reload()", 500);
}

function clearTimerList()
{
	win=window.open("/clearTimerList", "switchStatus", "width=1, height=1, left=0, top=0");
	win.focus();
	parent.setTimeout("reload()", 500);
}

function DVRrecord(xy)
{
	NewWindow("/cgi-bin/record?command="+xy, "record", "200", "100", "no");
}

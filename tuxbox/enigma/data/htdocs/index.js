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

function Mute()
{
	win=window.open("/setVolume?mute=1", "switchStatus","width=1,height=1,left=0,top=0");
	win.focus();
	parent.setTimeout("reload()", 100);
}

function unMute()
{
	win=window.open("/setVolume?mute=0", "switchStatus","width=1,height=1,left=0,top=0");
	win.focus();
	parent.setTimeout("reload()", 100);
}

function switchChannel(xy)
{
	win=window.open("?path="+xy, "switchStatus","width=1,height=1,left=0,top=0");
	win.focus();
	parent.setTimeout("reload()", 3000);
}

function openEPG(xy)
{
	NewWindow('/getcurrentepg2?ref='+xy, 'EPG', '600', '700', 'yes');
}

function openMultiEPG(xy)
{
	NewWindow('/getMultiEPG?ref='+xy, 'MultiEPG', screen.width, screen.height, 'yes');
}

function admin(xy)
{
	NewWindow(xy+'&requester=webif', 'admin', '200', '100', 'no', '3000');
}

function openSI()
{
	NewWindow("/cgi-bin/streaminfo?requester=webif", "si", "300", "250", "no");
}

function cleanupTimerList()
{
	win=window.open("/cleanupTimerList", "switchStatus", "width=1, height=1, left=0, top=0");
	win.focus();
}

function clearTimerList()
{
	win=window.open("/clearTimerList", "switchStatus", "width=1, height=1, left=0, top=0");
	win.focus();
}

function DVRrecord(xy)
{
	NewWindow("/cgi-bin/record?command="+xy, "record", "200", "100", "no");
}

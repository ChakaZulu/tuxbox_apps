function setVol(volume)
{
	document.location = "/setVolume?volume=" + volume;
	if (window.screen.width < 800)
		setTimeout("document.location.reload()", 1000);
	else
	{
		headerUpdateVolumeBar(volume, 0);
		data.location.reload();
	}
}

function toggleMute(xy)
{
	document.location = "/setVolume?mute="+xy;
	if (window.screen.width < 800)
		setTimeout("document.location.reload()", 1000);
	else
	{
		headerUpdateVolumeBar(data.volume, 0);
		data.location.reload();
	}
}

function switchChannel(xy, bouquet, channel)
{
	if (window.screen.width < 800)
	{
		NewWindow('/cgi-bin/zapTo?path='+xy+'&curBouquet='+bouquet+'&curChannel='+channel, 'zap', '1', '1', 'no');
	}
	else
	{
		if (zapMode < 4)
			document.location = "/cgi-bin/zapTo?path="+xy+"&curBouquet="+bouquet+"&curChannel="+channel;
		else
			document.location = "?path="+xy+"&mode=zap&zapmode=4&zapsubmode=1";

		setTimeout("parent.data.location.reload()", 500);
	}
}

function logging()
{
	parent.body.document.location = "/log/debug.html";
}

function remoteControl(box)
{
	if (box == "dbox2")
		NewWindow("/showRemoteControl", "RC", "165", "500", "no");
	else
		NewWindow("/showRemoteControl", "RC", "1024", "640", "no");
}

function satFinder(transponder)
{
	NewWindow("/satFinder?" + transponder, "satfind", "170", "150", "no");
}

function openEPG(ref)
{
	NewWindow('/getcurrentepg?type=extended&ref=' + ref, 'EPG', '780', screen.height, 'yes');
}

function openMultiEPG(ref)
{
	NewWindow('/getMultiEPG?ref=' + ref, 'MultiEPG', screen.width, screen.height, 'yes');
}

function admin(command)
{
	NewWindow('/cgi-bin/admin?command=' + command + '&requester=webif', 'admin', '200', '100', 'no', '3000');
}

function openSI()
{
	NewWindow("/xml/streaminfo", "si", "780", "700", "yes");
}

function openChannelInfo()
{
	NewWindow("/cgi-bin/channelinfo", "ci", "780", "600", "yes");
}

function DVRrecord(command)
{
	document.location = "/cgi-bin/videocontrol?command=" + command; 
	setTimeout("document.location.reload()", 500);
}

function sendMessage2TV()
{
	NewWindow("/tvMessageWindow", "msg", "780", "150", "no");
}

function selectAudio()
{
	NewWindow("/cgi-bin/selectAudio?requester=webif", "audio", "250", "130", "no");
}

function selectSubChannel()
{
	NewWindow("/cgi-bin/selectSubChannel", "subchannel", "250", "130", "no");
}

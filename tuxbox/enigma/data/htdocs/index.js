function setVol(xy)
{
	document.location = "/setVolume?volume="+xy;
	setTimeout("reload()", 100);
}

function setVid(xy)
{
	document.location = "/setVideo?position="+xy;
	setTimeout("reload()", 100);
}

function Mute(xy)
{
	document.location = "/setVolume?mute="+xy;
	setTimeout("reload()", 100);
}

function switchChannel(xy, bouquet, channel)
{
	document.location = "/cgi-bin/zapTo?path="+xy+"&curBouquet="+bouquet+"&curChannel="+channel;
	if (screen.width < 800)
		setTimeout("reload()", 3000);
	else
		setTimeout("parent.header.location.reload()", 3000);
}

function deleteMovie(xy)
{
	if (confirmAction('Do you really want to delete this movie?'))
	{
		document.location = "/cgi-bin/deleteMovie?ref="+xy;
		setTimeout("reload()", 3000);
	}
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

function vcontrol(xy)
{
	document.location = "/cgi-bin/videocontrol?command="+xy;
}

function openSI()
{
	NewWindow("/cgi-bin/streaminfo?requester=webif", "si", "300", "250", "no");
}

function openChannelInfo()
{
	NewWindow("/cgi-bin/channelinfo", "ci", "780", "400", "yes");
}

function DVRrecord(xy)
{
	NewWindow("/cgi-bin/record?command="+xy, "record", "200", "100", "no", "5000");
	setTimeout("reload()", 2000);
}

function startPlugin(xy)
{
	document.location = "/cgi-bin/startPlugin?requester=webif&name="+xy;
}

function stopPlugin()
{
	document.location = "/cgi-bin/stopPlugin?requester=webif";
}

function sendMessage2TV()
{
	NewWindow("/tvMessageWindow", "msg", "780", "150", "no");
}

function selectAudio()
{
	NewWindow("/cgi-bin/selectAudio?requester=webif", "audio", "200", "100", "no");
}

function record(xy)
{
	NewWindow('/addTimerEvent?'+xy, 'record', '200', '200', 'no', '5000');
}

function EPGDetails(xy)
{
	NewWindow('/EPGDetails?'+xy, 'EPGDetails', '600', '700', 'no');
}

function switchChannel(xy)
{
	win=window.open("/?path="+xy, "switchStatus","width=1,height=1,left=0,top=0");
	win.focus();
	parent.setTimeout("reload()", 3000);
}

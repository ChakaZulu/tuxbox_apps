function record(xy)
{
	NewWindow('addTimerEvent?'+xy, 'record', '200', '200', 'no', '5000');
}

function EPGDetails(xy)
{
	NewWindow('EPGDetails?'+xy, 'EPGDetails', '780', '400', 'no');
}

function switchChannel(xy)
{
	document.location = "cgi-bin/zapTo?path="+xy;
}

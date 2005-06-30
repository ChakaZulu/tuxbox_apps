function record(ref, start, duration, description, channel)
{
	NewWindow('/addTimerEvent?ref=' + ref + '&start=' + start + '&duration=' + duration + '&descr=' + description + '&channel=' + channel, 'record', '200', '200', 'no', '5000');
}

function EPGDetails(xy)
{
	NewWindow('/EPGDetails?' + xy, 'EPGDetails', '780', '400', 'no');
}

function switchChannel(xy)
{
	document.location = "/cgi-bin/zapTo?path=" + xy;
}

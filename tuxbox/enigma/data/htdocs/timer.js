function cleanupTimerList()
{
	document.location = "/cleanupTimerList";
	parent.setTimeout("reload()", 500);
}

function clearTimerList()
{
	document.location = "/clearTimerList";
	parent.setTimeout("reload()", 500);
}

function editTimerEvent(xy)
{
	NewWindow('/editTimerEvent?'+xy, 'editTimer', '750', '200', 'no');
}

function deleteTimerEvent(xy)
{
	document.location = "/deleteTimerEvent?"+xy;
	parent.setTimeout("reload()", 500);
}

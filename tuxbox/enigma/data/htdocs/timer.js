function cleanupTimerList()
{
	document.location = "/cleanupTimerList";
	setTimeout("reload()", 500);
}

function clearTimerList()
{
	document.location = "/clearTimerList";
	setTimeout("reload()", 500);
}

function editTimerEvent(xy)
{
	NewWindow('/editTimerEvent?'+xy, 'editTimer', '750', '200', 'no');
}

function deleteTimerEvent(xy)
{
	document.location = "/deleteTimerEvent?"+xy;
	setTimeout("reload()", 500);
}

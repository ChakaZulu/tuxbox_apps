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
	NewWindow('/editTimerEvent?'+xy, 'editTimer', '780', '250', 'no');
}

function deleteTimerEvent(xy)
{
	document.location = "/deleteTimerEvent?"+xy;
	setTimeout("reload()", 500);
}

function addTimerEvent()
{
	alert("Sorry, this function is not working yet...");
}

function showAddTimerEventWindow()
{
	NewWindow("/showAddTimerEventWindow", 'addTimer', '780', '250', 'no');
}



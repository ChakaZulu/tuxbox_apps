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

function editTimerEvent(xy)
{
	NewWindow('/editTimerEvent?'+xy, 'editTimer', '600', '300', 'no');
}

function deleteTimerEvent()
{
	win=window.open("/deleteTimerEvent", "switchStatus", "width=1, height=1, left=0, top=0");
	win.focus();
	parent.setTimeout("reload()", 500);
}

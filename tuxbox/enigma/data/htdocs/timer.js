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
	NewWindow('/editTimerEvent?'+xy, 'editTimer', '780', '260', 'no');
}

function deleteTimerEvent(xy)
{
	NewWindow('/deleteTimerEvent?'+xy, 'deleteTimer', '300', '150', 'no');
}

function addTimerEvent()
{
	var currentChannel = document.channelselector.channel.selectedIndex;
	var channel = document.channelselector.channel.options[currentChannel].value;
	var currentSday = document.channelselector.sday.selectedIndex;
	var sday = document.channelselector.sday.options[currentSday].text;
	var currentSmonth = document.channelselector.smonth.selectedIndex;
	var smonth = document.channelselector.smonth.options[currentSmonth].text;
	var currentShour = document.channelselector.shour.selectedIndex;
	var shour = document.channelselector.shour.options[currentShour].text;
	var currentSmin = document.channelselector.smin.selectedIndex;
	var smin = document.channelselector.smin.options[currentSmin].text;
	var currentEday = document.channelselector.eday.selectedIndex;
	var eday = document.channelselector.eday.options[currentEday].text;
	var currentEmonth = document.channelselector.emonth.selectedIndex;
	var emonth = document.channelselector.emonth.options[currentEmonth].text;
	var currentEhour = document.channelselector.ehour.selectedIndex;
	var ehour = document.channelselector.ehour.options[currentEhour].text;
	var currentEmin = document.channelselector.emin.selectedIndex;
	var emin = document.channelselector.emin.options[currentEmin].text;
	var descr = document.channelselector.descr.value;
	var currentAfterEvent = document.channelselector.after_event.selectedIndex;
	var after_event = document.channelselector.after_event.options[currentAfterEvent].value;

	var url = '/addTimerEvent2?ref=' + channel + '&sday=' + sday + '&smonth=' + smonth + '&shour=' + shour + '&smin=' + smin + '&eday=' + eday + '&emonth=' + emonth + '&ehour=' + ehour + '&emin=' + emin + '&descr=' + descr + '&after_event=' + after_event;
	NewWindow(url, 'add', '200', '200', 'no', '5000');
}

function showAddTimerEventWindow()
{
	NewWindow("/showAddTimerEventWindow", 'addTimer', '780', '280', 'no');
}



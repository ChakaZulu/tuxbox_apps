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

	var url = '/addTimerEvent2?ref=' + channel + '&sday=' + sday + '&smonth=' + smonth + '&shour=' + shour + '&smin=' + smin + '&eday=' + eday + '&month=' + emonth + '&hour=' + ehour + '&emin=' + emin + '&descr=' + descr;
	NewWindow(url, 'add', '200', '200', 'no');
}

function showAddTimerEventWindow()
{
	NewWindow("/showAddTimerEventWindow", 'addTimer', '780', '250', 'no');
}



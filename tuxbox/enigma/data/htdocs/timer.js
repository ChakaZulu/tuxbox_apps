function cleanupTimerList()
{
	document.location = "cleanupTimerList";
	setTimeout("reload()", 500);
}

function clearTimerList()
{
	document.location = "clearTimerList";
	setTimeout("reload()", 500);
}

function editTimerEvent(xy)
{
	NewWindow('showEditTimerEventWindow?'+xy, 'editTimer', '780', '350', 'no');
}

function deleteTimerEvent(xy)
{
	if (confirmAction('Do you really want to delete this timer event?'))
	{
		NewWindow('deleteTimerEvent?'+xy, 'deleteTimer', '300', '150', 'no');
		setTimeout("reload()", 1000);
	}
}

function addTimerEvent()
{
	var url = "";
	var currentTimer = document.channelselector.type.selectedIndex;
	var timer = document.channelselector.type.options[currentTimer].value;
	var currentChannel = document.channelselector.channel.selectedIndex;
	var channel = document.channelselector.channel.options[currentChannel].value;
	if (timer == "regular")
	{
		var currentSday = document.channelselector.sday.selectedIndex;
		var sday = document.channelselector.sday.options[currentSday].text;
		var currentSmonth = document.channelselector.smonth.selectedIndex;
		var smonth = document.channelselector.smonth.options[currentSmonth].text;
	}
	var currentShour = document.channelselector.shour.selectedIndex;
	var shour = document.channelselector.shour.options[currentShour].text;
	var currentSmin = document.channelselector.smin.selectedIndex;
	var smin = document.channelselector.smin.options[currentSmin].text;
	if (timer == "regular")
	{
		var currentEday = document.channelselector.eday.selectedIndex;
		var eday = document.channelselector.eday.options[currentEday].text;
		var currentEmonth = document.channelselector.emonth.selectedIndex;
		var emonth = document.channelselector.emonth.options[currentEmonth].text;
	}
	var currentEhour = document.channelselector.ehour.selectedIndex;
	var ehour = document.channelselector.ehour.options[currentEhour].text;
	var currentEmin = document.channelselector.emin.selectedIndex;
	var emin = document.channelselector.emin.options[currentEmin].text;
	var descr = document.channelselector.descr.value;
	var currentAfterEvent = document.channelselector.after_event.selectedIndex;
	var after_event = document.channelselector.after_event.options[currentAfterEvent].value;
	if (timer == "repeating")
	{
		var mo = "off";
		var tu = "off";
		var we = "off";
		var th = "off";
		var fr = "off";
		var sa = "off";
		var su = "off";

		if (document.channelselector.mo.checked)
			mo = "on";
		if (document.channelselector.tu.checked)
			tu = "on";
		if (document.channelselector.we.checked)
			we = "on";
		if (document.channelselector.th.checked)
			th = "on";
		if (document.channelselector.fr.checked)
			fr = "on";
		if (document.channelselector.sa.checked)
			sa = "on";
		if (document.channelselector.su.checked)
			su = "on";

		url = 'addTimerEvent?timer=repeating&ref=' + channel + '&shour=' + shour + '&smin=' + smin + '&ehour=' + ehour + '&emin=' + emin + '&mo=' + mo + '&tu=' + tu + '&we=' + we + '&th=' + th + '&fr=' + fr + '&sa=' + sa + '&su=' + su + '&descr=' + descr + '&after_event=' + after_event;
	}
	else
		url = 'addTimerEvent?timer=regular&ref=' + channel + '&sday=' + sday + '&smonth=' + smonth + '&shour=' + shour + '&smin=' + smin + '&eday=' + eday + '&emonth=' + emonth + '&ehour=' + ehour + '&emin=' + emin + '&descr=' + descr + '&after_event=' + after_event;

	NewWindow(url, 'add', '200', '200', 'no', '5000');
}

function showAddTimerEventWindow(timer)
{
	NewWindow("showAddTimerEventWindow?timer=" + timer, 'addTimer', '780', '380', 'no');
}



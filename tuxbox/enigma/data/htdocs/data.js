function getUpdateCycleTime()
{
	return updateCycleTime;
}

function updateVolumeBar()
{
	for (var i = 9; i <= 63; i += 6)
	{
		var vol = 0;
		if (mute == 0)
			vol = volume;
		if (i <= vol)
			parent.header.getElem("id", "imgVol" + i, null).src = "led_on.gif";
		else
			parent.header.getElem("id", "imgVol" + i, null).src = "led_off.gif";
	}
}

function updateMute()
{
	if (mute == 0)
		parent.header.getElem("id", "mute", null).src = "speak_on.gif";
	else
		parent.header.getElem("id", "mute", null).src = "speak_off.gif";
}

function init(requester)
{
	<!-- service & epg data -->
	parent.header.getElem("id", "servicename", null).firstChild.nodeValue = serviceName;
	parent.header.getElem("id", "nowt", null).firstChild.nodeValue = nowT;
	parent.header.getElem("id", "nowd", null).firstChild.nodeValue = nowD;
	parent.header.getElem("id", "nowst", null).firstChild.nodeValue = nowSt;
	parent.header.getElem("id", "nextt", null).firstChild.nodeValue = nextT;
	parent.header.getElem("id", "nextd", null).firstChild.nodeValue = nextD;
	parent.header.getElem("id", "nextst", null).firstChild.nodeValue = nextSt;
	
	<!-- status bar -->
	parent.header.getElem("id", "diskgb", null).firstChild.nodeValue = diskGB;
	parent.header.getElem("id", "diskh", null).firstChild.nodeValue = diskH;
	parent.header.getElem("id", "vpid", null).firstChild.nodeValue = vpid;
	parent.header.getElem("id", "apid", null).firstChild.nodeValue = apid;
	parent.header.getElem("id", "ip", null).firstChild.nodeValue = ip;
	parent.header.getElem("id", "lock", null).firstChild.nodeValue = lock;
	parent.header.getElem("id", "uptime", null).firstChild.nodeValue = upTime;
	
	<!-- volume bar -->
	updateVolumeBar();
	
	<!-- mute -->
	updateMute();
	
	<!-- channel stats -->
	if (dolby == 1)
		parent.header.getElem("id", "imgDolby", null).src = "dolby_on.png";
	else
		parent.header.getElem("id", "imgDolby", null).src = "dolby_off.png";
	if (crypt == 1)
		parent.header.getElem("id", "imgCrypt", null).src = "crypt_on.png";
	else
		parent.header.getElem("id", "imgCrypt", null).src = "crypt_off.png";
	if (format == 1)
		parent.header.getElem("id", "imgFormat", null).src = "format_on.png";
	else
		parent.header.getElem("id", "imgFormat", null).src = "format_off.png";

	<!-- recording -->
	if (recording == 1)
		parent.header.getElem("id", "recording", null).src = "blinking_red.gif";
	else
		parent.header.getElem("id", "recording", null).src = "trans.gif";
	
	<!-- channavi -->
	parent.channavi.getElem("id", "filler", null).width = 780 - 110 - 110 * (butAudio + butVideo + butEPG + butInfo + butStream + butRecord + butStop);
}
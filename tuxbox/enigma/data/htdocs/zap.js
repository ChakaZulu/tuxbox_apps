function zapBouquetKeyUp()
{
	switch(window.event.keyCode)
	{
		case 33:
		case 38:
			bouquetBackward();
			document.channelselector.bouquet.focus();
			break;
		case 34:
		case 40:
			bouquetForward();
			document.channelselector.bouquet.focus();
			break;
		case 39:
			document.channelselector.channel.focus();
			break;
		default:
	}
}
function zapChannelKeyUp()
{
	switch(window.event.keyCode)
	{
		case 13:
			channelChange();
			break;
		case 37:
			document.channelselector.bouquet.focus();
			break;
		case 38:
			if (document.channelselector.channel.selectedIndex == 0)
			{
				if (autoBouquetChange == 0)
					document.channelselector.channel.selectedIndex = channels[currentBouquet].length - 1;
				else
					if (currentChannel == 0)
						bouquetBackward();
			}
			channelChange();
			break;
		case 40:
			if (document.channelselector.channel.selectedIndex >= channels[currentBouquet].length - 1)
			{
				if (autoBouquetChange == 0)
					document.channelselector.channel.selectedIndex = 0;
				else
					if (currentChannel == channels[currentBouquet].length - 1)
						bouquetForward();
			}
			channelChange();
			break;
		case 33:
			bouquetBackward();
			break;
		case 34:
			bouquetForward();
			break;
		default:
	}
}
function zapHeaderReload()
{
	parent.header.location.reload();
	document.channelselector.channel.focus();
}
function channelChange()
{
	currentChannel = document.channelselector.channel.selectedIndex;
	currentBouquet = document.channelselector.bouquet.selectedIndex;
	if (currentChannel >= 0)
	{
		var channel = document.channelselector.channel.options[currentChannel].value;
		switchChannel(channel, currentBouquet, currentChannel);
	}
}
function zapChannelForward()
{
	currentChannel = currentChannel + 1;
	if (currentChannel >= channels[currentBouquet].length)
		if (autoBouquetChange == 0)
			currentChannel = 0;
		else
			bouquetForward();
	document.channelselector.channel.selectedIndex = currentChannel;
	var channel = document.channelselector.channel.options[currentChannel].value;
	switchChannel(channel, currentBouquet, currentChannel);
}
function bouquetForward()
{
	currentBouquet = currentBouquet + 1;
	if (currentBouquet >= bouquets.length)
		currentBouquet = 0;
	loadChannels(currentBouquet, 0);
	document.channelselector.bouquet.selectedIndex = currentBouquet;
	document.channelselector.channel.selectedIndex = 0;
}
function zapChannelBackward()
{
	currentChannel = currentChannel - 1;
	if (currentChannel < 0)
		if (autoBouquetChange == 0)
			currentChannel = channels[currentBouquet].length - 1;
		else
			bouquetBackward();
	document.channelselector.channel.selectedIndex = currentChannel;
	var channel = document.channelselector.channel.options[currentChannel].value;
	switchChannel(channel, currentBouquet, currentChannel);
}
function bouquetBackward()
{
	currentBouquet = currentBouquet - 1;
	if (currentBouquet < 0)
		currentBouquet = bouquets.length - 1;
	loadChannels(currentBouquet, currentChannel);
	document.channelselector.bouquet.selectedIndex = currentBouquet;
	document.channelselector.channel.selectedIndex = channels[currentBouquet].length - 1;
}
function bouquetChange()
{
	var channel = -1;
	var bouquet = document.channelselector.bouquet.selectedIndex;
	if (bouquet == currentBouquet)
		channel = currentChannel;
	loadChannels(bouquet, channel);
}
function epg()
{
	var selChannel = document.channelselector.channel.selectedIndex;
	if (selChannel >= 0)
		openEPG(document.channelselector.channel.options[selChannel].value);
	else
		alert("No Channel selected");
}
function mepg()
{
	openMultiEPG(document.channelselector.bouquet.options[currentBouquet].value);
}
function loadChannels(bouquet, channel)
{
	deleteChannelOptions();
	addChannelOptions(bouquet);
	document.channelselector.channel.selectedIndex = channel;
	document.channelselector.channel.focus();
}
function addChannelOptions(bouquet)
{
	for (var i = 0; i < channels[bouquet].length; i++)
	{
		newOption = new Option(channels[bouquet][i], channelRefs[bouquet][i], false, true);
		document.channelselector.channel.options[document.channelselector.channel.length] = newOption;
	}
}
function deleteChannelOptions()
{
	var j = document.channelselector.channel.options.length;
	for (var i = j - 1 ; i >= 0; i--)
		document.channelselector.channel.options[i] = null;
}
function loadBouquets(bouquet)
{
	for (var i = 0; i < bouquets.length; i++)
	{
		newOption = new Option(bouquets[i], bouquetRefs[i], false, true);
		document.channelselector.bouquet.options[i] = newOption;
	}
	document.channelselector.bouquet.selectedIndex = bouquet;
}

function zapBouquetKeyUp()
{
	var key = window.event.keyCode;
	if (key == 33 || key == 38)
	{
		bouquetBackward();
		document.channelselector.bouquet.focus();
	}
	else
	if (key == 34 || key == 40)
	{
		bouquetForward();
		document.channelselector.bouquet.focus();
	}
	else 
	if (key == 39)
		document.channelselector.channel.focus();
}
function zapChannelKeyUp()
{
	var key = window.event.keyCode;
	if (key == 13)
		channelChange();
	else
	if (key == 37)
		document.channelselector.bouquet.focus();
	else
	if (key == 38)
	{
		if ((document.channelselector.channel.selectedIndex == 0) && (autoBouquetChange == 1))
		{
			if (currentChannel == 0)
				bouquetBackward();
		}
		channelChange();
	}
	else
	if (key == 40)
	{
		if ((document.channelselector.channel.selectedIndex >= channels[currentBouquet].length - 1) && (autoBouquetChange == 1))
		{
			if (currentChannel == channels[currentBouquet].length - 1)
				bouquetForward();
		}
		channelChange();
	}
	else
	if (key == 33)
		bouquetBackward();
	else
	if (key == 34)
		bouquetForward();
}
function zapHeaderReload()
{
	parent.header.location.reload();
	document.channelselector.channel.focus();
}
function channelChange()
{
	currentChannel = document.channelselector.channel.selectedIndex;
	if (currentChannel >= 0)
	{
		var channel = document.channelselector.channel.options[currentChannel].value;
		currentBouquet = document.channelselector.bouquet.selectedIndex;
		switchChannel(channel, currentBouquet, currentChannel);
	}
}
function zapChannelForward()
{
	currentChannel = currentChannel + 1;
	if (currentChannel >= channels[currentBouquet].length)
		currentChannel = 0;
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
		currentChannel = channels[currentBouquet].length - 1;
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
	{
		var channel = document.channelselector.channel.options[selChannel].value;
		openEPG(channel);
	}
	else
		alert("No Channel selected");
}
function mepg()
{
	var bouquet = document.channelselector.bouquet.options[currentBouquet].value;
	openMultiEPG(bouquet);
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

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
function zapChannelBackward()
{
	currentChannel = currentChannel - 1;
	if (currentChannel < 0)
		currentChannel = channels[currentBouquet].length - 1;
	document.channelselector.channel.selectedIndex = currentChannel;
	var channel = document.channelselector.channel.options[currentChannel].value;
	switchChannel(channel, currentBouquet, currentChannel);
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

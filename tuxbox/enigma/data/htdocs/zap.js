function channelChange()
{
	var channel = "";
	var j = document.channelselector.channel.selectedIndex;
	if (j >= 0)
	{
		channel = document.channelselector.channel.options[j].value;
		switchChannel(channel);
	}
}
function bouquetChange()
{
	var i = document.channelselector.bouquet.selectedIndex;
	loadChannels(i);
}
function epg()
{
	var channel = "";
	var j = document.channelselector.channel.selectedIndex;
	if (j >= 0)
	{
		channel = document.channelselector.channel.options[j].value;
		openEPG(channel);
	}
	else
		alert("No Channel selected");
}
function mepg()
{
	var i = document.channelselector.bouquet.selectedIndex;
	var bouquet = document.channelselector.bouquet.options[i].value;
	openMultiEPG(bouquet);
}
function loadChannels(bouquet)
{
	deleteChannelOptions();
	addChannelOptions(bouquet);
	document.channelselector.channel.selectedIndex = -1;
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
function loadBouquets()
{
	for (var i = 0; i < bouquets.length; i++)
	{
		newOption = new Option(bouquets[i], bouquetRefs[i], false, true);
		document.channelselector.bouquet.options[i] = newOption;
	}
	document.channelselector.bouquet.selectedIndex = 0;
}
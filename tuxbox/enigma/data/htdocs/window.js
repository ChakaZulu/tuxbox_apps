function NewWindow(mypage, myname, w, h, scroll, timeout)
{
	var winl = (screen.width - w) / 2;
	var wint = (screen.height - h) / 2;
	winprops = 'height='+h+', width='+w+', top='+wint+', left='+winl+', scrollbars='+scroll+', resizable'
	win = window.open(mypage, myname, winprops)
	if (parseInt(navigator.appVersion) >= 4) { win.window.focus(); }
	if (timeout > 0) { win.window.setTimeout("close()", timeout); }
}

function reload()
{
	document.location.reload();
}

function confirmAction(xy)
{
	var check = window.confirm(xy);
	return(check);
}

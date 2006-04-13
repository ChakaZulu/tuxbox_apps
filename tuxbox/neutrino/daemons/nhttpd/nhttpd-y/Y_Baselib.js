/*DHTML - Table*/
function y_add_row_to_table(_table, _class)
{
	var __row=document.createElement("TR");
	var __class = document.createAttribute("class");
	__class.nodeValue = _class;
	__row.setAttributeNode(__class);
	_table.appendChild(__row);
	return __row;
}
function y_add_plain_cell_to_row(_row, _name)
{
	var __cell=document.createElement("TD");
	__cell.setAttribute("name", _name);
	_row.appendChild(__cell);
	return __cell;
}
function y_add_text_cell_to_row(_row, _name, _value)
{
	var __cell=y_add_plain_cell_to_row(_row, _name);
	var __text=document.createTextNode(_value);
	__cell.appendChild(__text);
	return __cell;
}
function y_add_html_cell_to_row(_row, _name, _value)
{
	var __cell=y_add_plain_cell_to_row(_row, _name);
	__cell.innerHTML = _value;
	return __cell;
}

/*XMLHttpRequest AJAX*/
var g_req;
function loadXMLDoc(_url, _processReqChange) 
{
	if (window.XMLHttpRequest) 
	{
		g_req = new XMLHttpRequest();
		g_req.onreadystatechange = _processReqChange;
		if(g_req.overrideMimeType)
		{	g_req.overrideMimeType('text/xml');}
		g_req.open("GET", _url, true);
		g_req.send(null);
	} 
	else if (window.ActiveXObject)
	{
		g_req = new ActiveXObject("Microsoft.XMLHTTP");
		if (g_req) 
		{
			g_req.onreadystatechange = _processReqChange;
			g_req.open("GET", _url, true);
			g_req.send();
		}
	} 
	else
		alert("Kein Browser-Support für XMLHttpRequest");
}
function loadSyncURL(_url) 
{
	var _req;
	if (window.XMLHttpRequest) 
	{
		_req = new XMLHttpRequest();
		_req.open("GET", _url, false);
		_req.send(null);
	} 
	else if (window.ActiveXObject)
	{
		_req = new ActiveXObject("Microsoft.XMLHTTP");
		if(_req) 
		{
			_req.open("GET", _url, false);
			_req.send();
		}
	} 
	else
		alert("Kein Browser-Support für XMLHttpRequest");
	if (_req.readyState == 4 && _req.status == 200) 
		return _req.responseText;
	else
		return "";
}

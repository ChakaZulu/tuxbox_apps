/***************************************************************************
    copyright            : (C) 2001 by TheDOC
    email                : thedoc@chatville.de
	homepage			 : www.chatville.de
	modified by			 : -
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*
$Log: xmlrpc.cpp,v $
Revision 1.1  2001/12/11 13:34:59  TheDOC
initial release

*/

#include "xmlrpc.h"

xmlrpc_value::xmlrpc_value()
{
	type = NOTYPE;
}

xmlrpc_value::xmlrpc_value(int type, void* value)
{
	setValue(type, value);
}

void xmlrpc_value::setValue(int t, void* value)
{
	type = t;
	switch(type)
	{
	case INT:
		int_value = (int&)value;
		break;
	case BOOLEAN:
		boolean_value = (bool&)value;
		break;
	case DOUBLE:
		double_value = (double&)value;
		break;
	case DATETIME:
		datetime_value = (time_t&)value;
		break;
	case BASE64:
		base64_value = (std::string&)value;
		break;
	case STRING:
		string_value = (std::string&)value;
		break;
	case ARRAY:
		array_value = *(xmlrpc_array *)value;
		break;
	case STRUCT:
		struct_value = *(xmlrpc_struct *)value;
		break;
	}
}

// Einfache In-Order Traversierung (jedenfalls ne Abwandlung davon ;)
void xmlrpc_value::getXML(ostrstream *ostr)
{
	*ostr << "<value>" << endl;
	switch(type)
	{
	case INT:
		*ostr << "<i4>" << int_value << "</i4>" << endl;
		break;
	case BOOLEAN:
		*ostr << "<boolean>" << (boolean_value?1:0) << "</boolean>" << endl;
		break;
	case DOUBLE:
		*ostr << "<double>" << double_value << "</double>" << endl;
		break;
	case STRING:
		*ostr << "<string>" << string_value << "</string>" << endl;
		break;
	case DATETIME:
		*ostr << "<dateTime.iso8601>" << date_to_ISO8601(datetime_value) << "</dateTime.iso8601>" << endl;
	case ARRAY:
		*ostr << "<array><data>" << endl;
		for (int i = 0; i < array_value.size(); i++)
		{
			array_value[i]->getXML(ostr);
		}
		*ostr << "</data></array>" << endl;
		break;
	case STRUCT:
		*ostr << "<struct>" << endl;
		for (xmlrpc_struct::iterator it = struct_value.begin(); it != struct_value.end(); ++it)
		{
			*ostr << "<member>" << endl << "<name>" << (*it).first << "</name>" << endl;
			(*it).second->getXML(ostr);
			*ostr << "</member>" << endl;
		}
		*ostr << "</struct>" << endl;
		break;
	}
	*ostr << "</value>" << endl;
	//return ostr.str();
}


// Stolen from XMLRPC-EPI
time_t xmlrpc_value::date_from_ISO8601 (const char *text)
{
	struct tm tm;
	int n;
	int i;
	time_t t;
	char buf[18];

	if (strchr (text, '-')) {
		char *p = (char *) text, *p2 = buf;
		while (p && *p) {
			if (*p != '-') {
				*p2 = *p;
				p2++;
			}
			p++;
		}
		text = buf;
	}


	tm.tm_isdst = -1;

	if (strlen (text) < 17) {
		return -1;
	}

	n = 1000;
	tm.tm_year = 0;
	for (i = 0; i < 4; i++) {
		tm.tm_year += (text[i] - '0') * n;
		n /= 10;
	}
	n = 10;
	tm.tm_mon = 0;
	for (i = 0; i < 2; i++) {
		tm.tm_mon += (text[i + 4] - '0') * n;
		n /= 10;
	}
	tm.tm_mon--;

	n = 10;
	tm.tm_mday = 0;
	for (i = 0; i < 2; i++) {
		tm.tm_mday += (text[i + 6] - '0') * n;
		n /= 10;
	}

	n = 10;
	tm.tm_hour = 0;
	for (i = 0; i < 2; i++) {
		tm.tm_hour += (text[i + 9] - '0') * n;
		n /= 10;
	}

	n = 10;
	tm.tm_min = 0;
	for (i = 0; i < 2; i++) {
		tm.tm_min += (text[i + 12] - '0') * n;
		n /= 10;
	}

	n = 10;
	tm.tm_sec = 0;
	for (i = 0; i < 2; i++) {
		tm.tm_sec += (text[i + 15] - '0') * n;
		n /= 10;
	}

	tm.tm_year -= 1900;

	return mktime (&tm);
}

std::string xmlrpc_value::date_to_ISO8601 (time_t value)
{
	struct tm *tm;
	tm = localtime (&value);
	char buf[20];

	strftime (buf, 20, "%Y%m%dT%H:%M:%S", tm);
	std::string tmp_string(buf);
	return tmp_string;
}

int xmlrpc_value::parseXML(std::vector<struct command> *command_list, int *counter)
{
	//cout << "Parse XML " << ((*command_list)[*counter].cmd) << " - " << counter << " - " << (*counter) << endl;
	
	if ((*command_list)[*counter].cmd == "value")
	{
		//cout << "value" << endl;
		(*counter)++;

		if ((*command_list)[*counter].cmd == "int" || (*command_list)[*counter].cmd == "i4")
		{
			(*counter)++;
			type = INT;
			int_value = atoi((*command_list)[*counter].parm.c_str());
			//cout << "Int-Value: " << int_value << endl;

			if ((*command_list)[*counter].cmd != "/int" && (*command_list)[*counter].cmd != "/i4")
			{
				cout << "</int> or </i4> missing" << endl;
				return -1;
			}
		}
		else if ((*command_list)[*counter].cmd == "double") // BUG BUG BUG geht irgendwie nicht mit atof... who needs double anyway? ;)
		{
			(*counter)++;
			type = DOUBLE;
			double_value = atof((*command_list)[*counter].parm.c_str());
			//cout << "Double-Value: " << double_value << endl;

			if ((*command_list)[*counter].cmd != "/double")
			{
				cout << "</double> missing" << endl;
				return -1;
			}
		}
		else if ((*command_list)[*counter].cmd == "string")
		{
			(*counter)++;
			type = STRING;
			string_value = (*command_list)[*counter].parm.c_str();
			//cout << "String-Value: " << string_value << endl;

			if ((*command_list)[*counter].cmd != "/string")
			{
				cout << "</string> missing" << endl;
				return -1;
			}
		}
		else if ((*command_list)[*counter].cmd == "boolean")
		{
			(*counter)++;
			type = BOOLEAN;
			if ((*command_list)[*counter].parm == "0")
				boolean_value = false;
			else if ((*command_list)[*counter].parm == "1")
				boolean_value = true;
			else
			{
				cout << "Wrong Boolean value: " << (*command_list)[*counter].parm << endl;
				return -1;
			}

			//cout << "Boolean-Value: " << boolean_value << endl;

			if ((*command_list)[*counter].cmd != "/boolean")
			{
				cout << "</boolean> missing" << endl;
				return -1;
			}
		}
		else if ((*command_list)[*counter].cmd == "dateTime.iso8601")
		{
			(*counter)++;
			type = DATETIME;
			datetime_value = date_from_ISO8601((*command_list)[*counter].parm.c_str());
			//cout << "Datetime-Value: " << datetime_value << endl;

			if ((*command_list)[*counter].cmd != "/dateTime.iso8601")
			{
				cout << "</dateTime.iso8601> missing" << endl;
				return -1;
			}
		}
		else if ((*command_list)[*counter].cmd == "array")
		{
			(*counter)++;
		
			//cout << "array" << endl;
			if ((*command_list)[*counter].cmd != "data")
			{
				cout << "<data> missing" << endl;
			}
			(*counter)++;

			type = ARRAY;
			array_value.clear();
			while((*command_list)[*counter].cmd != "/data")
			{
				xmlrpc_value *tmp_value = new xmlrpc_value();
				if (tmp_value->parseXML(command_list, counter) == -1)
					return -1;
				array_value.push_back(tmp_value);				
				(*counter)++;
			}
			(*counter)++;
			
			//cout << "Aktuelles Kommando: " << (*command_list)[*counter].cmd << endl;
			
			if ((*command_list)[*counter].cmd != "/array")
			{
				cout << "</array> missing" << endl;
				return -1;
			}

		}
		else if ((*command_list)[*counter].cmd == "struct")
		{
			(*counter)++;
			//cout << "struct" << endl;
			type = STRUCT;

			struct_value.clear();
					
			while((*command_list)[*counter].cmd == "member")
			{
				//cout << "member" << endl;
				(*counter)++;
				std::string structName;
				
				if ((*command_list)[*counter].cmd == "name")
				{
					//cout << "name" << endl;
					(*counter)++;
					structName = (*command_list)[*counter].parm;
					if ((*command_list)[*counter].cmd != "/name")
					{
						cout << "</name> missing" << endl;
						return -1;
					}
					(*counter)++;
				}
				else
				{
					cout << "Struct-Name missing" << endl;
					return -1;
				}
				xmlrpc_value *tmp_value = new xmlrpc_value();
				if (tmp_value->parseXML(command_list, counter) == -1)
					return -1;
				struct_value.insert(xmlrpc_value::xmlrpc_struct_pair(structName, tmp_value));
				(*counter)++;
				//cout << "COMMAND: " << (*command_list)[*counter].cmd << endl;
				if ((*command_list)[*counter].cmd != "/member")
				{
					cout << "</member> missing" << endl;
					return -1;
				}
				(*counter)++;
			}


			if ((*command_list)[*counter].cmd != "/struct")
			{
				cout << "</struct> missing" << endl;
				return -1;
			}

		}

		(*counter)++;
		if ((*command_list)[*counter].cmd != "/value")
		{
			cout << "</value> missing" << endl;
			return -1;
		}

	}

	else
	{
		cout << "<value> missing" << endl;
		return -1;
	}

	return 0;
}

xmlrpc_params::xmlrpc_params()
{
	params.clear();
}

void xmlrpc_params::addParam(xmlrpc_value* value)
{
	params.insert(params.end(), value);
}

void xmlrpc_params::getXML(ostrstream *ostr)
{
	if (params.size() != 0)
	{
		*ostr << "<params>" << endl;

		for (int i = 0; i < params.size(); i++)
		{
			*ostr << "<param>" << endl;
			params[i]->getXML(ostr);
			*ostr << "</param>" << endl;
		}
		*ostr << "</params>" << endl;
	}
}

void xmlrpc_fault::getXML(ostrstream *ostr)
{
	xmlrpc_value::xmlrpc_struct fault_struct;
	xmlrpc_value *code = new xmlrpc_value(INT, (void*) faultCode);
	xmlrpc_value *string = new xmlrpc_value(STRING, (void*) faultString.c_str());
	fault_struct.insert(xmlrpc_value::xmlrpc_struct_pair("faultCode", code));
	fault_struct.insert(xmlrpc_value::xmlrpc_struct_pair("faultString", string));
	xmlrpc_value *fault = new xmlrpc_value(STRUCT, &fault_struct);

	fault->getXML(ostr);
}

std::string xmlrpc_response::getXML()
{
	ostrstream ostr;
	ostr.clear();

	ostr << "<?xml version=\"1.0\"?>" << endl;
	ostr << "<methodResponse>" << endl;
	if (type == RESPONSE)
		params->getXML(&ostr);
	else
		fault->getXML(&ostr);
	ostr << "</methodResponse>" << endl << ends;
	
	return ostr.str();
}

int xmlrpc_response::parseXML(std::vector<struct command> *command_list, int *counter)
{
	params = new xmlrpc_params();

	if ((*command_list)[*counter].cmd == "params")
	{
		cout << "params" << endl;
		(*counter)++;

		while ((*command_list)[*counter].cmd == "param")
		{
			cout << "param" << endl;

			(*counter)++;
			xmlrpc_value *value = new xmlrpc_value();
			value->parseXML(command_list, counter);
			params->addParam(value);
			
			(*counter)++;
			if ((*command_list)[*counter].cmd != "/param")
			{
				cout << "</param> missing" << endl;
				return -1;
			}
			(*counter)++;
		}
	}
	else if ((*command_list)[*counter].cmd == "fault")
	{
		cout << "fault" << endl;
	}
	else
	{
		cout << "params-Fehler" << endl;
		return -1;
	}

	return 0;
}

int xmlrpc_request::parseXML(std::vector<struct command> *command_list, int *counter)
{
	params = new xmlrpc_params();

	if ((*command_list)[*counter].cmd == "params")
	{
		//cout << "params" << endl;
		(*counter)++;

		while ((*command_list)[*counter].cmd == "param")
		{
			//cout << "param" << endl;

			(*counter)++;
			xmlrpc_value *value = new xmlrpc_value();
			value->parseXML(command_list, counter);
			params->addParam(value);
			
			(*counter)++;
			if ((*command_list)[*counter].cmd != "/param")
			{
				cout << "</param> missing" << endl;
				return -1;
			}
			(*counter)++;
		}
	}
	else
	{
		cout << "params-Fehler" << endl;
		return -1;
	}

	return 0;
}

std::string xmlrpc_request::getXML()
{
	ostrstream ostr;
	ostr.clear();

	ostr << "<?xml version=\"1.0\"?>" << endl;
	ostr << "<methodCall>" << endl;
	ostr << "<methodName>" << methodName << "</methodName>" << endl;
	params->getXML(&ostr);
	ostr << "</methodCall>" << endl << ends;
	
	return ostr.str();
}

void xmlrpc_parse::readFile(std::string filename)
{
	std::ifstream input(filename.c_str());
	std::string line;

	xml.clear();

	while (std::getline(input, line))
	{
        xml.append(line);
    }
	cout << xml << endl;

	input.close();

}

int xmlrpc_parse::parseXML()
{
	std::istringstream iss(xml);
	std::string tmp_string;
	int parmcount = 0;
	while(std::getline(iss, tmp_string, '>'))
	{
		parmcount++;
		std::string parm;
		std::string cmd;
		
		std::istringstream iss2(tmp_string);
		
		std::getline(iss2, parm, '<');
		std::getline(iss2, cmd, '<');
		//cout << parm << " - " << cmd << endl;

		struct command tmp_command;
		tmp_command.parm = parm;
		tmp_command.cmd = cmd;
		
		command_list.push_back(tmp_command);
	}
	if (command_list.size() < 2)
	{
		type = FAILED;
		return -1;
	}

	/*if (command_list[0].cmd.substr(4) != "?xml")
	{
		cout << "Kein XML-Dokument" << endl;
		return -1;
	}*/

	int counter = 0;
	if (command_list[1].cmd == "methodResponse")
	{
		response = new xmlrpc_response();
		
		type = RESPONSE;
		//cout << "methodResponse" << endl;
		
		counter = 2;

		response->parseXML(&command_list, &counter);
		cout << response->getXML() << endl;
	}
	else if (command_list[1].cmd == "methodCall")
	{
		request = new xmlrpc_request();

		type = REQUEST;
		//cout << "methodCall" << endl;
		if (command_list[2].cmd == "methodName" && command_list[3].cmd == "/methodName")
		{
			request->setMethodName(command_list[3].parm);
		}
		else
		{
			cout << "methodName-Fehler" << endl;
			return -1;
		}

		counter = 4;
		request->parseXML(&command_list, &counter);
		//cout << request->getXML() << endl;
	}
	else 
	{
		cout << "Parse error on method" << endl;
		type = FAILED;
		return -1;
	}
}

std::string handle::makeHandle(int type, int count,  ...)
{
	ostrstream ostr;

	if (type == SERVICE)
	{
		va_list arguments;
		va_start(arguments, count);
		ostr << "SERVICE no=" << va_arg(arguments, int) << ends;
		va_end(arguments);
	}

	return ostr.str();
}

void handle::parseHandle(std::string tmp_handle)
{
	std::istringstream iss(tmp_handle);
	std::string command;
	
	std::getline(iss, command, ' ');

	valid = false;

	if (command == "SERVICE")
	{
		std::string tmp_string;
		type = SERVICE;
		while(std::getline(iss, tmp_string, ' '))
		{
			std::istringstream iss(tmp_string);
			std::string cmd;
			std::string parm;
			std::getline(iss, cmd, '=');
			std::getline(iss, parm, '=');

			if (cmd == "no")
			{
				channelnumber = atoi(parm.c_str());
				valid = true;
			}
		}
	}
}

void xmlrpc::setInput(std::string xml)
{
	xmlin = xml;
}

void xmlrpc::parse()
{
	xmlrpc_parse parser;
	xmlrpc_request *request;
	xmlrpc_response response;
	handle h;

	parser.setXML(xmlin);
	parser.parseXML();

	if (parser.getType() == REQUEST)
	{
		request = parser.getRequest();
		std::string methodName = request->getMethodName();
		std::string tmp_handle;

		if (methodName == "getList")
		{
			xmlrpc_value::xmlrpc_array list_array;
			
			tmp_handle = request->getParams()->getParam(0)->getStringValue();

			if (tmp_handle == "")
			{
				for (int count = 0; count < container_obj->channels_obj->numberChannels(); count++)
				{
					std::string tmp_string;
					xmlrpc_value::xmlrpc_struct channel_struct;
					xmlrpc_value *servicename = new xmlrpc_value(STRING, (void*) container_obj->channels_obj->getServiceName(count).c_str());
					tmp_string = "Service";
					xmlrpc_value *servicetype = new xmlrpc_value(STRING, (void*) tmp_string.c_str());
					xmlrpc_value *servicehandle = new xmlrpc_value(STRING, (void*) h.makeHandle(SERVICE, 1, count).c_str());
					
					channel_struct.insert(xmlrpc_value::xmlrpc_struct_pair("caption", servicename));
					channel_struct.insert(xmlrpc_value::xmlrpc_struct_pair("type", servicetype));
					channel_struct.insert(xmlrpc_value::xmlrpc_struct_pair("handle", servicehandle));

					xmlrpc_value *new_channel = new xmlrpc_value(STRUCT, &channel_struct);
					list_array.push_back(new_channel);
				}
			}
			xmlrpc_params params;
			xmlrpc_value *array_value = new xmlrpc_value(ARRAY, &list_array);
			params.addParam(array_value);
			response.setParams(&params);

			xmlout = response.getXML();
		}
		else if (methodName == "zapTo")
		{
			tmp_handle = request->getParams()->getParam(0)->getStringValue();

			h.parseHandle(tmp_handle);
			if (h.handleIsValid())
			{
					container_obj->channels_obj->setCurrentChannel(h.getChannelNumber());

					container_obj->channels_obj->zapCurrentChannel(container_obj->zap_obj, container_obj->tuner_obj);
					container_obj->channels_obj->setCurrentOSDProgramInfo(container_obj->osd_obj);
					
					container_obj->channels_obj->receiveCurrentEIT();
					container_obj->channels_obj->setCurrentOSDProgramEIT(container_obj->osd_obj);
					container_obj->channels_obj->updateCurrentOSDProgramAPIDDescr(container_obj->osd_obj);
					xmlrpc_params params;
					response.setParams(&params);
			}
			else
			{
				xmlrpc_fault fault;
				fault.setFaultCode(3);
				fault.setFaultString("invalid handle");

				response.setType(FAULT);
				response.setFault(&fault);
			}
			
			
			xmlout = response.getXML();
		}
		else if (methodName == "getInfo")
		{
			tmp_handle = request->getParams()->getParam(0)->getStringValue();

			xmlrpc_value::xmlrpc_struct tmp_struct;

			if (tmp_handle == "")
			{
				xmlrpc_value *value = new xmlrpc_value(STRING, (void*) h.makeHandle(SERVICE, 1, container_obj->channels_obj->getCurrentChannelNumber()).c_str());
				tmp_struct.insert(xmlrpc_value::xmlrpc_struct_pair("handle", value));
			}
			
			h.parseHandle(tmp_handle);
		
			if (h.handleIsValid() || tmp_handle == "")
			{
				std::string tmp_string;

				

				if (h.getType() == SERVICE)
				{
					int channelnumber = h.getChannelNumber();
					
					if (channelnumber != container_obj->channels_obj->getCurrentChannelNumber())
					{
						container_obj->channels_obj->setCurrentChannel(h.getChannelNumber());

						container_obj->channels_obj->zapCurrentChannel(container_obj->zap_obj, container_obj->tuner_obj);
						container_obj->channels_obj->setCurrentOSDProgramInfo(container_obj->osd_obj);
					
						container_obj->channels_obj->receiveCurrentEIT();
						container_obj->channels_obj->setCurrentOSDProgramEIT(container_obj->osd_obj);
						container_obj->channels_obj->updateCurrentOSDProgramAPIDDescr(container_obj->osd_obj);
					}
					
					{
						tmp_string = "";
						xmlrpc_value *value = new xmlrpc_value(STRING, (void*) tmp_string.c_str());
						tmp_struct.insert(xmlrpc_value::xmlrpc_struct_pair("parentHandle", value));
					}

					{
						xmlrpc_value *value = new xmlrpc_value(STRING, (void*) container_obj->channels_obj->getCurrentServiceName().c_str());
						tmp_struct.insert(xmlrpc_value::xmlrpc_struct_pair("caption", value));
					}

					{
						tmp_string = "Service";
						xmlrpc_value *value = new xmlrpc_value(STRING, (void*) tmp_string.c_str());
						tmp_struct.insert(xmlrpc_value::xmlrpc_struct_pair("type", value));
					}

					if (container_obj->channels_obj->getCurrentType() == 0x1)
					{
						xmlrpc_value *value = new xmlrpc_value(INT, (void*) container_obj->channels_obj->getCurrentVPID());
						tmp_struct.insert(xmlrpc_value::xmlrpc_struct_pair("videoPid", value));
					}

					{
						xmlrpc_value::xmlrpc_array apid_array;
						
						for (int i = 0; i < container_obj->channels_obj->getCurrentAPIDcount(); i++)
						{
							xmlrpc_value::xmlrpc_struct apid_struct;
							{
								xmlrpc_value *value = new xmlrpc_value(INT, (void*) container_obj->channels_obj->getCurrentAPID(i));
								apid_struct.insert(xmlrpc_value::xmlrpc_struct_pair("audioPid", value));
							}
							{
								tmp_string = (container_obj->channels_obj->getCurrentDD(i)?"ac3":"mpeg");
								xmlrpc_value *value = new xmlrpc_value(INT, (void*) tmp_string.c_str());
								apid_struct.insert(xmlrpc_value::xmlrpc_struct_pair("type", value));
							}

							xmlrpc_value *value = new xmlrpc_value(STRUCT, &apid_struct);
							apid_array.push_back(value);
						}

						xmlrpc_value *value = new xmlrpc_value(ARRAY, &apid_array);
						tmp_struct.insert(xmlrpc_value::xmlrpc_struct_pair("audioPids", value));
					}


				}
				xmlrpc_params params;
				xmlrpc_value *struct_value = new xmlrpc_value(STRUCT, &tmp_struct);
				params.addParam(struct_value);
				response.setParams(&params);
			}
			else
			{
				xmlrpc_fault fault;
				fault.setFaultCode(3);
				fault.setFaultString("invalid handle");

				response.setType(FAULT);
				response.setFault(&fault);
			}
			
			
			xmlout = response.getXML();
		}
		else if (methodName == "beginRecordMode")
		{
			tmp_handle = request->getParams()->getParam(0)->getStringValue();

			h.parseHandle(tmp_handle);
			if (h.handleIsValid())
			{
					int channelnumber = h.getChannelNumber();
					
					if (channelnumber != container_obj->channels_obj->getCurrentChannelNumber())
					{
						container_obj->channels_obj->setCurrentChannel(h.getChannelNumber());

						container_obj->channels_obj->zapCurrentChannel(container_obj->zap_obj, container_obj->tuner_obj);
						container_obj->channels_obj->setCurrentOSDProgramInfo(container_obj->osd_obj);
					
						container_obj->channels_obj->receiveCurrentEIT();
						container_obj->channels_obj->setCurrentOSDProgramEIT(container_obj->osd_obj);
						container_obj->channels_obj->updateCurrentOSDProgramAPIDDescr(container_obj->osd_obj);
					}
					container_obj->zap_obj->dmx_stop();
					xmlrpc_params params;
					response.setParams(&params);
			}
			else
			{
				xmlrpc_fault fault;
				fault.setFaultCode(3);
				fault.setFaultString("invalid handle");

				response.setType(FAULT);
				response.setFault(&fault);
			}
			
			
			xmlout = response.getXML();
		}
		else if (methodName == "endRecordMode")
		{
			tmp_handle = request->getParams()->getParam(0)->getStringValue();

			container_obj->zap_obj->dmx_start();

			xmlrpc_params params;
			response.setParams(&params);
						
			xmlout = response.getXML();
		}
	}
	else
	{
		xmlrpc_fault fault;
		fault.setFaultCode(0);
		fault.setFaultString("Syntax Error");
		
		response.setFault(&fault);

		xmlout = response.getXML();		
	}
	
}

std::string xmlrpc::getOutput()
{
	return xmlout;
}


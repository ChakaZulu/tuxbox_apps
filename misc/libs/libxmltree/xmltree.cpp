/* xmltree.cpp
 *
 * XMLTree API implementation
 *
 * $Id: xmltree.cpp,v 1.4 2009/11/22 15:36:38 rhabarber1848 Exp $
 *
 */
 
#define stricmp strcasecmp

#include <string.h>
#include <malloc.h>
#include "xmltree.h"

/* XMLAttribute class */

XMLAttribute::XMLAttribute()
{
  name=value=0;
  next=prev=0;
}

XMLAttribute::XMLAttribute(char *nam, char *val)
{
  name=value=0;
  next=prev=0;

  SetName(nam);
  SetValue(val);
}

XMLAttribute::XMLAttribute(XMLAttribute *prv, char *nam, char *val)
{
  name=value=0;

  SetName(nam);
  SetValue(val);
  SetPrevious(prv);

  next=0;
}

XMLAttribute::XMLAttribute(XMLAttribute *prv, char *nam, char *val, XMLAttribute *nxt)
{
  name=value=0;

  SetName(nam);
  SetValue(val);

  SetPrevious(prv);
  SetNext(nxt);
}

XMLAttribute::~XMLAttribute()
{
  if (name) { free(name); name=0; };
  if (value) { free(value); value=0; };

  next=prev=0;
}

void XMLAttribute::SetName(char *nam)
{
  if (name) free(name);

  name=strdup(nam);
}

void XMLAttribute::SetValue(char *val)
{
  if (value) free(value);

  value=strdup(val);
}

void XMLAttribute::SetNext(XMLAttribute *nxt)
{
  next=nxt;
}

void XMLAttribute::SetPrevious(XMLAttribute *prv)
{
  prev=prv;
}

/* XMLTreeNode class */

XMLTreeNode::XMLTreeNode(XMLTreeNode *prnt)
{
  attributes=0;
  next=child=0;
  type=data=0;
  dataSize=0;

  SetMatchingMode(prnt?prnt->mmode:MATCH_CASE);
  SetParent(prnt);
}

XMLTreeNode::XMLTreeNode(XMLTreeNode *prnt, char *typ)
{
  attributes=0;
  next=child=0;
  type=data=0;
  dataSize=0;

  SetType(typ);
  SetMatchingMode(prnt?prnt->mmode:MATCH_CASE);
  SetParent(prnt);
}

XMLTreeNode::XMLTreeNode(XMLTreeNode *prnt, char *typ, char *dta, unsigned int dtaSize)
{
  attributes=0;
  next=child=0;
  type=0;

  SetType(typ);
  SetData(dta, dtaSize);
  SetMatchingMode(prnt?prnt->mmode:MATCH_CASE);
  SetParent(prnt);
}

XMLTreeNode::XMLTreeNode(XMLTreeNode *prnt, char *typ, char *dta, unsigned int dtaSize, XMLTreeNode *cld)
{
  attributes=0;
  next=0;
  type=0;

  SetType(typ);
  SetData(dta, dtaSize);
  SetChild(cld);
  SetMatchingMode(prnt?prnt->mmode:MATCH_CASE);
  SetParent(prnt);
}

XMLTreeNode::XMLTreeNode(XMLTreeNode *prnt, char *typ, char *dta, unsigned int dtaSize, XMLTreeNode *cld, XMLTreeNode *nxt)
{
  attributes=0;
  type=0;

  SetType(typ);
  SetData(dta, dtaSize);
  SetChild(cld);
  SetNext(nxt);
  SetMatchingMode(prnt?prnt->mmode:MATCH_CASE);
  SetParent(prnt);
}

XMLTreeNode::~XMLTreeNode()
{
  DeleteAttributes();
  DeleteChildren();

  if (type) { free(type); type=0; };
  if (data) { delete[] data; dataSize=0; };

  next=0;
}

XMLAttribute *XMLTreeNode::GetAttribute(const char *name) const
{
  XMLAttribute *a;

  a=attributes;

  while (a)
  {
    if (a->GetName())
    {
      switch (mmode)
      {
        case MATCH_CASE:
          if (!strcmp(a->GetName(), name)) return a;

        case MATCH_NOCASE:
          if (!stricmp(a->GetName(), name)) return a;
      }
    }

    a=a->GetNext();
  }

  return 0;
}

char *XMLTreeNode::GetAttributeValue(const char *name) const
{
  XMLAttribute *a;

  a=GetAttribute(name);
  if (a) return a->GetValue();

  return 0;
}

void XMLTreeNode::AddNode(XMLTreeNode *node, addmode mode)
{
  XMLTreeNode *n=0;

  if (!node) return;
  node->next=0;

  switch (mode)
  {
    case ADD_NEIGHBOUR:
      n=this;

    case ADD_CHILD:
      n=child;
  }

  if (n)
  {
    while (n->next) n=n->next;

    n->next=node;
  }
  else
    child=node;
}

XMLTreeNode *XMLTreeNode::AddNode(addmode mode)
{
  XMLTreeNode *n=new XMLTreeNode(this);

  AddNode(n, mode);

  return n;
}

void XMLTreeNode::SetNext(XMLTreeNode *nxt)
{
  next=nxt;
}

void XMLTreeNode::SetChild(XMLTreeNode *cld)
{
  child=cld;
}

void XMLTreeNode::SetParent(XMLTreeNode *prnt)
{
  parent=prnt;
}

void XMLTreeNode::SetAttribute(char *name, char *value)
{
  XMLAttribute *a;

  a=GetAttribute(name);

  if (a)
    a->SetValue(value);
  else
  {
    a=attributes;

    if (a)
    {
      while (a->GetNext()) a=a->GetNext();

      a->SetNext(new XMLAttribute(a, name, value, 0));
    }
    else
    {
      a=attributes=new XMLAttribute(0, name, value, 0);
    }
  }
}

void XMLTreeNode::SetType(char *typ)
{
  if (type) free(type);

  type=strdup(typ);
}

void XMLTreeNode::SetData(char *dat, unsigned int datSize)
{
  if (data) delete[] data;

  dataSize=datSize;
  data=new char[dataSize+1];
  data[dataSize]=0;

  memcpy(data, dat, datSize);
}

void XMLTreeNode::AppendData(char *dat, unsigned int datSize)
{
  if (!data)
    SetData(dat, datSize);
  else
  {
    char *tmp;

    tmp=new char[dataSize+datSize];
    memcpy(tmp, data, dataSize);
    memcpy(tmp+dataSize, dat, datSize);

    SetData(tmp, dataSize+datSize);

    delete[] tmp;
  }
}

void XMLTreeNode::SetPDataOff(int pdo)
{
  pdataOff=pdo;
}

void XMLTreeNode::SetMatchingMode(matchmode mode)
{
  mmode=mode;
};

void XMLTreeNode::DeleteAttribute(char *name)
{
  XMLAttribute *a, *next, *prev;

  a=GetAttribute(name);
  if (!a) return;

  next=a->GetNext();
  prev=a->GetPrevious();

  if (prev) prev->SetNext(next);
  if (next) next->SetPrevious(prev);

  delete a;
}

void XMLTreeNode::DeleteAttributes()
{
  XMLAttribute *a, *b;

  a=attributes;

  while (a)
  {
    b=a->GetNext(); delete a;
    a=b;
  }

  attributes=0;
}

void XMLTreeNode::DeleteChildren()
{
  XMLTreeNode *a, *b;

  if (!child) return;
  a=child;

  while (a)
  {
    b=a->next; delete a;
    a=b;
  }

  child=0;
}

void XMLTreeParser::StartElementHandler(const XML_Char *name, const XML_Char **atts)
{
  XMLTreeNode     *n;
  const XML_Char **a=atts;

  n=new XMLTreeNode(current, (char *) name);

  if (a)
  {
    while (*a) { n->SetAttribute((char *) a[0], (char *) a[1]); a+=2; };
  }
  
  if (current)
  {
    n->SetPDataOff(current->GetDataSize());
    current->AddNode(n, XMLTreeNode::ADD_CHILD);
    current=n;
  }
  else
  {
    root=current=n;
  }
}

void XMLTreeParser::EndElementHandler(const XML_Char* /*name*/)
{
  if (current)
  {
    if (current->GetParent()) current=current->GetParent();
  }
}

void XMLTreeParser::CharacterDataHandler(const XML_Char *s, int len)
{
  if (current)
    current->AppendData((char *) s, len);
}

XMLTreeParser::XMLTreeParser(const XML_Char *encoding): XML_Parser(encoding)
{
  root=current=0;

  startElementHandler=endElementHandler=characterDataHandler=1;
}

XMLTreeParser::~XMLTreeParser()
{
  if (root)
  {
    delete root;
    root=current=0;
  }
}

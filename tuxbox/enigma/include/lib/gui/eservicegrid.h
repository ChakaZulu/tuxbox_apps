#ifndef __eservicegrid_h
#define __eservicegrid_h

#include "ewidget.h"

class eServiceGrid: public eWidget
{
	eService *(*grid);
	QSize gridsize;
	QSize elemsize;
	
	eTextPara *para;
	
	void validate();
	
	void createGrid(QSize gridsize);
	eService **allocateGrid(QSize size);
	
public:
	eServiceGrid(eWidget *parent);
	
	void setGridSize(int gx, int gy);
	void addService(eService *service);
	
};

#endif

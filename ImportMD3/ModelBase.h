#ifndef  _MODEL_BASE_H_
#define  _MODEL_BASE_H_

#include "sys_base.h"

class IGenericModelVertex
{
public:
	virtual double Up()		= 0;
	virtual double Right()	= 0;
	virtual double Front()	= 0;

	virtual void setUp(double v)		= 0;
	virtual void setRight(double v)	= 0;
	virtual void setFront(double v)	= 0;

	virtual double U()	= 0;
	virtual double V()	= 0;
};

class IGenericModelPolygon
{
public:
	virtual IGenericModelVertex& VertexByIndex(UINT32 idx)		= 0;
	virtual	Vector<double>		 Normal()						= 0;
};


#endif //  _MODEL_BASE_H_

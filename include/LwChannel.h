/**************************************
 *
 *  LwChannel.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Public interface into the channel objects
 *  Locks down an interface so it can be called
 *  w/o any cat reinterpretation
 *
 **************************************/

#ifndef _LW_LWCHANNEL_H
#define _LW_LWCHANNEL_H

#include <stdio.h>
#include <stdlib.h>
#include "string.h"

#include <lwrender.h>

#include "sys_extra.h"
#include "lwmrbxprt.h"

class LC_cChannel;

class LC_cVertexOffset
{
	friend LC_cChannel;
	LC_VertexOffset	innards;
public:
	LC_cVertexOffset()
	{
		innards.location[0] = 0;
		innards.location[1] = 0;
		innards.location[2] = 0;
	}

	static LC_cVertexOffset INVALID;
};

typedef Block<LC_cVertexOffset>		LW_FrameVertices;

class LC_cChannel
{
	LC_Channel	innards;

	LWItemID	m_id;

protected:
	LWItemID	m_parentid;
	char		m_name[256];

	// This is for my base (starting) position ... if I bother to set it
	LC_ObjectPosition	m_base;

	// This is where things are stored during runtime
	AutoArray<LC_Polygon *>			m_polys;	//  1 per poly
	AutoArray<LC_ObjectPosition *>	m_opos;		//  1 per frame
	AutoArray<LC_VertexPosition *>	m_vpos;		//  1 per point
	AutoArray<LW_FrameVertices>		m_vofs;		// (1 per point) per frame

	LC_cChannel(LWItemID id, char *name):
		m_id(id),
		m_parentid(LWITEM_NULL),
		m_polys(0),
		m_opos(0),
		m_vpos(0),
		m_vofs(LW_FrameVertices::INVALID)
	{
		strcpy(m_name,name);
		memset(&m_base,0,sizeof(m_base));
	}

	~LC_cChannel()
	{
	}

public:
	// Function for pulling out struct for plugins
	LC_Channel *getInnards()
	{
		memset(&innards,0,sizeof(innards));

		innards.data = (void *)this;
		innards.name = (const char *)getName();
		innards.flavor = getFlavor();
		innards.lwid = getID();
		innards.lwparentid = getParentID();
		innards.vertexcount = getVertexCount();
		innards.polygoncount = getPolygonCount();

		return &innards;
	}

	// Functions for feeling out what this is
	virtual LC_FLAVOR getFlavor()		= 0;

	// Get functions
	LWItemID getID()				{return m_id;}
	LWItemID getParentID()			{return m_parentid;}
	char *getName()					{return m_name;}

	unsigned int		getVertexCount()	{return m_vpos.Next();}
	LC_VertexPosition  *getVertex(int idx)	{return m_vpos[idx];}

	unsigned int getPolygonCount()		{return m_polys.Next();}
	LC_Polygon	*getPolygon(int idx)	{return m_polys[idx];}

	LC_VertexOffset	   *getVertexAtFrame(int vtxidx, int frame)
	{
		return &(m_vofs[frame][vtxidx].innards);
	}

	LC_ObjectPosition	*getObjectAtFrame(int frame)
	{
		return m_opos[frame];
	}

	LC_ObjectPosition	*getObjectAtRest()	{return &m_base;}

	int operator == ( const LC_cChannel& rhs) const
	{
		return m_id == rhs.m_id;
	}
};

#endif //_LW_LWCHANNEL_H
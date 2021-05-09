/**************************************
 *
 *  ListenChannel.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Channel definition class
 *  Base class and one for each of the available flavors
 *
 **************************************/

#ifndef _LW_LISTENCHANNEL_H
#define _LW_LISTENCHANNEL_H

#include <stdio.h>
#include <stdlib.h>
#include "string.h"

#include "lwrender.h"
#include "lwdisplce.h"
#include "lwmeshes.h"
#include "lwsurf.h"
#include "lwserver.h"
#include "lwpanel.h"
#include "lwmotion.h"

#include "sys_base.h"
#include "ExportObj.h"

#include "LwChannel.h"

// This is the channel listener object - you get one of these for each channel
// that each plugin is listening for.  It simply represents what it is we
// are listening for
class LWEntXform
{
	class _Xform
	{
	public:
		void 			*id_source;
		int				id_target;

		static _Xform INVALID;

		_Xform() :
			id_source((void *)0),
			id_target(-1)
		{ ;}
	};

	// These help us keep things in order ...
	AutoArray<_Xform>	m_pnts;
	AutoArray<_Xform>	m_polys;

	// Check them all = m_la
	// Start from either where we are told to start, or from the max
	int findIdx(AutoArray<_Xform>& ar, void *idx, int last)
	{	// If input doesn't care, start at the last one we found, -1
		if (last < 0)
			last = ar.Next();

		while (last-- > 0)
		{
			if (ar[last].id_source == idx)
				break;
		}

		return last;
	}

	int add(AutoArray<_Xform>& ar, void *idx)
	{
		int i = ar.Next();
		ar[i].id_target = i;
		ar[i].id_source = idx;

		return i;
	}

public:
	LWEntXform():
	  m_pnts(_Xform::INVALID),
	  m_polys(_Xform::INVALID)
	{;}

	// Poly/Point 
	int add(LWPntID idx)	{	return add(m_pnts,idx);}
	int add(LWPolID idx)	{	return add(m_polys,idx);}

	int getIndex(LWPntID idx, int last = -1)	{	return findIdx(m_pnts,idx,last);}
	int getIndex(LWPolID idx, int last = -1)	{	return findIdx(m_polys,idx,last);}
};

/*
 * Sub-class for all the available chanel types
 */
class ListenChannel : public LC_cChannel
{
protected:
	int			m_activeframe;
	LWTime		m_activetime;

	// These help us keep things in order ...
	LWEntXform		m_pntXform;
	LWEntXform		m_polyXform;

	int validatePointVMAPs(LWPntID pnt, LWFVector uv, LWFVector weight);
	int validatePolyVMAPs(LWPolID pol, LWPntID pnt, LWFVector uv, LWFVector weight);

	ListenChannel(LWItemID id, char *name);

	// scanning functions
	static int scanPntsForFill(void *v, LWPntID pnt);	// ListenChannel
	static int scanPolysForFill(void *v, LWPolID pol);	// ListenChannel

	void	*vScratch;

public:
	ListenChannel();
	virtual ~ListenChannel();

	virtual void initialize(LWMeshInfoID mi);
	void setActiveFrame(int i, LWTime t)
	{	m_activeframe = i; m_activetime = t;	}

// Functions to be overridden
	// Functions for feeling out what this is
	virtual LC_FLAVOR getFlavor()		{ return LISTEN_NULL;}
	virtual char *getMapName()			{ return 0; }
	virtual LWID getMapType()			{ return (LWID)0; }

	// setup functions
	virtual void addPoly(LWPolID poly);
	virtual void addPoint(LWPntID pnt)
	{
		LWFVector uv = {0,0,0};
		LWFVector weight = {1,1,1};

		addPoint(pnt,uv,weight);
	}
	void addPoint(LWPntID pnt, LWFVector uv, LWFVector weight);

	// Construction functions
	virtual void processVertex(const LWDisplacementAccess *da);
	virtual void processMotion(const LWItemMotionAccess *ma)	{;}
};

// TAG channels
class ListenTAGChannel : public ListenChannel
{
public:
	ListenTAGChannel(LWItemID id, char *name):
		ListenChannel(id, name)
	{;}

// Function overrides
	// Functions for feeling out what this is
	LC_FLAVOR getFlavor()					{ return LISTEN_TAG; }

	// setup functions
	void addPoly(LWPolID poly);
	void addPoint(LWPntID pnt)
	{
		if (m_vpos.Next() < 3)
		{
			ListenChannel::addPoint(pnt);
		}
	}
};

// POLY watching channels
class ListenPOLYChannel : public ListenChannel
{
public:
	ListenPOLYChannel(LWItemID id, char *name):
		ListenChannel(id, name)
	{;}

// Function overrides
	// Functions for feeling out what this is
	LC_FLAVOR getFlavor()		{ return LISTEN_POLYS; }
	char *getMapName()			{ return getName(); }
	LWID getMapType()			{ return LWVMAP_TXUV; }

	// Pass through setup functions
//	void addPoly(LWPolID poly);
	void addPoint(LWPntID pnt)
	{
		LWFVector uv = {0,0,0};
		LWFVector weight = {1,1,1};

		// If we don't care ... well, don't care!
		if (validatePointVMAPs(pnt, uv, weight) == 0) {
			return;
		}

		ListenChannel::addPoint(pnt,uv,weight);
	}

	// Construction functions
	void processFrame(LWMeshInfoID mi);
};

// BONE and WEIGHT map watching channels
class ListenBONEChannel : public ListenChannel
{
	char m_map[256];

public:
	ListenBONEChannel(LWItemID id, char *name, char *map = 0):
		ListenChannel(id, name)
	{
		m_map[0] = 0;
		if (map)
			strcpy(m_map,map ? map : name);
	}
	~ListenBONEChannel();

// Function overrides
	// Need to register ourselves as a server
	void initialize(LWMeshInfoID mi);

	// Functions for feeling out what this is
	LC_FLAVOR getFlavor()		{ return LISTEN_BONE; }
	char *getMapName()			{ return m_map; }
	LWID getMapType()			{ return LWVMAP_WGHT; }

	// Pass through setup functions
//	void addPoly(LWPolID poly);
	void addPoint(LWPntID pnt)
	{
		LWFVector uv = {0,0,0};
		LWFVector weight = {1,1,1};

		// If we don't care ... well, don't care!
		if (validatePointVMAPs(pnt, uv, weight) == 0) {
			return;
		}

		ListenChannel::addPoint(pnt,uv,weight);
	}

	// Construction functions
	void processFrame(LWMeshInfoID mi);
	void processMotion(const LWItemMotionAccess *ma);
};

#include "ObjectWrapper.h"

#endif // _LW_LISTENCHANNEL_H
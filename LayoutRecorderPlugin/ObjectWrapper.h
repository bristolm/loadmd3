/**************************************
 *
 *  ObjectWrapper.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Wraps an internal Lightwave object
 *  Distills surfaces, UV down to a few
 *  different 'channel' types.
 *  Caches animation data from Lightwave
 *
 **************************************/

#ifndef _LW_OBJECTWRAPPER_H
#define _LW_OBJECTWRAPPER_H

#include "ListenChannel.h"

class ObjectWrapper:Cache<ObjectWrapper>
{
	LWItemID m_id;

	// Reference counter
	int		 m_refs;

	// Private so only we can create/delete them
	ObjectWrapper(LWItemID id);

	// This is the list of channels I own
	AutoArray<ListenChannel *>	m_channels;

	// 
	void reset();
	void initialize();

public:
	LWItemID getID()	{return m_id;}

	// Public wrapper handling functions
	void releaseWrapper();
	static ObjectWrapper *aquireWrapper(LWItemID id);
	static ObjectWrapper *findWrapper(LWItemID id);

	unsigned int ChannelCount()	{return m_channels.Next();}
	ListenChannel* getChannel(int idx) {return m_channels[idx];}

	// Lightwave interfacing funtions
	int LWSurfaceCount();
	int LWVertexCount(int surfidx);
	int LWPolygonCount(int surfidx);
	const char		*LWSurfaceName(int idx);
	LWSurfaceID		*LWGetSurfs();

	// Scanning - used during setup
	static int scanPntsForVMap(void *v, LWPntID pnt);	// LWMeshInfoID
	static int scanPolysForVMap(void *v, LWPolID pol);	// LWMeshInfoID

	// Used prior to, and after processing
	static void resetAll();
	static void initializeAll();

	// Processing - used during export
	void beginFrame(LWFrame i, LWTime t);
	void processVertex(LWDisplacementAccess *da);
};



#endif // _LW_OBJECTWRAPPER_H
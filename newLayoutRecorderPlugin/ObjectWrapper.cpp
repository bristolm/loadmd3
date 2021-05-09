/**************************************
 *
 *  ObjectWrapper.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Wraps an internal Lightwave object
 *  Distills surfaces, UV down to a few
 *  different 'channel' types.
 *  Caches animation data from Lightwave
 *
 *  One of these wraps each requested LightWave Object
 *  Takes a LWMeshInfo and distills it into a number of predefined channel flavors
 *  ExportWrappers interface with this to retrieve information for their targets
 *  These are regenerated each time a preview is run - in this way the user
 *  can alter the point orders w/ serious repercussions
 *
 **************************************/

extern "C"
{
#include "LayoutRecorderPlugin.h"
}
#include "ListenChannel.h"

#define			_TMP_BUFF_SIZE	1024
static char		tmp[_TMP_BUFF_SIZE];

// statics
int							Cache<ObjectWrapper>::NextUniqueIndex = 0;
AutoArray<ObjectWrapper *>	Cache<ObjectWrapper>::All(0);

ObjectWrapper::ObjectWrapper(LWItemID id):
	m_id(id),
	m_refs(1),
	m_channels(0)
{;}

void ObjectWrapper::reset()
{
	for (unsigned int i = 0 ;i < m_channels.Next(); i++)
	{
		ListenChannel *lc = m_channels[i];
		if (lc)
			delete lc;
		m_channels[i] = 0;
	}
}

// FIXME:  Need to reselect what was selected ...
void ObjectWrapper::initialize()
{
	// clean out the tag index
	int i = 0;

	// Setup the element array
	int eidx = 0;
	ListenChannel *lc;

	// Grab the mesh info
	LWMeshInfoID mi = LW_objInfo->meshInfo(m_id,0);

	if (mi == 0)
	{
		return;
	}

	// Then my valid "tag_" surfaces	ELEMENT_SURF
	const char *c = LW_objInfo->filename(m_id);
	LWSurfaceID *surfs = LW_surfFuncs->byObject(c);
	for (i = 0; surfs[i] != 0; i++)
	{
		const char *c = LW_surfFuncs->name(surfs[i]);
		if (c && memcmp(c,"tag_",4) == 0) {
			lc = new ListenTAGChannel(m_id,(char *)c);
			m_channels[eidx++] = lc;

			// Fill it in
			lc->initialize(mi);
		}
	}

	// Then the vmaps					ELEMENT_TXUV
	int iMax = LW_objFuncs->numVMaps(LWVMAP_TXUV);
	for (i = 0; i < iMax; i++)
	{	// look for each vmap in this object
		c = LW_objFuncs->vmapName(LWVMAP_TXUV,i);			// Get the name
		void *vmap = mi->pntVLookup( mi, LWVMAP_TXUV, c);	// Grab a ref
		int dim = mi->pntVSelect( mi, vmap );				// Select it

		if (mi->scanPolys(mi,&scanPolysForVMap,mi) ||		// And look for the use of it
			mi->scanPoints(mi,&scanPntsForVMap,mi) )
		{
			lc = new ListenPOLYChannel(m_id,(char *)c);
			m_channels[eidx++] = lc;

			lc->initialize(mi);
		}
	}

	// Then the bones			ELEMENT_BONE
	LWItemID *curBone = (LWItemID *)LW_itemInfo->first(LWI_BONE,m_id);
	for (;curBone != LWITEM_NULL;
		  curBone = (LWItemID *)LW_itemInfo->next(curBone))
	{
//		c = LW_boneInfo->weightMap(curBone);
//		if (!c)
//			c = (char *)LW_itemInfo->name(curBone);
//			continue;

// look for the weightmap in this object .. NO grab them all
//		void *vmap = mi->pntVLookup( mi, LWVMAP_WGHT, c);	// Grab a ref
//		int dim = mi->pntVSelect( mi, vmap );				// Select it

//		if (mi->scanPolys(mi,&scanPolysForVMap,mi) ||		// And look for the use of it
//			mi->scanPoints(mi,&scanPntsForVMap,mi) )
//		{
			lc = new ListenBONEChannel(curBone,
										(char *)LW_itemInfo->name(curBone),
										(char *)LW_boneInfo->weightMap(curBone));
			m_channels[eidx++] = lc;

			lc->initialize(mi);
//		}
	}
}

ObjectWrapper *ObjectWrapper::aquireWrapper(LWItemID id)
{
	ObjectWrapper *lcg = 0;
	int idx = 0;

	for (; idx < getCacheSize(); idx ++) {
		lcg = getByCacheID(idx);
		// Found one, register that someone else cares
		if (lcg && lcg->m_id == id){
			(lcg->m_refs)++;
			return lcg;
		}
	}

	lcg = new ObjectWrapper(id);

	return lcg;
}

void ObjectWrapper::releaseWrapper()
{
	ObjectWrapper *lcg = 0;
	unsigned int idx = 0;

	if (--m_refs == 0) {
		delete this;
	}
}

ObjectWrapper *ObjectWrapper::findWrapper(LWItemID id)
{
	ObjectWrapper *lcg = 0;
	for (int idx = 0; idx < getCacheSize(); idx ++) {
		lcg = getByCacheID(idx);
		if (lcg == 0)
			continue;

		// Found one, register that someone else cares
		if (lcg->m_id == id){
			return lcg;
		}
	}

	return 0;
}

// Static Scanning functions
int ObjectWrapper::scanPntsForVMap(void *v, LWPntID pnt)
{
	static float f[3];

	LWMeshInfoID mi = (LWMeshInfoID)v;

	// Return if this point maps to this VMAP or not
	return (mi->pntVGet(mi,pnt,f));
}

int ObjectWrapper::scanPolysForVMap(void *v, LWPolID pol)
{
	static float f[3];

	LWMeshInfoID mi = (LWMeshInfoID)v;

	// Check 3 points in this polygon
	for (int i = 0; i <3 ; i++)
	{
		LWPntID pnt = mi->polVertex(mi,pol,i);
		if (mi->pntVPGet(mi,pnt,pol,f))
			return 1;
	}

	return 0;
}

// setup/clearout functions
void ObjectWrapper::resetAll()
{
	for (int idx = 0; idx < getCacheSize(); idx ++) {
		ObjectWrapper *lcg = getByCacheID(idx);
		if (lcg)
			lcg->reset();
	}
}
void ObjectWrapper::initializeAll()
{
	for (int idx = 0; idx < getCacheSize(); idx ++) {
		ObjectWrapper *lcg = getByCacheID(idx);
		if (lcg)
			lcg->initialize();
	}
}


// Workhorse functions
void ObjectWrapper::beginFrame(LWFrame iFrame, LWTime tTime)
{
	LWMeshInfoID mi = LW_objInfo->meshInfo(m_id,0);

	// Check each channel
	for (UINT32 pidx = 0; pidx < m_channels.Next(); pidx++)
	{
		ListenChannel *lc = m_channels[pidx];
		if (lc == 0)
			continue;

		lc->setActiveFrame((UINT32)iFrame, tTime);
	}
}

void ObjectWrapper::processVertex(LWDisplacementAccess *da)
{
	// Check each channel
	for (UINT32 pidx = 0; pidx < m_channels.Next(); pidx++)
	{
		ListenChannel *lc = m_channels[pidx];
		if (lc == 0)
			continue;

		lc->processVertex(da);
	}
}

/*
	// 1) get the surface name
     s=meshInfo->polTag(meshInfo,polId,LWPTAG_SURF);

2) lookup the surface
     surfArray=surfFuncs->byName(s,objName);
     surf=surfArray[0];

3) get the texture
     tex=surfFuncs->getTex(surf,SURF_COLR);

4) get first texture layer
     texLayer=texFuncs->firstLayer(tex);

5) get the vmap
     texFuncs->getParam(texLayer,TXTAG_VMAP,&vmapId);

6) select the vmap
     dim=meshInfo->pntVSelect(meshInfo,(void *)vmapId);

7) get texture coordinates
     (for all points in this poly):
     isMapped=meshInfo->pntVGet(meshInfo,pnt,tc);

8) get per-polygon (discontinuous) coordinates   (6.5)
     ismapped = meshInfo->pntVPGet( meshInfo, pnt, poly, tc );

*/

void *getChannelFromGroup(void *inst, int idx)
{
	return ((ObjectWrapper *)inst)->getChannel(idx);
}

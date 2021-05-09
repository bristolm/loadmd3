/**************************************
 *
 *  ListenChannel.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Channel definition class
 *  Given a flavor, differnet information is collected from the provided LWMeshInfo
 *  LISTEN_TAG - records position of 3 points in a single polygon w/ a surface of name tag_*
 *  LISTEN_BONE - records bone positions during animation - also grabs point data for
 *                all vertices effected by an attached WeightMap
 *  LISTEN_MESH - records point positions and polygon layouts for a UV mesh
 *
 **************************************/

extern "C"
{
#include "LayoutRecorderPlugin.h"
}
#include "ListenChannel.h"

#include "math.h"

#define			_TMP_BUFF_SIZE	1024
static char		tmp[_TMP_BUFF_SIZE];

// statics
LWEntXform::_Xform	LWEntXform::_Xform::INVALID =	LWEntXform::_Xform();

LC_cVertexOffset	LC_cVertexOffset::INVALID = LC_cVertexOffset();
LW_FrameVertices	LW_FrameVertices::INVALID = LW_FrameVertices();

//
// ListenChannel functions
//
ListenChannel::ListenChannel()
:LC_cChannel(0,"NULL"),
	m_activeframe(-1),
	vScratch(0)
{;}

ListenChannel::ListenChannel(LWItemID id, char *name)
:LC_cChannel(id,name),
	m_activeframe(-1),
	vScratch(0)
{;}

ListenChannel::~ListenChannel()
{	// Clean out "our" arrays
	unsigned int i = 0;

	for (i = 0; i < m_polys.Next(); i++)
		delete m_polys[i];

	for (i = 0; i < m_opos.Next(); i++)
		delete m_opos[i];

	for (i = 0; i < m_vpos.Next(); i++)
		delete m_vpos[i];

	// m_vofs deletes itself ?
}

void ListenChannel::initialize(LWMeshInfoID mi)
{
	// Fill it in
	if (mi != 0) 
	{
		vScratch = mi;

		// In scanning the polys, we may get the points ?
		mi->scanPolys(mi,&scanPolysForFill,this);
//		mi->scanPoints(mi,&scanPntsForFill,this);
	}
	vScratch = 0;

	m_parentid = (LWItemID)LW_itemInfo->parent(getID());
}

int ListenChannel::scanPntsForFill(void *v, LWPntID pnt)
{	// Stash this point in our points list
	((ListenChannel *)v)->addPoint(pnt);
	return 0;
}

int ListenChannel::scanPolysForFill(void *v, LWPolID pol)
{
	((ListenChannel *)v)->addPoly(pol);
	return 0;
}

int ListenChannel::validatePointVMAPs(LWPntID pnt, LWFVector uv, LWFVector weight)
{
	if (getMapName() == 0)
		return 0;

	int i = 0;
	void *v = 0;
	LWMeshInfoID mi = (LWMeshInfoID)vScratch;

	// Grab UV values in a UV by this name
	if ((v = mi->pntVLookup( mi, getMapType(), getMapName() )) != 0
		&& (i = mi->pntVSelect( mi, v )) != 0)
	{
		if (mi->pntVGet(mi,pnt,
			(getMapType() == LWVMAP_WGHT) ? weight : uv) > 0)
			return 1;
	}

	return 0;
}

void ListenChannel::addPoint(LWPntID pnt, LWFVector uv, LWFVector weight)
{
	LWFVector pos = {1,1,1};

	LWMeshInfoID mi = (LWMeshInfoID)vScratch;

	// Find our index for this point, stash the UV value
	int bValid = m_pntXform.add(pnt);

	// Fill in the appropriate point in our little collector
	LC_VertexPosition *vp = m_vpos[bValid];
	if (vp != 0) {
		return;
	}

	// Build the new one
	vp = new LC_VertexPosition;
	vp->lwid = pnt;
	vp->index = bValid;

	vp->texmap[0] = uv[0];
	vp->texmap[1] = uv[1];

	vp->influence = weight[0];

	mi->pntBasePos(mi,pnt,pos);
	vp->start.location[0] = pos[0];
	vp->start.location[1] = pos[1];
	vp->start.location[2] = pos[2];

	// stash it
	m_vpos[bValid] = vp;
}

int ListenChannel::validatePolyVMAPs(LWPolID pol, LWPntID pnt, LWFVector uv, LWFVector weight)
{
	if (getMapName() == 0)
		return 0;

	void *v = 0;
	int i = 0;
	LWMeshInfoID mi = (LWMeshInfoID)vScratch;

	if ((v = mi->pntVLookup( mi, getMapType(), getMapName() )) != 0
		&& (i = mi->pntVSelect( mi, v )) != 0)
	{
		if (mi->pntVPGet(mi,pnt,pol,
			(getMapType() == LWVMAP_WGHT) ? weight : uv) > 0)
			return 1;
	}

	return 0;
}

void ListenChannel::addPoly(LWPolID pol)
{
	LWFVector uv = {0,0,0};
	LWFVector weight = {1,1,1};

	int bValid = 0;
	int i;
	LC_Polygon *lp = 0;

	LWMeshInfoID mi = (LWMeshInfoID)vScratch;

	lp = new LC_Polygon;

	// Look for a valid VMAP for each point
	for (int k = 0; k <3 ; k++)
	{	// Check the 3 points and build this polygon by finding the IDs
		if (mi->polSize(mi,pol) != 3)
		{
			delete lp;
			return;
		}

		LWPntID pnt = mi->polVertex(mi,pol,k);
		i = -1;

		// See if this might be a UV mapped poly (VMAD or VMAP)
		LC_VertexPosition *vp = 0;
		if (validatePolyVMAPs(pol,pnt,uv,weight) != 0 ||
			validatePointVMAPs(pnt,uv,weight) != 0		)
		{	// This IS a UV poly we care about - see if we've already accounted for the point
			while ((i = m_pntXform.getIndex(pnt, i)) > -1) 
			{	// It is possible that this is another UV position for
				// this same point - check if the UV's match - if they do,
				// then this is the same point, and use that index instead
				vp = m_vpos[i];
				if (vp->texmap[0] == uv[0]
					&& vp->influence == weight[0]
					&& fabs(vp->texmap[1] - uv[1]) < 0.0001)
					// Same point - use it
					break;
			}
			if (i < 0)
			{	// This is a new point
				// Add the point, stash the UV value
				i = m_pntXform.add(pnt);

				// Build the new one
				vp = new LC_VertexPosition;
				vp->lwid = pnt;
				vp->index = i;

				vp->texmap[0] = uv[0];
				vp->texmap[1] = uv[1];

				vp->influence = weight[0];
				
				LWFVector pos = {1,1,1};
				mi->pntBasePos(mi,pnt,pos);
				vp->start.location[0] = pos[0];
				vp->start.location[1] = pos[1];
				vp->start.location[2] = pos[2];

				// stash it
				m_vpos[i] = vp;
			}

			lp->vertex[k] = i;
		}

		// If the point is nothing we want, skip the poly
		if (i < 0)
		{
			delete lp;
			return;
		}
	}

	// Add the new polygon into the mesh
	// Right now we really don't care about these polys - just the points in them
	i = m_polyXform.add(pol);
	const char *c = mi->polTag(mi,pol,LWPTAG_SURF);

	lp->lwid = pol;
	lp->surf = (LW_surfFuncs->byName(c,0))[0];
	lp->index = i;
	m_polys[i] = lp;
}

void ListenChannel::processVertex(const LWDisplacementAccess *da)
{
	if (m_activeframe < 0) 
		return;

	//stash the position for each copy of this point that
	//we are monitoring
	int vidx = -1;
	int voffidx = -1;

	while ((vidx = m_pntXform.getIndex(da->point,vidx)) > -1)
	{
		// stash the position
		LC_VertexPosition	*vp = m_vpos[vidx];

		// Setup the offset spot
		LC_VertexOffset		*vo = getVertexAtFrame(vp->index,m_activeframe);
		vo->location[0] = (float)da->source[0];
		vo->location[1] = (float)da->source[1];
		vo->location[2] = (float)da->source[2];
	}
}

//
// ListenTAGChannel functions
//
void ListenTAGChannel::addPoly(LWPolID pol)
{
	// Just the first poly for Tags
	if (m_polys.Next() > 0) {
		return;
	}

	// Look to see if this Surface is valid 
	LWMeshInfo *mi = ((LWMeshInfo *)vScratch);
	const char *c = mi->polTag(mi,pol,LWPTAG_SURF);
	if (c && strcmp(c,getName()) != 0)
		return;

	int j = m_polyXform.add(pol);

	LC_Polygon *lp = new LC_Polygon;
	lp->index = j;
	lp->surf = (LW_surfFuncs->byName(c,0))[0];
	lp->lwid = pol;
	for (int k = 0; k <3 ; k++)
	{	// Check the 3 points and build this polygon by finding the IDs
		LC_VertexPosition *vp = 0;
		LWPntID pnt = mi->polVertex(mi,pol,k);
		addPoint(pnt);

		// Reference it in the polygon
		lp->vertex[k] = k;
	}
	m_polys[j] = lp;
}

//
// ListenPOLYChannel functions
//

//
// ListenBONEChannel functions
//
void ListenBONEChannel::initialize(LWMeshInfoID mi)
{
	// add a plugin to track me
	char cmdbuf[256];
	sprintf(cmdbuf,"SelectItem %x",getID());
	LW_cmdFunc(cmdbuf);
	sprintf(cmdbuf,"ApplyServer %s %s",LWITEMMOTION_HCLASS,MRB_HIDDEN_MOTIONNAME);
	LW_cmdFunc(cmdbuf);

	// Now do the regular stuff
	ListenChannel::initialize(mi);

	// Set up my base position
	double	d[3];
	// Get my position - relative to parent
	LW_boneInfo->restParam(getID(),LWIP_POSITION,d);
	m_base.xyz[0] = (float)d[0];
	m_base.xyz[1] = (float)d[1];
	m_base.xyz[2] = (float)d[2];

	// Get my orientation - relative to parent
	LW_boneInfo->restParam(getID(),LWIP_ROTATION,d);
	m_base.hpb[0] = (float)d[0];
	m_base.hpb[1] = (float)d[1];
	m_base.hpb[2] = (float)d[2];
}

ListenBONEChannel::~ListenBONEChannel()
{
	// Find the server and delete it
	int idx = 1;			// Base 1 index
	const char *c = 0;
	for (;(c = LW_itemInfo->server(getID(),LWITEMMOTION_HCLASS,idx)) != 0;idx++)
	{
		if (strcmp(c,MRB_HIDDEN_MOTIONNAME) == 0)
		{
			// Clear out the server
			char cmdbuf[255];
			sprintf(cmdbuf,"SelectItem %x",getID());
			LW_cmdFunc(cmdbuf);
			sprintf(cmdbuf,"RemoveServer %s %x",LWITEMMOTION_HCLASS,idx);
			LW_cmdFunc(cmdbuf);
			break;
		}
	}
}

void ListenBONEChannel::processMotion(const LWItemMotionAccess *ma)
{
	double	d[3];

	// For each frame we track the position and orientation of the bones
	LC_ObjectPosition *lo = new LC_ObjectPosition;
	m_opos[m_activeframe] = lo;

	// Get my position - relative to parent
	ma->getParam(LWIP_POSITION, m_activetime, d);
	lo->xyz[0] = (float)d[0];
	lo->xyz[1] = (float)d[1];
	lo->xyz[2] = (float)d[2];

	// Get my orientation - relative to parent
	ma->getParam(LWIP_ROTATION, m_activetime, d);
	lo->hpb[0] = (float)d[0];
	lo->hpb[1] = (float)d[1];
	lo->hpb[2] = (float)d[2];
}


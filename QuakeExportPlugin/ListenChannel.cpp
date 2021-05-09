extern "C"
{
#include "ExportQuakeMDPlugin.h"
}
#include "ListenChannel.h"

#define			_TMP_BUFF_SIZE	1024
static char		tmp[_TMP_BUFF_SIZE];

// statics
AutoArray<ListenChannelGroup *>	ListenChannelGroup::ActiveGroups(0);

LWEntXform::_Xform	LWEntXform::_Xform::INVALID =	LWEntXform::_Xform();

LC_cVertexOffset	LC_cVertexOffset::INVALID = LC_cVertexOffset();
LW_FrameVertices	LW_FrameVertices::INVALID = LW_FrameVertices();

//
// ListenChannel functions
//

ListenChannel::ListenChannel(LWItemID id, char *name):
	m_id(id),
	m_activeframe(-1),
	m_polys(0),
	m_opos(0),
	m_vpos(0),
	m_vofs(LW_FrameVertices::INVALID),
	vScratch(0)
{
	strcpy(Name,name);
}

void ListenChannel::_Setup(LWMeshInfoID mi)
{
	// Fill it in
	if (mi != 0) 
	{
		vScratch = mi;

		mi->scanPoints(mi,&scanPntsForFill,this);
		mi->scanPolys(mi,&scanPolysForFill,this);
	}
	vScratch = 0;
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
	int bValid = 0;
	int i = 0;
	void *v = 0;
	LWMeshInfoID mi = (LWMeshInfoID)vScratch;

	if ((v = mi->pntVLookup( mi, LWVMAP_TXUV, Name )) != 0
		&& (i = mi->pntVSelect( mi, v )) != 0)
	{
		if (mi->pntVGet(mi,pnt,uv) > 0)
			bValid = 1;
	}

	if ((v = mi->pntVLookup( mi, LWVMAP_WGHT, Name )) != 0
		&& (i = mi->pntVSelect( mi, v )) != 0)
	{
		if (mi->pntVGet(mi,pnt,weight) > 0)
			bValid = 1;
	}

	return bValid;
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
	vp->texmap[1] = 1.0f - uv[1];

	vp->influence = weight[0];

	mi->pntBasePos(mi,pnt,pos);
	vp->location[0] = pos[0];
	vp->location[1] = pos[1];
	vp->location[2] = pos[2];

	// stash it
	m_vpos[bValid] = vp;
}

int ListenChannel::validatePolyVMAPs(LWPolID pol, LWPntID pnt, LWFVector uv, LWFVector weight)
{
	int bValid = 0;
	void *v = 0;
	int i = 0;
	LWMeshInfoID mi = (LWMeshInfoID)vScratch;

	if ((v = mi->pntVLookup( mi, LWVMAP_TXUV, Name )) != 0
		&& (i = mi->pntVSelect( mi, v )) != 0)
	{
		if (mi->pntVPGet(mi,pnt,pol,uv) > 0)
			bValid = 1;
	}

	if ((v = mi->pntVLookup( mi, LWVMAP_WGHT, Name )) != 0
		&& (i = mi->pntVSelect( mi, v )) != 0)
	{
		if (mi->pntVPGet(mi,pnt,pol,weight) > 0)
			bValid = 1;
	}

	return bValid;
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
		LC_VertexPosition *vp = 0;
		LWPntID pnt = mi->polVertex(mi,pol,k);
		i = -1;

		// See if this might be a UV mapped poly (VMAD or VMAP)
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
					&& fabs(vp->texmap[1] - 1.0f + uv[1]) < 0.0001)
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
				vp->texmap[1] = 1.0f - uv[1];

				vp->influence = weight[0];
				
				LWFVector pos = {1,1,1};
				mi->pntBasePos(mi,pnt,pos);
				vp->location[0] = pos[0];
				vp->location[1] = pos[1];
				vp->location[2] = pos[2];

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

	lp->lwid = pol;
	lp->index = i;
	m_polys[i] = lp;
}

void ListenChannel::processVertex(LWDisplacementAccess *da)
{
	if (m_activeframe < 0)
		return;

	//stash the position for each copy of this point we are monitoring
	int vidx = -1;
	int voffidx = -1;

	while ((vidx = m_pntXform.getIndex(da->point,vidx)) > -1)
	{
		// stash the position
		LC_VertexPosition	*vp = m_vpos[vidx];

		// Setup the offset spot
		LC_VertexOffset		*vo = getVertexAtFrame(vp->index,m_activeframe);
		vo->location[0] = da->source[0];
		vo->location[1] = da->source[1];
		vo->location[2] = da->source[2];
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
	const char *c = objInfo->filename(m_id);
	LWSurfaceID *surfs = (LWSurfaceID *)surfFuncs->byObject(c);
	for (int i = 0; surfs[i] != 0; i++)
	{
		c = surfFuncs->name(surfs[i]);
		if (c && strcmp(c,Name) != 0)
			continue;

		int j = m_polyXform.add(pol);

		LC_Polygon *lp = new LC_Polygon;
		lp->index = j;
		lp->lwid = pol;
		lp->vertex[0] = 0;
		lp->vertex[1] = 1;
		lp->vertex[2] = 2;
		m_polys[j] = lp;

		break;
	}
}

// Process the beginning of a frame - this is when we get
// object orientations
void ListenTAGChannel::processFrame(LWMeshInfoID mi)
{
	if (m_activeframe < 0)
		return;

	LC_VertexPosition	*vp = 0;
	LC_VertexOffset		*vo = 0;
	LC_ObjectPosition	*op = 0;
	int i = 0;

	LWFVector			fpos = {0,0,0};

	// Get the position of each of the 3 points in our polygon
	for (i = 0; i < 3; i++)
	{
		vp = m_vpos[i];
		mi->pntOtherPos(mi,vp->lwid,fpos);

		vo = getVertexAtFrame(i,m_activeframe);

		op = m_opos[i];
		if (op == 0) {
			op = new LC_ObjectPosition;
			m_opos[i] = op;
		}

		// Infer the object's location during this frame
		op->location[0] = vo->location[0] = fpos[0];
		op->location[1] = vo->location[1] = fpos[1];
		op->location[2] = vo->location[2] = fpos[2];
	}
}

//
// ListenPOLYChannel functions
//
void ListenPOLYChannel::processFrame(LWMeshInfoID mi)
{	// Nothing really to do here, we
	// did all the setup previously
}

//
// ListenChannelGroup functions
//
LWSurfaceID *ListenChannelGroup::LWGetSurfs()
{	// get the filename ...
	const char *c = objInfo->filename(m_id);

	// Then the surfaces
	return surfFuncs->byObject(c);
}

int ListenChannelGroup::LWSurfaceCount()
{	// get the filename ...
	LWSurfaceID *s = LWGetSurfs();
	int i = 0;
	while (s[i] != 0){
		++i;
	}

	return i;
}

const char *ListenChannelGroup::LWSurfaceName(int idx)
{	// return the name of the surface at idx N
	LWSurfaceID *s = LWGetSurfs();

	return surfFuncs->name(s[idx]);
}

/*
 * Simply represents an instance of an object that bridges the
 * gap between the obj and the MD3 file.  This is used to push the
 * deformation into the MD3 at the right place, and it is also 
 * used to setup the buttons and whatnot on the Displacement Panel
 */
LWPntID			pntIndex[3][100];

ListenChannelGroup::ListenChannelGroup(LWItemID id):
	m_id(id),
	m_refs(1),
	m_channels(0),
	iScratch(0)
{
	sScratch[0] = 0;
	// clean out the tag index
	int i = 0;

	// Setup the element array
	int eidx = 0;
	ListenChannel *lc;

	// Grab the mesh info
	LWMeshInfoID mi = objInfo->meshInfo(m_id,0);

	if (mi == 0)
	{
		return;
	}

	// Then my valid "tag_" surfaces	ELEMENT_SURF
	const char *c = objInfo->filename(m_id);
	LWSurfaceID *surfs = surfFuncs->byObject(c);
	for (i = 0; surfs[i] != 0; i++)
	{
		const char *c = surfFuncs->name(surfs[i]);
		if (c && memcmp(c,"tag_",4) == 0) {
			lc = new ListenTAGChannel(m_id,mi,(char *)c);

			// Fill it in
			lc->vScratch = mi;
			m_channels[eidx++] = lc;
		}
	}

	// Then the vmaps					ELEMENT_TXUV
	int iMax = objFuncs->numVMaps(LWVMAP_TXUV);
	for (i = 0; i < iMax; i++)
	{	// look for each vmap in this object
		c = objFuncs->vmapName(LWVMAP_TXUV,i);				// Get the name
		void *vmap = mi->pntVLookup( mi, LWVMAP_TXUV, c);	// Grab a ref
		int dim = mi->pntVSelect( mi, vmap );				// Select it

		if (mi->scanPolys(mi,&scanPolysForVMap,mi) ||		// And look for the use of it
			mi->scanPoints(mi,&scanPntsForVMap,mi) )
		{
			lc = new ListenPOLYChannel(m_id,mi,(char *)c);
			m_channels[eidx++] = lc;
		}
	}

	// Then the weightmaps				ELEMENT_BONE

	// Thread me into the main list
	ActiveGroups[ActiveGroups.Next()] = this;
}


ListenChannelGroup::~ListenChannelGroup()
{
	// Clear me out of the main group list
	for (unsigned int idx = 0; idx < ActiveGroups.Next(); idx++) {
		if (ActiveGroups[idx] == this) {
			ActiveGroups[idx] = 0;
			break;
		}
	}
	ActiveGroups.Reduce();
}

// Static Scanning functions
int ListenChannelGroup::scanPntsForVMap(void *v, LWPntID pnt)
{
	static float f[3];

	LWMeshInfoID mi = (LWMeshInfoID)v;

	// Return if this point maps to this VMAP or not
	return (mi->pntVGet(mi,pnt,f));
}

int ListenChannelGroup::scanPolysForVMap(void *v, LWPolID pol)
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

// Workhorse functions
void ListenChannelGroup::BeginFrame(LWFrame iFrame, LWTime tTime)
{
	LWMeshInfoID mi = objInfo->meshInfo(m_id,0);

	// Check each channel
	for (UINT32 pidx = 0; pidx < m_channels.Next(); pidx++)
	{
		ListenChannel *lc = m_channels[pidx];
		if (lc == 0)
			continue;

		lc->setActiveFrame((UINT32)iFrame, tTime);
		lc->processFrame(mi);
	}
}

void ListenChannelGroup::ProcessVertex(LWDisplacementAccess *da)
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
 * A Block looks like this:
 *	Plugin DisplacementHandler 2 MRB::QuakeMD_MeshScraper
 *	{ ChannelList
 *    ListenChannel [Flavor] [ChannelName] [LW name]
 *	}
 */
#define ID_CHNL  LWID_( 'C','H','N','L' )

static LWBlockIdent idmain[] = {
  ID_CHNL, "ChannelList",
  0
};

#define ID_LSNC  LWID_( 'L','S','N','C' )

static LWBlockIdent idelement[] = {
  ID_LSNC, "ListenChannel",
  0
};

// Save to file
LWError ListenChannelGroup::Save(const LWSaveState *save)
{
	long l[2] = {0,0};
	// Start this block
	LWSAVE_BEGIN( save, &idmain[ 0 ], 0 );

	// Dump data for each Channel
	short sTmp = 0;
	for (unsigned int i = 0; i < m_channels.Next(); i ++)
	{
		ListenChannel *lc = m_channels[i];
		if (lc == 0) {
			continue;
		}

		l[0] = 1; //(long)(lc->IsListening);

		LWSAVE_BEGIN( save, &idelement[ 0 ], 1 );
		 LWSAVE_I4(save, (const long*)l, 1);
		 LWSAVE_STR(save, lc->Name);
//		 LWSAVE_STR(save, lc->TargetName);
		LWSAVE_END( save );
	}

	LWSAVE_END( save );

	return (LWError)0;
}

// Load from file
static int itmp = 0;
LWError ListenChannelGroup::Load(const LWLoadState *load)
{
	long l[2] = {0,0};
	ListenChannel *lc = 0;
	unsigned int i = 0;
	LWID id = LWLOAD_FIND( load, idmain);
	char tmp[_TMP_BUFF_SIZE];

	while ( id = LWLOAD_FIND( load, idelement )) {
	  LWLOAD_I4(load, l, 1);
	  LWLOAD_STR(load,tmp,_TMP_BUFF_SIZE);

	  // Find and Update the names (the Create should have prepped everything)
	  for (i = 0; i < m_channels.Next(); i++)
	  {
		  lc = m_channels[i];
		  if (strcmp(lc->Name,tmp) == 0) {
//			LWLOAD_STR(load,lc->TargetName,_TMP_BUFF_SIZE);

			break;
		  }
	  }
	  LWLOAD_END( load );
	}
	LWLOAD_END( load );

	return (LWError)0;
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

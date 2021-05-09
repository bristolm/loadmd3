/* Layout include file */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "lwxpanel.h"

/* Mystuff */
extern "C"
{
#include "LayoutExportMD2.h"
}
#include "lw_base.h"
#include "sys_base.h"
#include "sys_extra.h"
#include "ParseMD2.h"

#include "ModelScratchPad.h"

// extra lightwave files for geting the images
#include "lwrender.h"		// For Object Info, Scene Info
#include "lwmeshes.h"		// for Mesh info
#include "lwsurf.h"			// for Surface info
#include "lwtxtr.h"			// for Texture info
#include "lwimage.h"		// for Image List

class PolygonNormal
{
	Vector<float>	normal;
	int				numadded;

	int				added[3];
	Vector<float>	vecs[3];

public:
	PolygonNormal():
		normal(),
		numadded(0)
	{
		for (int i = 0; i < 3; i++)
		{
			added[i] = 0;
			vecs[i] = Vector<float>();
		}
	}

	void setVector(int slot, Vector<float>& v)
	{	// If this slot has already been populated, ignore this
		if (added[slot] != 0)
			return;
		added[slot] = 1;

		vecs[slot] += v;
		if (++numadded < 3)
			return;

		// Done adding - Calculate the Normal
		Matrix<float>	M(vecs[0],vecs[1],vecs[2]);
		normal = M.getNormal();
	}

	const Vector<float>& getNormal()			{return normal;}

	static PolygonNormal	INVALID;
};

class VertexToPolygonMap
{	
	class PolygonMap
	{
	public:
		UINT32	index;		// Index of the polygon we effect
		UINT32	slot;		// 0,1,2 - our point index in that poly

		PolygonMap(UINT32 i = 0, UINT32 s = 0):
			index(i),
			slot(s)
		{;}

		static PolygonMap INVALID;
	};

	// Unique Identifier
	LWPntID					id;

	AutoArray<PolygonMap>	targets;

public:
	VertexToPolygonMap(LWPntID pntid = 0):
		id(pntid),
		targets(PolygonMap::INVALID)
	{
	}

	// Add a reference to the polygons we participate in
	void add(UINT32 idx, UINT32 slot)
	{
		targets[targets.Next()] = PolygonMap(idx,slot);
	}

	// Figure out our normal vector from polygons we reference
	Vector<float> getNormal(AutoArray<PolygonNormal> &polys)
	{
		// Add up normals in our referenced polygons, divide, return
		Vector<float>	 Vtmp;
		for (unsigned int i = targets.Next(); i > 0; i--)
		{
			Vtmp += polys[targets[i -1].index].getNormal();
		}
		Vtmp /= targets.Next();

		Vector<float>	 Vret = LW_TO_MD2_Coords * Vtmp;
//		RotateByMatrix(Vtmp,LW_TO_MD2_Coords,Vret);

		return Vret;
	}

	// Push our location into the polygons
	void set(Vector<float> &vec, AutoArray<PolygonNormal> &polys)
	{
		for (unsigned int i = targets.Next(); i > 0; i--)
		{
			PolygonMap map = targets[i -1];
			polys[map.index].setVector(map.slot,vec);
		}
	}

	// Needed for 'find' in the AutoArray
	int operator==( const VertexToPolygonMap& rhs)
	{
		return (id == rhs.id);
	}

	static VertexToPolygonMap			INVALID;
};

PolygonNormal					PolygonNormal::INVALID = PolygonNormal();
VertexToPolygonMap				VertexToPolygonMap::INVALID = VertexToPolygonMap();
VertexToPolygonMap::PolygonMap	VertexToPolygonMap::PolygonMap::INVALID = PolygonMap();

// Image name finding function
void findImages(LC_Channel *lc, MD2 &model, char *dirpref, const char *surfflag)
{
	// Check for Skins - just the first point on the first poly ...
	LC_Polygon *lp = LW_xprtFuncs->get_polygon(lc,0);
	if (lp == 0)
	{
		return;
	}

	// Only accept Alpha maps (transparency) or color base maps
	LWObjectInfo *objInfo		= (LWObjectInfo *)LW_globalFuncs( LWOBJECTINFO_GLOBAL, GFUSE_TRANSIENT );
	LWSurfaceFuncs *surfFuncs	= (LWSurfaceFuncs *)LW_globalFuncs( LWSURFACEFUNCS_GLOBAL, GFUSE_TRANSIENT );
	LWTextureFuncs *texFuncs	= (LWTextureFuncs *)LW_globalFuncs( LWTEXTUREFUNCS_GLOBAL, GFUSE_TRANSIENT );
	LWImageList  *imgList		= (LWImageList  *)LW_globalFuncs( LWIMAGELIST_GLOBAL, GFUSE_TRANSIENT );

	LWMeshInfoID lwmesh = objInfo->meshInfo(lc->lwid,0);
	LWSurfaceID lwsurfid = lp->surf;

	// 
	LWTextureID lwtex = surfFuncs->getTex(lwsurfid,surfflag);
	if (lwtex == 0)
		return;
	for (LWTLayerID lwlay = texFuncs->firstLayer(lwtex);
		 lwlay;
		 lwlay = texFuncs->nextLayer(lwtex,lwlay))
	{	// Look for images ...
		int itype = texFuncs->layerType(lwlay);
		if (itype != TLT_IMAGE)
			continue;

		// ... with a UV projection type ...
		itype = 0;
		texFuncs->getParam(lwlay, TXTAG_PROJ, &itype);
		if (itype != TXPRJ_UVMAP)
			continue;

		// ... and a map that exists for this mesh ...
		void *lwvmap = 0;
		texFuncs->getParam(lwlay, TXTAG_VMAP, &lwvmap);
		if (lwvmap == 0
			|| lwvmap != lwmesh->pntVLookup(lwmesh, LWVMAP_TXUV, lc->name))
			continue;

		// ... on one of our polygon's point ...
		LWFVector uv = {0,0,0};
		itype = lwmesh->pntVSelect(lwmesh,lwvmap);
		LWPntID lwpnt = lwmesh->polVertex(lwmesh,lp->lwid,0);
		if (lwmesh->pntVPGet(lwmesh,lwpnt,lp->lwid,uv) == 0
			&& lwmesh->pntVGet(lwmesh,lwpnt,uv) == 0)
			continue;

		// wow, this one actually matches!  Grab the image and add it as a skin
		LWImageID *lwimageid = 0;
		texFuncs->getParam(lwlay, TXTAG_IMAGE, &lwimageid);
		const char *lwimage = imgList->name(lwimageid);
		sprintf(tmp,"%s%s%s",dirpref,DIR_SEPARATOR,lwimage);
		int charidx = 0;
		for (charidx = 0;tmp[charidx];charidx++)
		{
			if (tmp[charidx] == '\\')
				tmp[charidx] = '/';
		}

		// Make sure the extension in in lowercase
		for (--charidx ;charidx > 0 && tmp[charidx] != '.'; charidx--)
		{
			if (tmp[charidx] <= 'Z' && tmp[charidx] >= 'A')
			{
				tmp[charidx] = tmp[charidx] - 'A' + 'a';
			}
		}

		unsigned int skinidx = 0;
		for (;skinidx < model.SkinCount();skinidx++)
		{
			if (strcmp(tmp,model.Skin(skinidx).Name) == 0)
				 break;
		}

		MD2_Skin& skin = model.Skin(skinidx);
		if (skin.Name[0] == 0)
			strcpy(skin.Name,tmp);
	}
}

// Merely the MD2 construction functions 
static LC_Model init(	LC_Instance		userinst,
						LC_FrameLoop	**loops[])	// List of frameloops we're spitting out
{
	ModelScratchPad *m = new ModelScratchPad(*loops);

	// Add in some stuff from the base one
	*m = *(ModelScratchPad *)userinst;
	if (m->SaveHeader)
	{	// Write out the cfg file
		m->dumpHeaderFile();
	}

	// If we're not saving the model, don't bother rendering
	if (!m->SaveModel)
	{
		*loops = 0;
	}

	return (LC_Model)m;
}

static void build(LC_Model		mdl,			// current 'in progress' model
				  LWInstance	inst )			// opaque pointer to server's data for this model
{
	ModelScratchPad	*scratch = (ModelScratchPad *)mdl;
	if (scratch->SaveModel == 0)
		return;

	MD2&			model	= scratch->Model;

	// Skin skin sizes
	model.Skinwidth(scratch->SkinWidth);
	model.Skinheight(scratch->SkinHeight);

	LC_FrameLoop	**loops	= scratch->Loops;
	LC_Channel *lc = 0;

	// We've got a list of Channels - walk it and seed what we can
	for (unsigned int i = 0;
			lc = (LC_Channel *)LW_xprtFuncs->get_channel(inst,i);
			i++)
	{
		if (lc == 0)
			break;

		// Set up some arrays for finding the vertex 'normal'
		// 1)  Convert an LWPntID to an integer index (base 0) ...
		//     This collapses down points that have the same index, and are
		//     separate simply becauset hey have multiple UV coordinates
		AutoArray<LWPntID>				LWPntIDToAbsoluteIndex(0);

		// Ignore TAGs ...
		if (lc->flavor != LISTEN_POLYS)
		{
			continue;
		}

		// Quick check for some Skins
		findImages(lc,model,scratch->ModelDir,SURF_COLR);
		findImages(lc,model,scratch->ModelDir,SURF_TRAN);

		// We are always adding points and polys
		// 1 Vertex per LWPntID
		// 1 SkinPoint per Vertex

		// 2)  ... Stash the point's index from LWPntIDToAbsoluteIndex so 
		//     we have a list of "I'm point [0|1|2] in a poly with vertex N"
		AutoArray<VertexToPolygonMap>		EffectedVertexes(VertexToPolygonMap::INVALID);

		// We'll need to offset the things we get for each succesive mesh
		unsigned int polyoffset = model.PolyCount();
		unsigned int vertoffset = model.PointCount();
		unsigned int skinoffset = model.SkinPointCount();

		// Setup polygon and vertex info
		unsigned int ply = lc->polygoncount;
		while (ply-- > 0)
		{
			LC_Polygon *lp = LW_xprtFuncs->get_polygon(lc,ply);
			if (lp == 0)
				continue;

			// in step with import values
			MD2_Poly&	poly = model.Poly(polyoffset + lp->index);

			for (int pntidx = 0; pntidx < 3; pntidx ++)
			{
				// Set up an array that ignores extra points created by discontinuous UV
				LC_VertexPosition *vp = LW_xprtFuncs->get_vertex(lc,lp->vertex[pntidx]);
				unsigned int absidx = LWPntIDToAbsoluteIndex.find(vp->lwid);
				if (absidx == LWPntIDToAbsoluteIndex.BAD_INDEX)
				{	// Store the translation in the absolute index array
					absidx = LWPntIDToAbsoluteIndex.Next();
					LWPntIDToAbsoluteIndex[absidx] = vp->lwid;
					
					// And ready a slot in the Normalizer array
					EffectedVertexes[absidx] = VertexToPolygonMap(vp->lwid);
				}
				EffectedVertexes[absidx].add(lp->index,pntidx);

				// Record the point for the polygon (absindex)
				poly.vertex[pntidx] = absidx + vertoffset;

				// Record the skinpoint for the polygon (index)
				poly.texvec[pntidx] = lp->vertex[pntidx] + skinoffset;

				MD2_Point_Skin &skpnt = model.SkinPoint(poly.texvec[pntidx]);
				skpnt.tex[0] = vp->texmap[0];
				skpnt.tex[1] = 1.0f - vp->texmap[1];
			}
		}

		// Shove in the vertex positions
		unsigned int frameMD2 = 0;
		for (unsigned int loopidx = 0; loops && loops[loopidx]; loopidx ++)
		{	// Each loop, hit all the frames
			for (unsigned int fridx = 0;
					fridx < loops[loopidx]->length;
					fridx++, frameMD2++)
			{
				unsigned int frameLW = loops[loopidx]->lwframes[fridx];
				sprintf(model.Frame(frameMD2).Creator,
							"%s%d",loops[loopidx]->Name,fridx);

				// Setup a list of polygon normal scratch buffers for this frame
				AutoArray<PolygonNormal>	PolygonNormals(PolygonNormal::INVALID);

				// Now stash all the vertex positions
				int vtx = lc->vertexcount;
				while (vtx-- > 0)
				{
					LC_VertexOffset *vo = LW_xprtFuncs->get_vertex_at_frame(lc,vtx,frameLW);

					// Convert from LWO coordinates to MD2 coordinates
					Vector<vec_t>		v_lw(	vo->location[0],
												vo->location[1],
												vo->location[2]);

					// Stash the values to seed the vertex normals
					LC_VertexPosition *vp = LW_xprtFuncs->get_vertex(lc,vtx);
					unsigned int absidx = LWPntIDToAbsoluteIndex.find(vp->lwid);
					EffectedVertexes[absidx].set(v_lw,PolygonNormals);

					Vector<float>		v_MD2 = LW_TO_MD2_Coords * v_lw;
//						RotateByMatrix(v_lw,LW_TO_MD2_Coords,v_MD2);

					// Store position values
					MD2_Point_Frame &vert = model.PointsAtFrame(frameMD2)
												[absidx + vertoffset];
					for (int m = 0; m < 3; m ++)
					{
						vert.v[m] = v_MD2[m];
					}
				}

				// Go through the vertexes again and snag the vertex normals
				vtx = lc->vertexcount;
				while (vtx-- > 0)
				{
					LC_VertexPosition *vp = LW_xprtFuncs->get_vertex(lc,vtx);
					unsigned int absidx = LWPntIDToAbsoluteIndex.find(vp->lwid);
					MD2_Point_Frame &vert = model.PointsAtFrame(frameMD2)
												[absidx + vertoffset];

					vert.setNormal(EffectedVertexes[absidx].getNormal(PolygonNormals));
				}
			}
		}
	}
}

static void save(LC_Model mdl)
{
	ModelScratchPad	*scratch = (ModelScratchPad *)mdl;

	if (scratch->SaveModel == 0)
	{
		delete scratch;
		return;
	}

	// Do some post work
	MD2&			model	= scratch->Model;

	// Save out the data
	sprintf(tmp,"%s%s%s%s%s.MD2",	scratch->QuakeDir,
									scratch->QuakeDir[0] ? DIR_SEPARATOR : "",
									scratch->ModelDir,
									scratch->ModelDir[0] ? DIR_SEPARATOR : "",
									scratch->ModelName);
	model.WritetoDisk(tmp);
	delete scratch;
}

// Description
static char *describe(void *data)
{
	return (((ModelScratchPad *)data)->ModelName);
}

////////////////////////////////////
// XPanel functions - this is a VIEW panel
////////////////////////////////////
enum {	TABS_MAIN = 0x8000,

		// Directory stuff
		ID_QUAKEDIR_NAME,
		ID_MODELDIR_NAME,

		// Model stuff
		GROUPID_MODEL,
		ID_MODEL_NAME, ID_CHK_MODEL_NAME, 
		ID_SKINWIDTH, ID_SKINHEIGHT,

		// Headerfile output stuff
		GROUPID_HEADER,
		ID_HEADER_NAME, ID_CHK_HEADER_NAME
};

// Overall Controls
#define STR_QDirName_TEXT		"Base dir: "
#define STR_MDirName_TEXT		"Model Subdirectory: "

// Model only controls
#define STR_ModelName_TEXT		"Model Name: "
#define STR_SkinWid_TEXT		"Skin Width: "
#define STR_SkinHei_TEXT		"Skin Height: "
#define STR_ChkSaveModel_TEXT	"Save w/ All"

// Header file only controls
#define STR_Header_TEXT			"Header File: "
#define STR_ChkSaveHeadr_TEXT	"Save w/ All"

static void *xgetval( void *inst, unsigned long vid )
{
	void *v = 0;

	ModelScratchPad *data = (ModelScratchPad *)inst;

	// Return the index that matches the ref we have
	switch ( vid ) {
		// Overall values
      case ID_QUAKEDIR_NAME:
		v = (void *)&(data->QuakeDir);
		break;

      case ID_MODELDIR_NAME:
		v = (void *)&(data->ModelDir);
		break;

		// Model values
      case ID_MODEL_NAME:
		v = (void *)&(data->ModelName);
		break;

      case ID_CHK_MODEL_NAME:
		v = (void *)&(data->SaveModel);
		break;

      case ID_SKINWIDTH:
		v = (void *)&(data->SkinWidth);
		break;

      case ID_SKINHEIGHT:
		v = (void *)&(data->SkinHeight);
		break;

		// Header values
      case ID_HEADER_NAME:
		v = (void *)&(data->HeaderFile);
		break;

      case ID_CHK_HEADER_NAME:
		v = (void *)&(data->SaveHeader);
		break;

      default:
		  ;
	}

	return v;
}
static LWXPRefreshCode xsetval( void *inst, unsigned long vid, void *value )
{
	ModelScratchPad *data = (ModelScratchPad *)inst;

	switch ( vid ) {
	// Overall values
      case ID_QUAKEDIR_NAME:
		strcpy((char *)data->QuakeDir, (char *)value);
		break;

      case ID_MODELDIR_NAME:
		strcpy((char *)data->ModelDir, (char *)value);
		break;

		// Model values
      case ID_MODEL_NAME:
		strcpy((char *)data->ModelName, (char *)value);
		break;

      case ID_CHK_MODEL_NAME:
		data->SaveModel = *(int *)value;
		break;

      case ID_SKINWIDTH:
		data->SkinWidth = *(int *)value;
		break;

      case ID_SKINHEIGHT:
		data->SkinHeight = *(int *)value;
		break;

		// Header values
      case ID_HEADER_NAME:
		strcpy((char *)data->HeaderFile, (char *)value);
		break;

      case ID_CHK_HEADER_NAME:
		data->SaveHeader = *(int *)value;
		break;

      default:
		  ;
	}

	return LWXPRC_DFLT;
}

// When this is destroyed, delete the model
XCALL_( void )
xpanelDestroy(void *inst)
{
	ModelScratchPad *data = (ModelScratchPad *)inst;
	delete data;
}

// Potentially feeding with 'seed' data
static LWXPanelID buildpanel(void *seeddata)
{
	ModelScratchPad *data = (ModelScratchPad *)seeddata;
	if (data == 0)
	{
		data = new ModelScratchPad(0);
		strcpy(data->ModelName,"(unknown)");
	}	// sFileName
	
	static LWXPanelControl xctl[] = {
		{ ID_QUAKEDIR_NAME,		STR_QDirName_TEXT,		"sFileName" },
		{ ID_MODELDIR_NAME,		STR_MDirName_TEXT,		"string" },

		{ ID_MODEL_NAME,		STR_ModelName_TEXT,		"string" },
		{ ID_CHK_MODEL_NAME,	STR_ChkSaveModel_TEXT,	"iBoolean" },
		{ ID_SKINWIDTH,			STR_SkinWid_TEXT,		"integer" },
		{ ID_SKINHEIGHT,		STR_SkinHei_TEXT,		"integer" },

		{ ID_HEADER_NAME,		STR_Header_TEXT,		"string" },
		{ ID_CHK_HEADER_NAME,	STR_ChkSaveHeadr_TEXT,	"iBoolean" },
		{ 0 }
	};

	static LWXPanelDataDesc xdata[] = {
		{ ID_QUAKEDIR_NAME,		STR_QDirName_TEXT,		"string" },
		{ ID_MODELDIR_NAME,		STR_MDirName_TEXT,		"string" },

		{ ID_MODEL_NAME,		STR_ModelName_TEXT,		"string" },
		{ ID_CHK_MODEL_NAME,	STR_ChkSaveModel_TEXT,	"integer" },
		{ ID_SKINWIDTH,			STR_SkinWid_TEXT,		"integer" },
		{ ID_SKINHEIGHT,		STR_SkinHei_TEXT,		"integer" },

		{ ID_HEADER_NAME,		STR_Header_TEXT,		"string" },
		{ ID_CHK_HEADER_NAME,	STR_ChkSaveHeadr_TEXT,	"integer" },
		{ 0 }
	};

	// Try and arrange some things
	static LWXPanelHint xhint[] = {
		XpDESTROYNOTIFY(xpanelDestroy),
		XpXREQCFG(ID_QUAKEDIR_NAME,
					LWXPREQ_DIR,
					"Quake 2 game dir path",
					0),

		// Some ranges 
		XpMIN(ID_SKINWIDTH,16),XpMAX(ID_SKINWIDTH,4096),
		XpMIN(ID_SKINHEIGHT,16),XpMAX(ID_SKINHEIGHT,4096),

		// Model group
		XpGROUP_(GROUPID_MODEL),
			XpH(ID_MODEL_NAME),
			XpH(ID_CHK_MODEL_NAME),
			XpH(ID_SKINWIDTH),
			XpH(ID_SKINHEIGHT),
		XpEND,
		XpLABEL(GROUPID_MODEL,"Model Setup"),

		// Header group
		XpGROUP_(GROUPID_HEADER),
			XpH(ID_HEADER_NAME),
			XpH(ID_CHK_HEADER_NAME),
		XpEND,
		XpLABEL(GROUPID_HEADER,"Header file Setup"),

		// Tabs - 1 row with model, header
		XpTABS_(TABS_MAIN),
			XpH(GROUPID_MODEL),
			XpH(GROUPID_HEADER),
		XpEND,

		XpEND
	};

	LWXPanelID xpanel = LW_xpanFuncs->create( LWXP_VIEW, xctl );
	if ( !xpanel ) return NULL;

	LW_xpanFuncs->hint( xpanel, 0, xhint );
	LW_xpanFuncs->describe( xpanel, xdata, xgetval, xsetval );
	LW_xpanFuncs->viewInst( xpanel, data );
	LW_xpanFuncs->setData( xpanel, 0, data );

	data->XPanelID = xpanel;

	return xpanel;
};

// Update loops
static void activeloop(LC_Instance inst, LC_FrameLoop *loop)
{	// Nothing to do 
	return;
}

///////////////////////////////////
//Input / Output functions
////////////////////////////////////

// Save data
static void store( const LWSaveState *save, void *data)
{	
	ModelScratchPad *m = (ModelScratchPad *)data;
	m->SAVE(save);
}

// Load data
static void *load( const LWLoadState *load)
{
	ModelScratchPad *data = new ModelScratchPad(0);
	if (data->LOAD(load) == 0)
	{
		delete data;
		data = 0;
	}
	return (void *)data;
}

// Construction functions (Global callbacks)
static LWMRBCallbackType _callback = {
	&load,
	&store,

	&buildpanel,
	&describe,
	&activeloop,

	&init,
	&build,
	&save
};

LWMRBCallbackType *getCallback()
{
	return &_callback;
}


// Setup structure
static LWMRBExportType _type = {
	"Quake MD2",
	GLOBALPLUGINNAME,
};

LWMRBExportType *getFunc()
{
	return &_type;
}



/* Layout include file */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "lwxpanel.h"

/* Mystuff */
extern "C"
{
#include "LayoutExportMD3.h"
}
#include "lw_base.h"
#include "sys_base.h"
#include "sys_extra.h"
#include "ParseMD3.h"

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

		Vector<float>	 Vret = LW_TO_MD3_Coords * Vtmp;
//		RotateByMatrix(Vtmp,LW_TO_MD3_Coords,Vret);

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

AutoArray<MD3Loop *>			SpecificLoopData(0);

// Image name finding function
void findImages(LC_Channel *lc, MD3_Mesh &mesh, char *dirpref, const char *surfflag)
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
		for (int charidx = 0;tmp[charidx];charidx++)
		{
			if (tmp[charidx] == '\\')
				tmp[charidx] = '/';
		}

		unsigned int skinidx = 0;
		for (;skinidx < mesh.SkinCount();skinidx++)
		{
			if (strcmp(tmp,mesh.Skin(skinidx).Name) == 0)
				 break;
		}

		MD3_Skin& skin = mesh.Skin(skinidx);
		if (skin.Name[0] == 0)
			strcpy(skin.Name,tmp);
	}
}

// Merely the MD3 construction functions 
static LC_Model init(	LC_Instance		userinst,
						LC_FrameLoop	**loops[])	// List of frameloops we're spitting out
{
	ModelScratchPad *m = new ModelScratchPad(*loops);

	// Add in some stuff from the base one
	*m = *(ModelScratchPad *)userinst;
	if (m->SaveCfg)
	{	// Write out the cfg file
		m->dumpCfgFile();
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

	MD3&			model	= scratch->Model;
	LC_FrameLoop	**loops	= scratch->Loops;
	LC_Channel *lc = 0;

	// We've got a list of Channels - walk it and seed what we can
	for (unsigned int i = 0;
			lc = (LC_Channel *)LW_xprtFuncs->get_channel(inst,i);
			i++)
	{
		if (lc == 0)
			break;

		// TAGs ...
		if (lc->flavor == LISTEN_TAG) {
			// Find Tag
			unsigned int tagCount = 0;
			for (;tagCount < model.TagCount(); tagCount++)
			{
				if (strcmp(model.TagsAtFrame(0)[tagCount].Name,lc->name) == 0)
					break;
			}

			if (scratch->AnchorName[0] != 0)
			{
				if (strcmp(scratch->AnchorName,lc->name) == 0)
					scratch->AnchorTagIdx = (int)tagCount;
			}

			// Grab tag Positions/Orientations
			unsigned int frameMD3 = 0;
			for (unsigned int loopidx = 0;loops && loops[loopidx];loopidx ++)
			{	// Each loop, hit all the frames
				for (unsigned int fridx = 0;
						fridx < loops[loopidx]->length;
						fridx++, frameMD3++)
				{
					unsigned int frameLW = loops[loopidx]->lwframes[fridx];

					/* FIXME:  To update a current file, this does not work
					 * Need to check tags at each Frame to determine
					 * if it really needs to be added
					 */

					// Grab the tag and start it
					MD3_Tag &tag = model.TagsAtFrame(frameMD3)[tagCount];
					strcpy(tag.Name,lc->name);

					// Dig out the orientations for the 3 points
					Prep_PointsforTag p;
					Vector<vec_t>	vtmp[3];

					for (int vtx = 0; vtx < 3; vtx++)
					{
						LC_VertexOffset *vo = LW_xprtFuncs->get_vertex_at_frame(lc,vtx,frameLW);
						if (vo == 0)
							continue;

						vtmp[vtx] = Vector<vec_t>(	vo->location[0],
													vo->location[1],
													vo->location[2]);

						for (int cidx = 0; cidx < 3; cidx ++)
						{
							p.pos[vtx][cidx] = vtmp[vtx][cidx];
						}
					}

					// FIXME - calculate normal
					Vector<double> a, b, c;

					for (int z = 0; z < 3;z++)
					{
						a[z] = vtmp[1][z] - vtmp[0][z];
						b[z] = vtmp[2][z] - vtmp[0][z];
					}

					c = a.cross(b);
//					CrossProduct(a, b, c);

					double len = sqrt(	(c[0] * c[0]) + 
										(c[1] * c[1]) +
										(c[2] * c[2]) );

					for (z = 0; z < 3; z ++)
						p.nrml[z] = c[z] / len;

					p.FillTag(&tag);
				}
			}
		// Or meshes ...
		} else if (lc->flavor == LISTEN_POLYS) {
			// Find mesh
			unsigned int meshCount = 0;
			for (;meshCount < model.MeshCount(); meshCount++)
			{
				if (strcmp(model.Mesh(meshCount).Name(),lc->name) == 0)
					break;
			}

			// Grab Mesh
			MD3_Mesh& mesh = model.Mesh(meshCount);
			mesh.UpdateName(lc->name);

			// Quick check for some Skins
			findImages(lc,mesh,scratch->ModelDir,SURF_COLR);
			findImages(lc,mesh,scratch->ModelDir,SURF_TRAN);

			// Work off a few 'starter' values for appending
			unsigned int polyoffset = mesh.PolyCount();
			unsigned int vertoffset = mesh.PointCount();

			// Set up some arrays for finding the vertex normals
			// 1)  Convert an LWPntID to an integer index (base 0) ...
			//     This collapses down points that have the same index, and are
			//     separate simply becauset hey have multiple UV coordinates
			AutoArray<LWPntID>				LWPntIDToAbsoluteIndex(0);

			// 2)  ... Stash by the point's index from LWPntIDToAbsoluteIndex so 
			//     we have a list of "I'm point [0|1|2) in a poly with vertex N"
			AutoArray<VertexToPolygonMap>		EffectedVertexes(VertexToPolygonMap::INVALID);

			// Setup polygon info
			unsigned int ply = lc->polygoncount;
			while (ply-- > 0)
			{
				LC_Polygon *lp = LW_xprtFuncs->get_polygon(lc,ply);
				if (lp == 0)
					continue;

				MD3_Poly&			poly = mesh.Poly(ply + polyoffset);
				for (int pntidx = 0; pntidx < 3; pntidx ++)
				{	// Record the point for the polygon 
					poly.vind[pntidx] = lp->vertex[pntidx]+ vertoffset;

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
					EffectedVertexes[absidx].add(ply,pntidx);
				}
			}

			// Setup vertex info - per frame, by vertex
			int bFirst = 1;
			// Grab tag Positions/Orientations
			unsigned int frameMD3 = 0;
			for (unsigned int loopidx = 0; loops && loops[loopidx]; loopidx ++)
			{	// Each loop, hit all the frames
				for (unsigned int fridx = 0;
						fridx < loops[loopidx]->length;
						fridx++, frameMD3++)
				{
					unsigned int frameLW = loops[loopidx]->lwframes[fridx];
					// Setup a list of polygon normal scratch buffers for this frame
					AutoArray<PolygonNormal>	PolygonNormals(PolygonNormal::INVALID);

					// Track all the vertex positions
					int vtx = lc->vertexcount;
					while (vtx-- > 0)
					{
						LC_VertexOffset *vo = LW_xprtFuncs->get_vertex_at_frame(lc,vtx,frameLW);

						// Convert from LWO coordinates to MD3 coordinates
						Vector<vec_t>		v_lw(	vo->location[0],
													vo->location[1],
													vo->location[2]);

						// Stash the values to seed the vertex normals
						LC_VertexPosition *vp = LW_xprtFuncs->get_vertex(lc,vtx);
						unsigned int absidx = LWPntIDToAbsoluteIndex.find(vp->lwid);
						EffectedVertexes[absidx].set(v_lw,PolygonNormals);

						Vector<float>		v_md3 = LW_TO_MD3_Coords * v_lw;
//						RotateByMatrix(v_lw,LW_TO_MD3_Coords,v_md3);

						// Store values, and set ranges for min/max values
						MD3_Point_Frame &vert = mesh.PointsAtFrame(frameMD3)
															[vtx + vertoffset];
						for (int m = 0; m < 3; m ++)
						{
							vert.v[m] = v_md3[m] * 64;
						}

						// Grab UV texture values (once)
						if (bFirst)
						{
							LC_VertexPosition *vp = LW_xprtFuncs->get_vertex(lc,vtx);
							if (vp == 0)
								continue;

							MD3_Vert_Skin& skin = mesh.SkinPoint(vtx + vertoffset);
							skin.tex[0] = vp->texmap[0];
							skin.tex[1] = 1.0f - vp->texmap[1];
						}
					}

					// Go through the vertices again and snag the vertex normals
					vtx = lc->vertexcount;
					while (vtx-- > 0)
					{
						LC_VertexPosition *vp = LW_xprtFuncs->get_vertex(lc,vtx);
						unsigned int absidx = LWPntIDToAbsoluteIndex.find(vp->lwid);
						MD3_Point_Frame &vert = mesh.PointsAtFrame(frameMD3)
																[vtx + vertoffset];

						vert.setNormal(EffectedVertexes[absidx].getNormal(PolygonNormals));
					}
					
					bFirst = 0;
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
	MD3&			model	= scratch->Model;

	// if there was an achor tag, fix everything
	if (scratch->AnchorTagIdx != -1)
	{
		for (unsigned int fidx = 0; fidx < model.Mesh(0).FrameCount(); fidx ++)
		{
			MD3_Tag& anchor = model.TagsAtFrame(fidx)[scratch->AnchorTagIdx];

			// for each tag (except it) move the position and rotation around
			for (unsigned int tidx = 0; tidx < model.TagCount(); tidx++)
			{
				if (tidx == (unsigned int)scratch->AnchorTagIdx)
					continue;
				MD3_Tag &tag = model.TagsAtFrame(fidx)[tidx];

				// Fix location
				Vector<vec_t> V(tag.Position[0],
								tag.Position[1],
								tag.Position[2]);

				V = V - anchor.Position;
//				SubtractVector(V,anchor.Position,V);
				tag.Position = ~anchor.Rotation * V;
//				RotateByINVMatrix(V,anchor.Rotation,tag.Position);

				// Fix the rotations too
				for (int midx = 0; midx < 3; midx++)
				{
					V = Vector<vec_t>(	tag.Rotation[midx][0],
										tag.Rotation[midx][1],
										tag.Rotation[midx][2]);
					tag.Rotation[midx] = ~anchor.Rotation * V;
//					RotateByINVMatrix(V,anchor.Rotation,tag.Rotation[midx]);
				}
			}

			// for each point in each mesh, move it a bit
			for (unsigned int m = 0; m < model.MeshCount(); m++)
			{
				MD3_Mesh& mesh = model.Mesh(m);
				for (unsigned int v = 0; v < mesh.PointCount(); v++)
				{
					MD3_Point_Frame &vert = mesh.PointsAtFrame(fidx)[v];

					// Fix location
					Vector<vec_t> V(vert.v[0],
									vert.v[1],
									vert.v[2]);
					V /= 64;

					V = V - anchor.Position;
					Vector<vec_t> Vout = ~anchor.Rotation * V;

//					SubtractVector(V,anchor.Position,V);
//					RotateByINVMatrix(V,anchor.Rotation,Vout);
					for (int midx = 0; midx < 3; midx ++)
					{
						vert.v[midx] = Vout[midx] * 64;
					}

					// Fix 'normal'
				}
			}

			for (int c = 0; c < 3; c++)
			{
				anchor.Position[c] = 0;
				anchor.Rotation[c][0] = anchor.Rotation[c][1] = anchor.Rotation[c][2] = 0;
				anchor.Rotation[c][c] = 1.0;
			}
		}
	}

	// Save out the data
	sprintf(tmp,"%s%s%s%s%s.md3",	scratch->QuakeDir,
									scratch->QuakeDir[0] ? DIR_SEPARATOR : "",
									scratch->ModelDir,
									scratch->ModelDir[0] ? DIR_SEPARATOR : "",
									scratch->Name);
	model.WritetoDisk(tmp);
	delete scratch;
}

// Description
static char *describe(void *data)
{
	return (((ModelScratchPad *)data)->Name);
}

////////////////////////////////////
// XPanel functions - this is a VIEW panel
////////////////////////////////////
enum {	TABS_MAIN = 0x8000, TABS_LOOPS,
	
		// Overall stuff
		ID_QUAKEDIR_NAME, ID_MODELDIR_NAME,

		// Model stuff
		GROUPID_MODEL,
		ID_MODEL_NAME, ID_CHK_MODEL_NAME, ID_ANCHOR_NAME,

		// CFG file stuff
		GROUPID_CFG,
		ID_CFG_NAME, ID_CHK_CFG_NAME,
		ID_HEADERFILE,

		// Per-loop controls
		GROUPID_LOOP,
		ID_LOOPEDFRAMES, ID_FPS, ID_NOTES
};

// Overall controls
#define STR_QDirName_TEXT		"Base dir: "
#define STR_MDirName_TEXT		"Model Subdirectory: "

// Model only controls
#define STR_ModelName_TEXT		"Model Name"
#define STR_ChkSaveMesh_TEXT	"Save w/ All"
#define STR_AnchorName_TEXT		"Anchor tag name"

// Config file controls
#define STR_CfgName_TEXT		"Config File Name"
#define STR_CfgSaveAnim_TEXT	"Save w/ All"
#define STR_HeaderFile_TEXT		".cfg File Header"

// Per-loop controls
#define STR_Looping_TEXT		"Looping Frames"
#define STR_FPS_TEXT			"Frames / sec"
#define STR_Notes_TEXT			"Frame Notes"

static void *xgetval( void *inst, unsigned long vid )
{
	void *v = 0;

	ModelScratchPad *data = (ModelScratchPad *)inst;
	MD3Loop *loop = data->ActiveLoop < 0 ? 0 : SpecificLoopData[data->ActiveLoop];
	if (vid > GROUPID_LOOP && loop == 0)
	{	// nothing there, return
		return v;
	}

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
		v = (void *)&(data->Name);
		break;
      case ID_CHK_MODEL_NAME:
		v = (void *)&(data->SaveModel);
		break;
      case ID_ANCHOR_NAME:
		v = (void *)&(data->AnchorName);
		break;

	// cfg file values
      case ID_CFG_NAME:
		v = (void *)&(data->CfgName);
		break;
      case ID_CHK_CFG_NAME:
		v = (void *)&(data->SaveCfg);
		break;
      case ID_HEADERFILE:
		v = (void *)&(data->HeaderFile);
		break;

	// per-loop values
      case ID_LOOPEDFRAMES:
		v = (void *)&(loop->LoopingFrames);
		break;
      case ID_FPS:
		v = (void *)&(loop->FPS);
		break;
      case ID_NOTES:
		v = (void *)&(loop->Notes);
		break;

      default:
		  ;
	}

	return v;
}
static LWXPRefreshCode xsetval( void *inst, unsigned long vid, void *value )
{
	ModelScratchPad *data = (ModelScratchPad *)inst;

	MD3Loop *loop = data->ActiveLoop < 0 ? 0 : SpecificLoopData[data->ActiveLoop];
	if (vid > GROUPID_LOOP && loop == 0)
	{	// nothing there, return
		return LWXPRC_DFLT;;
	}
	
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
		strcpy((char *)data->Name, (char *)value);
		break;
      case ID_CHK_MODEL_NAME:
		data->SaveModel = *(( int * ) value );
		break;
      case ID_ANCHOR_NAME:
		strcpy((char *)data->AnchorName, (char *)value);
		break;

	// cfg file values
      case ID_CFG_NAME:
		strcpy((char *)data->CfgName, (char *)value);
		break;
      case ID_CHK_CFG_NAME:
		data->SaveCfg = *(( int * ) value );
		break;
      case ID_HEADERFILE:
		strcpy((char *)data->HeaderFile, (char *)value);
		break;

	// per-loop values
      case ID_LOOPEDFRAMES:
		loop->LoopingFrames = *(( int * ) value );
		break;
      case ID_FPS:
		loop->FPS = *(( int * ) value );
		break;
      case ID_NOTES:
		strcpy((char *)loop->Notes, (char *)value);
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
		strcpy(data->Name,"(unknown)");
	}	// sFileName
	
	static LWXPanelControl xctl[] = {
		{ ID_QUAKEDIR_NAME,		STR_QDirName_TEXT,		"sFileName" },
		{ ID_MODELDIR_NAME,		STR_MDirName_TEXT,		"string" },

		{ ID_MODEL_NAME,		STR_ModelName_TEXT,		"string" },
		{ ID_CHK_MODEL_NAME,	STR_ChkSaveMesh_TEXT,	"iBoolean" },
		{ ID_ANCHOR_NAME,		STR_AnchorName_TEXT,	"string" },

		{ ID_CFG_NAME,			STR_CfgName_TEXT,		"string" },
		{ ID_CHK_CFG_NAME,		STR_ChkSaveMesh_TEXT,	"iBoolean" },
		{ ID_HEADERFILE,		STR_HeaderFile_TEXT,	"sFileName" },

		{ ID_LOOPEDFRAMES,		STR_Looping_TEXT,		"integer" },
		{ ID_FPS,				STR_FPS_TEXT,			"integer" },
		{ ID_NOTES,				STR_Notes_TEXT,			"string" },

		{ 0 }
	};

	static LWXPanelDataDesc xdata[] = {
		{ ID_QUAKEDIR_NAME,		STR_QDirName_TEXT,		"string" },
		{ ID_MODELDIR_NAME,		STR_MDirName_TEXT,		"string" },

		{ ID_MODEL_NAME,		STR_ModelName_TEXT,		"string" },
		{ ID_CHK_MODEL_NAME,	STR_ChkSaveMesh_TEXT,	"integer" },
		{ ID_ANCHOR_NAME,		STR_AnchorName_TEXT,	"string" },

		{ ID_CFG_NAME,			STR_CfgName_TEXT,		"string" },
		{ ID_CHK_CFG_NAME,		STR_ChkSaveMesh_TEXT,	"integer" },
		{ ID_HEADERFILE,		STR_HeaderFile_TEXT,	"string" },

		{ ID_LOOPEDFRAMES,		STR_Looping_TEXT,		"integer" },
		{ ID_FPS,				STR_FPS_TEXT,			"integer" },
		{ ID_NOTES,				STR_Notes_TEXT,			"string" },

		{ 0 }
	};

	// Try and arrange some things
	static LWXPanelHint xhint[] = {
		XpDESTROYNOTIFY(xpanelDestroy),
		XpXREQCFG(ID_QUAKEDIR_NAME,
					LWXPREQ_DIR,
					"Quake III game dir path",
					0),

		// Set some limits
		XpMIN(ID_FPS,-1),
		XpMAX(ID_FPS,999),
		XpMIN(ID_LOOPEDFRAMES,-1),	// -1 == ALL
		XpMAX(ID_LOOPEDFRAMES,999),

		// Model group
		XpGROUP_(GROUPID_MODEL),
			XpH(ID_MODEL_NAME),
			XpH(ID_CHK_MODEL_NAME),
			XpH(ID_ANCHOR_NAME),
		XpEND,
		XpLABEL(GROUPID_MODEL,"Model Setup"),

		// Cfg file Group
		XpGROUP_(GROUPID_CFG),
			XpH(ID_CFG_NAME),
			XpH(ID_CHK_CFG_NAME),
			XpH(ID_HEADERFILE),
		XpEND,
		XpLABEL(GROUPID_CFG,"Config File"),

		// Tabs - 1 row with mesh/animation/script
		XpTABS_(TABS_MAIN),
			XpH(GROUPID_MODEL),
			XpH(GROUPID_CFG),
		XpEND,

		// Loop hints
		XpGROUP_(GROUPID_LOOP),
			XpH(ID_FPS),
			XpH(ID_LOOPEDFRAMES),
			XpH(ID_NOTES),
		XpEND,
		XpLABEL(GROUPID_LOOP, "Frame Loop editor"),

		// Tab for loops ...
		XpTABS_(TABS_LOOPS),
			XpH(GROUPID_LOOP),
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
{	// Make sure we have a Loop object for each
	MD3Loop *m = 0;
	ModelScratchPad *data = (ModelScratchPad *)inst;
	if (loop->ID == LOOP_END)
	{	// no selected loop
		data->ActiveLoop = -1;
	}
	else if (loop->ID < 0)
	{	// removing
		m = SpecificLoopData[-loop->ID];
		if (m != 0)
			delete m;
		SpecificLoopData[-loop->ID] = 0;
	}
	else
	{	// adding/updating
		m = SpecificLoopData[loop->ID];
		if (m == 0)
			m = new MD3Loop(loop->ID);
		SpecificLoopData[loop->ID] = m;
		data->ActiveLoop = loop->ID;
	}

	// refresh everything
	LWXPanelHint resize[] = {
		XpMAX(ID_LOOPEDFRAMES,loop->length),
		XpEND
	};

	LW_xpanFuncs->hint( data->XPanelID, ID_LOOPEDFRAMES, resize );
	LW_xpanFuncs->viewRefresh (data->XPanelID);
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
	"Quake MD3",
	GLOBALPLUGINNAME,
};

LWMRBExportType *getFunc()
{
	return &_type;
}



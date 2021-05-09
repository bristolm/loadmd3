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
#include "ParseMD3.h"

#include "LwChannel.h"

// extra lightwave files for geting the images
#include "lwrender.h"		// For Object Info, Scene Info
#include "lwmeshes.h"		// for Mesh info
#include "lwsurf.h"			// for Surface info
#include "lwtxtr.h"			// for Texture info
#include "lwimage.h"		// for Image List

static char	*DIR_SEPARATOR	= "\\";

static char tmp[1024];

LC_cVertexOffset	LC_cVertexOffset::INVALID = LC_cVertexOffset();
LW_FrameVertices	LW_FrameVertices::INVALID = LW_FrameVertices();

class PolygonNormal
{
	Vector<float>	normal;
	int				numadded;

	int				added[3];
	Vector<float>	vecs[3];

public:
	PolygonNormal():
		normal(Vector<float>(0)),
		numadded(0)
	{
		for (int i = 0; i < 3; i++)
		{
			added[i] = 0;
			vecs[i] = Vector<float>(0);
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
		Vector<float>	 Vtmp(Vector<float>(0));
		for (unsigned int i = targets.Next(); i > 0; i--)
		{
			Vtmp += polys[targets[i -1].index].getNormal();
		}
		Vtmp /= targets.Next();

		Vector<float>	 Vret(Vector<float>(0));
		RotateByMatrix(Vtmp,LW_TO_MD3_Coords,Vret);

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

// Object for handling the loop 'extra' data
// Structs for this plugin to use

class MD3Loop
{
public:
	int		index;

	int		LoopingFrames;
	int		FPS;
	char	Notes[256];

	MD3Loop(int idx):
		index(idx),
		LoopingFrames(0)
	{
		Notes[0] = 0;
		// get FPS from the system for default
		LWSceneInfo *sceneinfo =
			(LWSceneInfo *)LW_globalFuncs( LWSCENEINFO_GLOBAL, GFUSE_TRANSIENT );

		FPS = (int)sceneinfo->framesPerSecond;
	}
};

// List of specific Loop data - 1 for the whole app
static AutoArray<MD3Loop *>		SpecificLoopData(0);

// Model construction holding area
class ModelScratchPad
{
public:
	LWXPanelID				XPanelID;

	char					QuakeDir[256];
	char					ModelDir[256];

	char					Name[256];
	int						SaveModel;
	char					AnchorName[256];
	int						AnchorTagIdx;

	char					CfgName[256];
	int						SaveCfg;
	char					Extra1[256];
	char					Extra2[256];
	char					Extra3[256];

	MD3						Model;
	LC_FrameLoop			**Loops;

	int						ActiveLoop;

	ModelScratchPad(LC_FrameLoop **loops):
		XPanelID(0),
		SaveModel(0),AnchorTagIdx(-1),SaveCfg(0),
		Model(),
		Loops(loops),
		ActiveLoop(-1)
	{
		Name[0] = QuakeDir[0] = ModelDir[0] = AnchorName[0] = 0;
		Extra1[0] = Extra2[0] = Extra3[0] = 0;
		sprintf(CfgName,"animation.cfg");
	}

	ModelScratchPad& operator=( const ModelScratchPad& rhs)
	{	// Just copy some stuff over
		strcpy(QuakeDir,rhs.QuakeDir);
		strcpy(ModelDir,rhs.ModelDir);

		strcpy(Name,rhs.Name);
		SaveModel = rhs.SaveModel;
		strcpy(AnchorName,rhs.AnchorName);

		strcpy(CfgName,rhs.CfgName);
		SaveCfg = rhs.SaveCfg;

		strcpy(Extra1,rhs.Extra1);
		strcpy(Extra2,rhs.Extra2);
		strcpy(Extra3,rhs.Extra3);

		// Fill in model name based on this stuff
		sprintf(tmp,"%s.md3",Name);
		Model.UpdateName(tmp);

		return *this;
	}

	void dumpCfgFile()
	{
		sprintf(tmp,"%s%s%s%s%s",QuakeDir,DIR_SEPARATOR,
								ModelDir,DIR_SEPARATOR,
								CfgName);

		FILE *fp;
		if ((fp = fopen(tmp,"w")) == (FILE *)NULL)
			return;

		fprintf(fp,"// Generated by LW Export Plugin v%s\n",PROG_VERSION); 
		fprintf(fp,"%s\n",Extra1);
		fprintf(fp,"%s\n",Extra2);
		fprintf(fp,"%s\n",Extra3);
		fprintf(fp,"\n");
		int iStart = 0;
		for (int i = 0; Loops && Loops[i]; i++)
		{
			int idx = Loops[i]->ID;
			MD3Loop *loop = SpecificLoopData[idx];
			if (loop == 0)
			{
				loop = new MD3Loop(idx);
				SpecificLoopData[idx] = loop;
			}

			int loo = loop->LoopingFrames;
			if (loo < 0)
				loo = Loops[i]->length;

			fprintf(fp,"%d\t%d\t%d\t%d\t\t// %s %s\n",
						iStart, Loops[i]->length,
						loo, loop->FPS,
						Loops[i]->Name, loop->Notes);
			iStart += Loops[i]->length;
		}

		fclose(fp);
		fp = 0;
	}
};

// Image name finding function
void findImages(LC_cChannel *lc, MD3_Mesh &mesh, char *dirpref, const char *surfflag)
{
	// Check for Skins - just the first point on the first poly ...
	LC_Polygon *lp = lc->getPolygon(0);

	// Only accept Alpha maps (transparency) or color base maps
	LWObjectInfo *objInfo		= (LWObjectInfo *)LW_globalFuncs( LWOBJECTINFO_GLOBAL, GFUSE_TRANSIENT );
	LWSurfaceFuncs *surfFuncs	= (LWSurfaceFuncs *)LW_globalFuncs( LWSURFACEFUNCS_GLOBAL, GFUSE_TRANSIENT );
	LWTextureFuncs *texFuncs	= (LWTextureFuncs *)LW_globalFuncs( LWTEXTUREFUNCS_GLOBAL, GFUSE_TRANSIENT );
	LWImageList  *imgList		= (LWImageList  *)LW_globalFuncs( LWIMAGELIST_GLOBAL, GFUSE_TRANSIENT );

	LWMeshInfoID lwmesh = objInfo->meshInfo(lc->getID(),0);
	LWSurfaceID lwsurfid = lp->surf;

	// Get texture
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
			|| lwvmap != lwmesh->pntVLookup(lwmesh, LWVMAP_TXUV, lc->getName()))
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

// This is called for each separate object referenced for this
// model.  Consequently, things needed for 'state' need to be 
// stashed in the object referenced by LC_Model 
static void build(LC_Model		mdl,			// current 'in progress' model
				  LWInstance	inst )			// opaque pointer to server's data for this model
{
	ModelScratchPad	*scratch = (ModelScratchPad *)mdl;
	if (scratch->SaveModel == 0)
		return;

	MD3&			model	= scratch->Model;
	LC_FrameLoop	**loops	= scratch->Loops;
	LC_cChannel *lc = 0;
	unsigned int i = 0;

	// Set up some arrays for finding the vertex normals - 1 per mesh
	// 1)  Convert an LWPntID to an integer index (base 0) ...
	//     This collapses down points that have the same index, and are
	//     separate simply because they have multiple UV coordinates
	AutoArray<LWPntID>				*LWPntIDToAbsoluteIndex[64] = {0};

	// 2)  ... Stash by the point's index from LWPntIDToAbsoluteIndex so 
	//     we have a list of "I'm point [0|1|2) in a poly with vertex N"
	AutoArray<VertexToPolygonMap>	*EffectedVertexes[64] = {0};

	// Do some prelim "one time only" stuff like polygon maps, UV,
	// and  look for an Anchor tag
	if (scratch->AnchorName[0] != 0 && 
		scratch->AnchorTagIdx < 0)
	{
		for (i = 0;
				lc = (LC_cChannel *)LW_xprtFuncs->get_channel(inst,i);
				i++)
		{
			if (lc == 0)
				break;

			if (lc->getFlavor() == LISTEN_TAG)
			{	// Look for the Anchor tag
				unsigned int tagCount = 0;
				for (;tagCount < model.TagCount(); tagCount++)
				{
					if (strcmp(model.TagsAtFrame(0)[tagCount].Name,lc->getName()) == 0)
						break;
				}

				// Grab the tag
				MD3_Tag &tag = model.TagsAtFrame(0)[tagCount];
				strcpy(tag.Name,lc->getName());
				if (strcmp(scratch->AnchorName,lc->getName()) == 0)
				{
					scratch->AnchorTagIdx = (int)tagCount;
				}
			}
			else if (lc->getFlavor() == LISTEN_POLYS)
			{	// setup polygon maps and UV coordinates
				unsigned int meshCount = 0;
				for (;meshCount < model.MeshCount(); meshCount++)
				{
					if (strcmp(model.Mesh(meshCount).Name(),lc->getName()) == 0)
						break;
				}

				// Grab Mesh
				MD3_Mesh& mesh = model.Mesh(meshCount);
				mesh.UpdateName(lc->getName());

				// Setup the vertex collectors
				AutoArray<LWPntID>&		LWPntIDMap =
					*(LWPntIDToAbsoluteIndex[meshCount] = 
						new AutoArray<LWPntID>(0));

				AutoArray<VertexToPolygonMap>& EffVert =
					*(EffectedVertexes[meshCount] =
						new AutoArray<VertexToPolygonMap>(VertexToPolygonMap::INVALID));

				// Quick check for some Skins
				findImages(lc,mesh,scratch->ModelDir,SURF_COLR);
				findImages(lc,mesh,scratch->ModelDir,SURF_TRAN);

				// Setup polygon info
				unsigned int ply = lc->getPolygonCount();
				while (ply-- > 0)
				{
					LC_Polygon *lp = lc->getPolygon(ply);
					if (lp == 0)
						continue;

					MD3_Poly&			poly = mesh.Poly(ply);
					for (int pntidx = 0; pntidx < 3; pntidx ++)
					{	// Record the point for the polygon 
						poly.vind[pntidx] = lp->vertex[pntidx];

						// Set up an array that ignores extra points created by discontinuous UV
						LC_VertexPosition *vp = lc->getVertex(lp->vertex[pntidx]);
						unsigned int absidx = LWPntIDMap.find(vp->lwid);
						if (absidx == LWPntIDMap.BAD_INDEX)
						{	// Store the translation in the absolute index array
							absidx = LWPntIDMap.Next();
							LWPntIDMap[absidx] = vp->lwid;

							// And ready a slot in the Normalizer array
							EffVert[absidx] = VertexToPolygonMap(vp->lwid);
						}
						EffVert[absidx].add(ply,pntidx);
					}
				}

				// Setup UV info
				int vtx = lc->getVertexCount();
				while (vtx-- > 0)
				{
					LC_VertexPosition *vp = lc->getVertex(vtx);
					if (vp == 0)
						continue;

					MD3_Vert_Skin& skin = mesh.SkinPoint(vtx);
					skin.tex[0] = vp->texmap[0];
					skin.tex[1] = 1.0f - vp->texmap[1];
				}
			}
		}
	}

	// Now walk all the frames and build the model
	unsigned int frameMD3 = 0;
	for (unsigned int loopidx = 0;loops && loops[loopidx];loopidx ++)
	{	// Each loop, hit all the frames
		unsigned int frameLW = loops[loopidx]->lwstart;
		for (unsigned int fridx = loops[loopidx]->length;
				fridx > 0;
				fridx--, frameLW++, frameMD3++)
		{
			// Look for tags first, so we can get the AnchorTag position set
			unsigned int tagCount = 0;
			for (i = 0;
					lc = (LC_cChannel *)LW_xprtFuncs->get_channel(inst,i);
					i++)
			{
				if (lc == 0)
					break;
				if (lc->getFlavor() != LISTEN_TAG)
					continue;

				for (;tagCount < model.TagCount(); tagCount++)
				{
					if (strcmp(model.TagsAtFrame(0)[tagCount].Name,lc->getName()) == 0)
						break;
				}

				// Grab the tag and start it
				MD3_Tag &tag = model.TagsAtFrame(frameMD3)[tagCount];
				strcpy(tag.Name,lc->getName());

				// Dig out the orientations for the 3 points
				Prep_PointsforTag p;
				Vector<vec_t>	vtmp[3];

				for (int vtx = 0; vtx < 3; vtx++)
				{
					LC_VertexOffset *vo = lc->getVertexAtFrame(vtx,frameLW);
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

				CrossProduct(a, b, c);

				double len = sqrt(	(c[0] * c[0]) + 
									(c[1] * c[1]) +
									(c[2] * c[2]) );

				for (z = 0; z < 3; z ++)
					p.nrml[z] = c[z] / len;

				p.FillTag(&tag);
			}

			// Set up to collect the boundaries
			double	mins[3] = {999,999,999};
			double	maxs[3] = {-999,-999,-999};

			// Now do the meshes - get the point info
			unsigned int meshCount = 0;
			for (i = 0;
					lc = (LC_cChannel *)LW_xprtFuncs->get_channel(inst,i);
					i++)
			{
				if (lc == 0)
					break;
				if (lc->getFlavor() != LISTEN_POLYS)
					continue;

				for (;meshCount < model.MeshCount(); meshCount++)
				{
					if (strcmp(model.Mesh(meshCount).Name(),lc->getName()) == 0)
						break;
				}

				// Grab Mesh
				MD3_Mesh& mesh = model.Mesh(meshCount);

				// Grab my collector
				AutoArray<LWPntID>&	LWPntIDMap = 
								*(LWPntIDToAbsoluteIndex[meshCount]);

				AutoArray<VertexToPolygonMap>& EffVert =
								*(EffectedVertexes[meshCount]);

				// Setup a list of polygon normal scratch buffers for this frame
				AutoArray<PolygonNormal>	PolygonNormals(PolygonNormal::INVALID);

				// Track all the vertex positions
				int vtx = lc->getVertexCount();
				while (vtx-- > 0)
				{
					LC_VertexOffset *vo = lc->getVertexAtFrame(vtx,frameLW);

					// Convert from LWO coordinates to MD3 coordinates
					Vector<vec_t>		v_lw(	vo->location[0],
												vo->location[1],
												vo->location[2]);

					// Stash the values to seed the vertex normals
					LC_VertexPosition *vp = lc->getVertex(vtx);
					unsigned int absidx = LWPntIDMap.find(vp->lwid);
					EffVert[absidx].set(v_lw,PolygonNormals);

					Vector<float>		v_md3;
					RotateByMatrix(v_lw,LW_TO_MD3_Coords,v_md3);

					// Skew them based on Anchor tag position
					if (scratch->AnchorTagIdx != -1)
					{
						MD3_Tag &anchor = model.TagsAtFrame(frameMD3)
									[(unsigned int)scratch->AnchorTagIdx];

						Vector<float>		tmpV = v_md3;
						SubtractVector(tmpV,anchor.Position,tmpV);
						RotateByINVMatrix(tmpV,anchor.Rotation,v_md3);
					}

					// Store values, and set ranges for min/max values
					MD3_Point_Frame &vert = mesh.PointsAtFrame(frameMD3)[vtx];
					for (int m = 0; m < 3; m ++)
					{
						if (v_md3[m] < mins[m])
							mins[m] = v_md3[m];
						if (v_md3[m] > maxs[m])
							maxs[m] = v_md3[m];
						vert.v[m] = v_md3[m] * 64;
					}
				}

				// Go through the vertexes again and snag the vertex normals
				vtx = lc->getVertexCount();
				while (vtx-- > 0)
				{
					LC_VertexPosition *vp = lc->getVertex(vtx);
					unsigned int absidx = LWPntIDMap.find(vp->lwid);
					MD3_Point_Frame &vert = mesh.PointsAtFrame(frameMD3)[vtx];

					Vector<float> v_norm = EffVert[absidx].getNormal(PolygonNormals);

					// Skew them based on Anchor tag position
					if (scratch->AnchorTagIdx != -1)
					{
						MD3_Tag &anchor = model.TagsAtFrame(frameMD3)
									[(unsigned int)scratch->AnchorTagIdx];

						Vector<float>		tmpV = v_norm;
						SubtractVector(tmpV,anchor.Position,tmpV);
						RotateByINVMatrix(tmpV,anchor.Rotation,v_norm);
					}

					vert.setNormal(v_norm);
				}
			}

			// Store the frame boundaries
			MD3_Frame&	frm = model.Frame(frameMD3);
			for (int j = 0; j < 3; j++)
			{
				frm.Maxs[j] = maxs[j] / 64;
				frm.Mins[j] = mins[j] / 64;
				frm.Position[j] = 0;
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

	// If an anchor is requested, look for it
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

				SubtractVector(V,anchor.Position,V);
				RotateByINVMatrix(V,anchor.Rotation,tag.Position);

				// Fix the rotations too
				for (int midx = 0; midx < 3; midx++)
				{
					V = Vector<vec_t>(	tag.Rotation[midx][0],
										tag.Rotation[midx][1],
										tag.Rotation[midx][2]);
					RotateByINVMatrix(V,anchor.Rotation,tag.Rotation[midx]);
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
	sprintf(tmp,"%s%s%s%s%s.md3",scratch->QuakeDir,DIR_SEPARATOR,
									scratch->ModelDir,DIR_SEPARATOR,
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
		ID_XTRA1,ID_XTRA2, ID_XTRA3,

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
#define STR_Xtra_TEXT			"Extra text"

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
      case ID_XTRA1:
		v = (void *)&(data->Extra1);
		break;
      case ID_XTRA2:
		v = (void *)&(data->Extra2);
		break;
      case ID_XTRA3:
		v = (void *)&(data->Extra3);
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
      case ID_XTRA1:
		strcpy((char *)data->Extra1, (char *)value);
		break;
      case ID_XTRA2:
		strcpy((char *)data->Extra2, (char *)value);
		break;
      case ID_XTRA3:
		strcpy((char *)data->Extra3, (char *)value);
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
	}
	
	static LWXPanelControl xctl[] = {
		{ ID_QUAKEDIR_NAME,		STR_QDirName_TEXT,		"string" },
		{ ID_MODELDIR_NAME,		STR_MDirName_TEXT,		"string" },

		{ ID_MODEL_NAME,		STR_ModelName_TEXT,		"string" },
		{ ID_CHK_MODEL_NAME,	STR_ChkSaveMesh_TEXT,	"iBoolean" },
		{ ID_ANCHOR_NAME,		STR_AnchorName_TEXT,	"string" },

		{ ID_CFG_NAME,			STR_CfgName_TEXT,		"string" },
		{ ID_CHK_CFG_NAME,		STR_ChkSaveMesh_TEXT,	"iBoolean" },
		{ ID_XTRA1,				STR_Xtra_TEXT,			"string" },
		{ ID_XTRA2,				STR_Xtra_TEXT,			"string" },
		{ ID_XTRA3,				STR_Xtra_TEXT,			"string" },

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
		{ ID_XTRA1,				STR_Xtra_TEXT,			"string" },
		{ ID_XTRA2,				STR_Xtra_TEXT,			"string" },
		{ ID_XTRA3,				STR_Xtra_TEXT,			"string" },

		{ ID_LOOPEDFRAMES,		STR_Looping_TEXT,		"integer" },
		{ ID_FPS,				STR_FPS_TEXT,			"integer" },
		{ ID_NOTES,				STR_Notes_TEXT,			"string" },

		{ 0 }
	};

	// Try and arrange some things
	static LWXPanelHint xhint[] = {
		XpDESTROYNOTIFY(xpanelDestroy),

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
			XpH(ID_XTRA1),
			XpH(ID_XTRA2),
			XpH(ID_XTRA3),
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

////////////////////////////////////

////////////////////////////////////
//Input / Output functions
////////////////////////////////////

// Lump values for Load/Save
#define ID_MD3D  LWID_( 'M','D','3','D' )
#define ID_MD3L  LWID_( 'M','D','3','L' )
static LWBlockIdent idmain[] = {
	ID_MD3D, "MD3Data",
	ID_MD3L, "MD3Loop",
	0
};

#define ID_MD3G  LWID_( 'M','D','3','G' )
#define ID_MD3M  LWID_( 'M','D','3','M' )
#define ID_MD3C  LWID_( 'M','D','3','C' )
static LWBlockIdent idloopelement[] = {
	ID_MD3G, "General",
	ID_MD3M, "Model",
	ID_MD3C, "CFG",
	0
};

// Save data
static void store( const LWSaveState *save, void *data )
{	
	float f[1];
	f[0] = PLUGINVERSION ;

	LWSAVE_FP(save,f,1);			// Spit out version

	short s[3];
	s[0] = s[1] = s[2] = 0;
	if (data == 0)
	{	// Save generic data - roll through the loops
		for (unsigned int i = 0; i < SpecificLoopData.Next(); i++)
		{
			MD3Loop *m = SpecificLoopData[i];
			if (m == 0)
				continue;
			LWSAVE_BEGIN( save, &idmain[ 1 ], 1 );
			s[0] = m->index;
			s[1] = m->LoopingFrames;
			s[2] = m->FPS;
			LWSAVE_I2( save, s, 3);
			LWSAVE_STR(save, m->Notes);
			LWSAVE_END( save );
		}
	}
	else
	{	// Save model data
		ModelScratchPad *m = (ModelScratchPad *)data;

		LWSAVE_BEGIN( save, &idmain[ 0 ], 0 );
		{
			LWSAVE_BEGIN( save, &idloopelement[ 0 ], 1 );
			 LWSAVE_STR(save, m->QuakeDir);
			 LWSAVE_STR(save, m->ModelDir);
			LWSAVE_END( save );

			LWSAVE_BEGIN( save, &idloopelement[ 1 ], 1 );
			 s[0] = m->SaveModel;
			 LWSAVE_STR(save, m->Name);
			 LWSAVE_STR(save, m->AnchorName);
			 LWSAVE_I2(save, s,1);
			LWSAVE_END( save );

			LWSAVE_BEGIN( save, &idloopelement[ 2 ], 1 );
			 s[0] = m->SaveCfg;
			 LWSAVE_STR(save, m->CfgName);
			 LWSAVE_I2(save, s,1);
			 LWSAVE_STR(save, m->Extra1);
			 LWSAVE_STR(save, m->Extra2);
			 LWSAVE_STR(save, m->Extra3);
			LWSAVE_END( save );
		}
		LWSAVE_END( save );
	}
}

// Load data
static void *load( const LWLoadState *load )
{
	float f[1];
	LWLOAD_FP(load,f,1);		// Snag version

	ModelScratchPad *data = 0;
	MD3Loop *loop = 0;
	short s[3];
	s[0] = s[1] = s[2] = 0;

	while ( LWID id = LWLOAD_FIND( load, idmain ))
	{
		switch ( id )
		{
		case ID_MD3D:			// New model
			data = new ModelScratchPad(0);
			while ( LWID mid = LWLOAD_FIND( load, idloopelement ))
			{
				switch ( mid ) {
				case ID_MD3G:		// General
					LWLOAD_STR(load,data->QuakeDir,256);
					LWLOAD_STR(load,data->ModelDir,256);
					break;
				case ID_MD3M:		// Model
					LWLOAD_STR(load,data->Name,256);
					LWLOAD_STR(load,data->AnchorName,256);
					LWLOAD_I2(load,s,1);
					data->SaveModel = s[0];
					break;
				case ID_MD3C:		// CFG
					LWLOAD_STR(load,data->CfgName,256);
					LWLOAD_I2(load,s,1);
					data->SaveCfg = s[0];
					LWLOAD_STR(load,data->Extra1,256);
					LWLOAD_STR(load,data->Extra2,256);
					LWLOAD_STR(load,data->Extra3,256);
					break;
				}
				LWLOAD_END( load );
			}
			break;

		case ID_MD3L:			// Per-Loop data
			LWLOAD_I2(load,s,3);
			loop = new MD3Loop(s[0]);
			loop->LoopingFrames = s[1];
			loop->FPS = s[2];
			LWLOAD_STR(load,loop->Notes,256);
			SpecificLoopData[s[0]] = loop;
			break;
		}
		LWLOAD_END( load );
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



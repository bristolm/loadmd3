/**************************************
 *
 *  LayoutExportUnrealSkel.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Meat of all the functions for the Unreal Tournament Skeletal export module.
 *
 *  - Populates our LWMRBCallbackType struct and makes it available
 *  - Handles XPanel callbacks (XPanel embedded in LayoutRecorder panels)
 *  - Converts LayoutRecorder recorded data into Unreal Skeletal PSA and PSK
 *
 *  Unreal Tounament is a trademark of Epic MegaGames, Inc.
 *
 **************************************/

/* Layout include file */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Mystuff */
#include "sys_base.h"
extern "C"
{
#include "LayoutExportUTSkel.h"
}

#include "sys_math.h"
#include "lw_base.h"
#include "ParseUnrealSkl.h"

#define X		0
#define Y		1
#define Z		2
#define W		3

#define H		0
#define P		1
#define B		2

#define LOOP_NAME_MAX	64
#define DEFAULT_RATE	15.0

static char	*DIR_SEPARATOR	= "\\";

static char tmp[1024];

// We want a name like SKIN## and then flag types
static int cullBitsFromName(const char *name, int *basebits)
{
	*basebits = -1;
	if (name == 0 || name[0] == 0)
		return 0;

	char buf[256];
	strcpy(buf,name);
	if (strlen(buf) < 5)
		return 0;

	char *p = 0;
	for (p = &(buf[0]);*p;p++)
	{
		if (*p >= 'A' && *p <= 'Z')
		{
			*p = *p - 'A' + 'a';
		}
	}
	if (strncmp(buf,"skin",4) != 0)
		return 0;

	// walk past the skin numbers 
	p = &(buf[0]) + strlen("skin");
	if (*p < '0' || *p > '9')
		return -1;

	int matidx = 0;
	while (*p >= '0' && *p <= '9')
	{
		matidx *= 10;
		matidx += *p - '0';
		++p;
	}

	// now look for bit flags
	*basebits = 0;
	while (*p)
	{
		for (int i = 0; i < MAX_UT_MATERIAL_CHUNKS; i++)
		{
			if (strncmp(p,UT_NameChunks[i].Name,
						strlen(UT_NameChunks[i].Name)) != 0)
				continue;

			// matched - flag and start over
			p += strlen(UT_NameChunks[i].Name);
			*basebits |= UT_NameChunks[i].BitFlag;
			break;
		}

		// If this wasn't a good letter, advance and continue;
		if (i == MAX_UT_MATERIAL_CHUNKS)
			++p;
	}

	return matidx;
}

static int getBitsFromChannel(LC_Channel *lc)
{
	LWSurfaceFuncs *surfFuncs	=
		(LWSurfaceFuncs *)LW_globalFuncs( LWSURFACEFUNCS_GLOBAL, GFUSE_TRANSIENT );
	LWSurfaceID *surfid = surfFuncs->byName(lc->name,0);

	int basebits = 0;
	if (lc->flavor == LISTEN_TAG)
	{	// Weapon - Place Holder
		basebits = MTT_Placeholder;
	}
	else if (surfid == 0 || surfid[0] == 0)
	{	// Regular
	}

	// These stuff is off the Surface->Basic panel
	else if (surfFuncs->getInt(surfid[0],SURF_SIDE) == 2)
	{	// Double sided
		basebits = MTT_NormalTwoSided;
	}

	else if (surfFuncs->getEnv(surfid[0],SURF_TRAN) != 0)
	{	// Transparent --> Masked
		basebits = MTT_Masked;
	}
	else if (surfFuncs->getEnv(surfid[0],SURF_RIND) != 0)
	{	// Refraction Index -->  Modulate
		basebits = MTT_Modulate;
	}
	else if (surfFuncs->getEnv(surfid[0],SURF_TRNL) != 0)
	{	// Transluscent --> Translucent
		basebits = MTT_Translucent;
	}

	// Add the other 'extra' bits
	if (surfid != 0 && surfid[0] != 0)
	{
		if (surfFuncs->getEnv(surfid[0],SURF_SPEC) != 0)
		{	// Specularity --> Environment
			basebits |= MTT_Environment;
		}
		if (surfFuncs->getEnv(surfid[0],SURF_GLOS) != 0)
		{	// Glossiness ---> Unlit
			basebits |= MTT_Unlit;
		}
		if (surfFuncs->getFlt(surfid[0],SURF_REFL) != 0)
		{	// Reflectivity --> Alpha
			basebits |= MTT_Alpha;
		}

		if (surfFuncs->getEnv(surfid[0],SURF_BUMP) != 0)
		{	// Bump --> no smoothing
			basebits |= MTT_NoSmooth;
		}

		if (surfFuncs->getFlt(surfid[0],SURF_SMAN) == 0)
		{	// 0 Smoothing  -->  Flat
			basebits |= MTT_Flat;
		}
	}

	return basebits;
}

// Structs for this plugin to use
class UnrealLoop
{
public:
	int		index;		// provided by the main plugin

	char	LoopName[LOOP_NAME_MAX];
	char	LoopGroup[LOOP_NAME_MAX];
	double	LoopRate;
	int		StartBone;
	double	KeyReduction;
	int		RootInclude;

	UnrealLoop(int idx):
		index(idx),
		StartBone(0),KeyReduction(1),RootInclude(0)
	{
		// get FPS from the system for default
		LWSceneInfo *sceneinfo =
			(LWSceneInfo *)LW_globalFuncs( LWSCENEINFO_GLOBAL, GFUSE_TRANSIENT );

		LoopRate = (float)sceneinfo->framesPerSecond;
		LoopName[0] = LoopGroup[0] = 0;
	}
};

class UnrealData
{
public:
	LWXPanelID	XPanelID;

	char	Output[256];
	// Mesh
	char	MeshName[LOOP_NAME_MAX];
	int		AlwaysSaveMesh;
	// Animation
	char	AnimName[LOOP_NAME_MAX];
	int		AlwaysSaveAnim;
	// Script
	char	ScriptName[LOOP_NAME_MAX];
	int		AlwaysSaveScript;

	// Loops
	int			ActiveLoop;

	UnrealData():
		AlwaysSaveMesh(0),
		AlwaysSaveAnim(0),
		AlwaysSaveScript(0),
		ActiveLoop(-1)
	{
		Output[0] = AnimName[0] = ScriptName[0] = 0;
		sprintf(MeshName,"(unknown)");
	}
};

// List of specific Loop data - 1 for the whole app
static AutoArray<UnrealLoop *>		SpecificLoopData(0);

/*
 * Coordinate concerns - LW uses a left-handed coordinate set, 
 * typically oriented with +Y being UP. +Z forward, and +X right
 * Unreal is right-handed with +X being front, +Z being up, and
 * +Y being Left
 *
 * LW rotation is returned as HPB: 
 *   heading (yaw)  is around Y
 *   pitch          is around X
 *   bearing (roll) is around Z
 */

// work function - ang[] are in LW space, radians
static Quaternion<float> hpb_to_unreal_quat (float ang[3], int isroot = 0)
{
	// Flip the orientations and relabel things for the other system
	Vector<float>	v_lw(ang[1],ang[0],ang[2]);
	Vector<float>	v_unskel(v_lw[2],-v_lw[0],v_lw[1]);

	// Root needs to be flipped (again) ...
	if (isroot)
		v_unskel = Vector<float>() - v_unskel;

	// To 3 Quats and multiply Heading *  Pitch * Bearing in Unreal
	// Multiplying by Heading (z), Pitch (y) , Roll (x)
//	Quaternion<float> q_unskel = Quaternion<float>(m_unskel);

	Quaternion<float> Qx = Quaternion<float>(	sin(v_unskel[0] / 2.0), 0.0, 0.0,
												cos(v_unskel[0] / 2.0));
	Quaternion<float> Qy = Quaternion<float>(	0.0, sin(v_unskel[1] / 2.0), 0.0,
												cos(v_unskel[1] / 2.0));
	Quaternion<float> Qz = Quaternion<float>(	0.0, 0.0, sin(v_unskel[2] / 2.0),
												cos(v_unskel[2] / 2.0));

	Quaternion<float> q_unskel = (Qx * Qy * Qz);
//	q_unskel = Quaternion<float>(-q_unskel[0],q_unskel[1],-q_unskel[2],q_unskel[3]);

	// To a matrix, then a Quat
//	Matrix<float>		m_unskel(v_unskel);
//	Quaternion<float>	q_unskel(q_unskel);

	return q_unskel;
}

class ModelScratchPad
{
public:
	UnrealData				*Data;
	UnrealSkeletalModel		 Model;
	LC_FrameLoop			**Loops;

	// Building 'helper' values.  Designed to span multiple input objects.
	AutoArray<LWSurfaceID>		SurfToSmoothingGroup;
	AutoArray<LWPntID>			LWPointToUnrealPoint;
	AutoArray<float>			WeightMapInfluences;
	AutoArray<LC_Channel *>		BoneCache;

	ModelScratchPad(LC_Instance	inst, LC_FrameLoop **loops):
		Data((UnrealData *)inst),
		Model(),
		Loops(loops),
		SurfToSmoothingGroup(0),
		LWPointToUnrealPoint(0),
		WeightMapInfluences(0.0f),
		BoneCache(0)
		{;}
};

// Merely the Model construction functions 
static LC_Model	init(	LC_Instance		userinst,
						LC_FrameLoop	***loops)	// List of frameloops we're spitting out
{
	return (void *)new ModelScratchPad(userinst,*loops);

	UnrealSkeletalModel *model = new UnrealSkeletalModel();
	//model.UpdateName(name);

	return (void *)model;
}

// The Unreal Skeletal Model construction functions 
static void build(LC_Model		mdl,			// current 'in progress' model
				  LWInstance	inst )			// opaque pointer to server's data for this input LW model
{
	ModelScratchPad			*scratch = (ModelScratchPad *)mdl;

	UnrealData				*data	= scratch->Data;
	UnrealSkeletalModel&	model	= scratch->Model;
	LC_FrameLoop			**loops	= scratch->Loops;

	AutoArray<LWSurfaceID>&		SurfToSmoothingGroup = scratch->SurfToSmoothingGroup;
	AutoArray<LWPntID>&			LWPointToUnrealPoint = scratch->LWPointToUnrealPoint;
	AutoArray<float>&			WeightMapInfluences = scratch->WeightMapInfluences;
	AutoArray<LC_Channel *>&	BoneCache = scratch->BoneCache;

	//
	// First up, work with the list of Materials we might need
	// Essentially, we need to end out with a list where each
	// material is unique by name and PolyFlags
	//
	AutoArray<int>		ExtraFlagsbyLWPoly(0);

	// We've got a list of Channels - go through twice
	// First time strip the bones and 'extra flag' UV meshes
	LC_Channel *lc = 0;
	unsigned int i = 0;
	for (	i = 0;
			lc = (LC_Channel *)LW_xprtFuncs->get_channel(inst,i);
			i++)
	{
		if (lc == 0)
			break;
				
		// Bones - add to the list
		if (lc->flavor == LISTEN_BONE)
		{
			BoneCache[BoneCache.Next()] = lc;
		}

		// Apply the 'extra' tags to appropriate polys
		if (lc->flavor == LISTEN_POLYS)
		{
			unsigned int matCount = 0;
			for (;matCount < MAX_EXTRA_UV_FLAGS;matCount++)
			{
				if (strcmp(lc->name,UV_ExtraFlags[matCount].Name))
				{
					continue;
				}

				// We matched one, add the flag to each polygon
				unsigned int tidx = lc->polygoncount;
				while (tidx-- > 0)
				{
					LC_Polygon *lp = LW_xprtFuncs->get_polygon(lc,tidx);
					if (lp == 0)
						continue;
					ExtraFlagsbyLWPoly[lp->index] |= UV_ExtraFlags[matCount].BitFlag;
				}
				break;
			}
		}
	}

	// Go through again and do just the tags and 'real' materials
	for (	i = 0;
			lc = (LC_Channel *)LW_xprtFuncs->get_channel(inst,i);
			i++)
	{
		if (lc == 0)
			break;
		// TAGs ... just the Weapon poly for now - treat as a mesh
		if (lc->flavor != LISTEN_TAG &&
				 lc->flavor != LISTEN_POLYS)
		{
			continue;
		}
		if (lc->flavor == LISTEN_TAG)
		{
			if (strcmp("tag_weapon",lc->name))
				continue;
		}

		// Skip any of the extra flag UV maps
		unsigned int matCount = 0;
		for (;matCount < MAX_EXTRA_UV_FLAGS;matCount++)
		{
			if (strcmp(lc->name,UV_ExtraFlags[matCount].Name))
			{
				continue;
			}
			break;
		}
		if (matCount != MAX_EXTRA_UV_FLAGS)
		{
			continue;
		}

		// Quick list of all the potential materials (match by name)
		AutoArray<int>		SameNameMaterials(0);

		// Find the base bits for this mesh
		// 2 options - it's named SKIN... with flags, or there is
		// a surface with the same name
		int basebits = 0;
		int seedmatidx  = cullBitsFromName(lc->name, &basebits);
		if (basebits == -1)
		{
			seedmatidx = -1;
			basebits = getBitsFromChannel(lc);
		}

		unsigned int matidx = 0;
		for (matidx = 0; matidx < model.MaterialCount();matidx++)
		{
			UNSKEL_VMaterial&	mat = model.Material(matidx);
			if (strcmp(mat.MaterialName,lc->name) == 0)
			{
				SameNameMaterials[SameNameMaterials.Next()] = matidx;
			}
		}

		// We want to add 1 wedge per delivered point per mesh
		AutoArray<int>		XlatVertexToWedge(-1);

		// Setup polygon info - polygons are all cumulative
		unsigned int tidx = lc->polygoncount;
		while (tidx-- > 0)
		{
			LC_Polygon *lp = LW_xprtFuncs->get_polygon(lc,tidx);
			if (lp == 0)
				continue;

			// Triangle numbers are cumulative
			UNSKEL_VTriangle&	tri = model.Triangle(model.TriangleCount());

			// Find the right Material for this polygon
			// - compare the basebits | extra bits with the material bits
			int mybits = (basebits | ExtraFlagsbyLWPoly[lp->index]);

			unsigned int m = 0;
			for (m = 0; m < SameNameMaterials.Next();m++)
			{
				if (model.Material(SameNameMaterials[m]).PolyFlags
						== mybits)
				{
					matidx = SameNameMaterials[m];
					break;
				}
			}
			if (m == SameNameMaterials.Next())
			{
				matidx = seedmatidx < 0 ? model.MaterialCount() : seedmatidx;

				// We didn't find one - make a new one
				UNSKEL_VMaterial&	mat = model.Material(matidx);
				strcpy(mat.MaterialName,lc->name);
				mat.TextureIndex = matCount;
				mat.AuxMaterial = 0;
				mat.AuxFlags = 0;
				mat.LodBias = 0;
				mat.LodStyle = 0;
				mat.PolyFlags = mybits;

				SameNameMaterials[SameNameMaterials.Next()] = matidx;
			}

			tri.MatIndex = matidx;	
			tri.AuxMatIndex = 0;		// ???

			// Points come in split on UV seams.  Unreal wants them merged
			// for the Vertexes, but split for the wedges
			for (int j = 0; j < 3; j++)
			{
				// Do the points here while we're doing this
				LC_VertexPosition *vp =  LW_xprtFuncs->get_vertex(lc,lp->vertex[j]);
				if (vp == 0)
					continue;

				// Add it if it hasn't been added already
				unsigned int pidx = LWPointToUnrealPoint.find(vp->lwid);
				if (pidx == LWPointToUnrealPoint.BAD_INDEX)
				{
					pidx = model.PointCount();
					LWPointToUnrealPoint[pidx] = vp->lwid;

					// Location (world ??)
					model.Point(pidx).Point = LW_TO_UNSKEL_Coords *
										Vector<float>(vp->start.location);
//						RotateByMatrix(Vector<float>(vp->start.location[0],
//													 vp->start.location[1],
//													 vp->start.location[2]),
//										LW_TO_UNSKEL_Coords,
//										model.Point(pidx).Point);
				}

				// Add a wedge for this point in this material (once each per mesh)
				int widx = XlatVertexToWedge[vp->index];
				if (widx == -1)
				{
					widx = model.WedgeCount();
					XlatVertexToWedge[vp->index] = widx;

					UNSKEL_VVertex& vtx = model.Wedge(model.WedgeCount());
					vtx.PointIndex = pidx;		// Unreal point index
					vtx.U = vp->texmap[0];
					vtx.V = 1 - vp->texmap[1];
					vtx.MatIndex = matidx;
				}
				tri.WedgeIndex[j] = widx;
			}

			// Define smoothing groups based on the poly's surface
			unsigned int smooth = SurfToSmoothingGroup.find(lp->surf);
			if (smooth == SurfToSmoothingGroup.BAD_INDEX)
			{
				smooth = SurfToSmoothingGroup.Next();
				SurfToSmoothingGroup[smooth] = lp->surf;
			}
			tri.SmoothingGroups = 1 << (smooth % 32);	// ???
		}

		// Setup vertex info by vertex
		int vidx = lc->vertexcount;
		while (vidx-- > 0)
		{	// Locations
			LC_VertexPosition *vp = LW_xprtFuncs->get_vertex(lc,vidx);
			if (vp == 0)
				continue;

			// See if we've already tagged this one in another mesh
			unsigned int pidx = LWPointToUnrealPoint.find(vp->lwid);
			if (pidx == LWPointToUnrealPoint.BAD_INDEX)
			{
				pidx = model.PointCount();
				LWPointToUnrealPoint[pidx] = vp->lwid;

				// Location (world ??)
				model.Point(pidx).Point = LW_TO_UNSKEL_Coords *
									Vector<float>(vp->start.location);
//					RotateByMatrix(Vector<float>(vp->start.location[0],
//												 vp->start.location[1],
//												 vp->start.location[2]),
//									LW_TO_UNSKEL_Coords,
//									model.Point(pidx).Point);
			}
		}
	}
}

// This finishes things off.
// Since we have all the info now, we're going to
// do some rearranging of things ...
static void save(LC_Model mdl)
{
	ModelScratchPad *scratch = (ModelScratchPad *)mdl;

	UnrealData				*data	= scratch->Data;
	UnrealSkeletalModel&	model	= scratch->Model;
	LC_FrameLoop			**loops	= scratch->Loops;

	AutoArray<LWSurfaceID>&		SurfToSmoothingGroup = scratch->SurfToSmoothingGroup;
	AutoArray<LWPntID>&			LWPointToUnrealPoint = scratch->LWPointToUnrealPoint;
	AutoArray<float>&			WeightMapInfluences = scratch->WeightMapInfluences;
	AutoArray<LC_Channel *>&	BoneCache = scratch->BoneCache;

	// Reorder the bones - 'ROOT' bones are first
	unsigned int i = 0;
	AutoArray<LC_Channel *>	BoneList(0);
	for (i = 0; i < BoneCache.Next(); i ++)
	{
		if (BoneCache[i] == 0)
			continue;

		unsigned int j = 0;
		for (; j < BoneCache.Next(); j ++)
		{
			if (BoneCache[j] == 0)
				continue;
			if (BoneCache[j]->lwid
					== BoneCache[i]->lwparentid)
				break;
		}
		if (j == BoneCache.Next())
		{	// This one didn't have any children
			BoneList[BoneList.Next()] = BoneCache[i];
			BoneCache[i] = 0;
		}
	}

	// Now just stack on the rest
	for (i = 0; i < BoneList.Next(); i ++)
	{
		for (unsigned int j = 0; j < BoneCache.Next(); j ++)
		{
			if (BoneCache[j] == 0)
				continue;
			if (BoneCache[j]->lwparentid == BoneList[i]->lwid)
			{	// This one is a child, add it on
				BoneList[BoneList.Next()] = BoneCache[j];
				BoneCache[j] = 0;
			}
		}
	}
	
	// Reprocess the bones only
	for (i = 0; i < BoneList.Next(); i ++)
	{
		LC_Channel *lc = BoneList[i];
		if (lc == 0)
			break;
		
		// Grab bone stuff
		unsigned int bidx = model.BoneCount();
		UNSKEL_FNamedBoneBinary& bone = model.Bone(bidx);
		strcpy(bone.Name,lc->name);
		bone.Flags = 0;

		bone.NumChildren = 0;
		bone.ParentIndex = 0;
		for (unsigned int j = 0; j < BoneList.Next(); j++)
		{	// Count each one that has the current one as a parent
			if (BoneList[j]->lwparentid == lc->lwid)
			{
				++bone.NumChildren;
			}

			// Remember which one is the current one's parent
			if (BoneList[j]->lwid == lc->lwparentid)
			{
				bone.ParentIndex = j;
			}
		}

		LC_ObjectPosition *lorest = LW_xprtFuncs->get_rest_position(lc);

		// Rest Angle
		Quaternion<float> Q = hpb_to_unreal_quat(lorest->hpb,bidx == 0);
		bone.BonePos.Orientation = FQuat(Q[X], Q[Y], Q[Z],
										 Q[W]);
		bone.BonePos.Position = LW_TO_UNSKEL_Coords * Vector<float>(lorest->xyz);

//		Vector<float>	v_lw(lorest->xyz);
//		RotateByMatrix(v_lw,LW_TO_UNSKEL_Coords,bone.BonePos.Position);

//		bone.BonePos.Length =
//		bone.BonePos.YSize = 
//		bone.BonePos.ZSize =

		// Setup vertex weights
		int vidx = lc->vertexcount;
		while (vidx-- > 0)
		{
			LC_VertexPosition *vp = LW_xprtFuncs->get_vertex(lc,vidx);
			if (vp == 0)
				continue;

			// Find this point's local point index
			unsigned int pidx = LWPointToUnrealPoint.find(vp->lwid);
			if (pidx == LWPointToUnrealPoint.BAD_INDEX)
			{	// This is pretty bad ...
				continue;
			}

			// Don't bother confusing things
			if (vp->influence == 0)
				continue;

			// Add a weight map for it
			UNSKEL_VRawBoneInfluence& wght = model.Weight(model.WeightCount());
			wght.Weight = vp->influence;
			wght.PointIndex = pidx;
			wght.BoneIndex = bidx;

			// Remember we added one
			WeightMapInfluences[pidx] += vp->influence;
		}

		// ANIMATIONS - UNSKEL_Animation
		// We need to be able to grab the frame names for this

		// See which frames we need to deal with ...
		int frameUNR = 0;
		for (unsigned int loopidx = 0; loops[loopidx]; loopidx ++)
		{
			// Fill in the animation info (if we havn't already)
			UNSKEL_Animation&	anim = model.Animation(loopidx);
			if (anim.Name[0] == 0)
			{	// Fill it in with info - find/build our matching loop
				UnrealLoop *unrloop = SpecificLoopData[loops[loopidx]->ID];

				strcpy(anim.Name, unrloop->LoopName);
				strcpy(anim.Group,unrloop->LoopGroup);

				anim.RootInclude			= unrloop->RootInclude;
				anim.KeyCompressionStyle	= 1;						// ???
				anim.KeyReduction			= (float)unrloop->KeyReduction;
				anim.AnimRate				= (float)unrloop->LoopRate;
				anim.FirstRawFrame			= 0;

				// Leave at full for now
				anim.KeyQuotum				= loops[loopidx]->length * BoneList.Next();	

				// FIXME - for now, all bones are in all animations
				anim.StartBone				= 0;
				anim.TotalBones				= BoneList.Next();
			}

			for (unsigned int fridx = 0;
					fridx < loops[loopidx]->length;
					fridx++, frameUNR++)
			{
				unsigned int frameLW = loops[loopidx]->lwframes[fridx];
				LC_ObjectPosition *lo = LW_xprtFuncs->get_position_at_frame(lc,frameLW);
				if (lo == 0)
					continue;

				// Add in keys - ((one per bone) per frame)
				UNSKEL_VQuatAnimKey&	key = anim.KeyFrame((fridx * BoneList.Next()) + bidx);

				// Find the bone's position
				key.Position = LW_TO_UNSKEL_Coords * Vector<float>(lo->xyz);
//				RotateByMatrix(	Vector<float>(lo->xyz[0],lo->xyz[1],lo->xyz[2]),
//								LW_TO_UNSKEL_Coords,
//								key.Position);

				// And its orientation
				Quaternion<float> Q = hpb_to_unreal_quat(lo->hpb,bidx == 0);
				key.Orientation = FQuat(Q[X], Q[Y], Q[Z],
										Q[W]);

				key.Time = 1.0f;
			}
			anim.NumRawFrames		= anim.KeyQuotum / BoneList.Next();
			anim.TrackTime			= anim.KeyQuotum / BoneList.Next();
		}
	}

	// Normalize any of the weightmaps that need normalizing
	for (unsigned int j = 0; j < model.WeightCount(); j ++)
	{
		UNSKEL_VRawBoneInfluence& wght = model.Weight(j);
		wght.Weight /= WeightMapInfluences[wght.PointIndex];
	}
	
	char name[512];
	if (scratch->Data->AlwaysSaveMesh)
	{
		sprintf(name,"%s%sModels%s%s",scratch->Data->Output,
				DIR_SEPARATOR,
				DIR_SEPARATOR,scratch->Data->MeshName);
		scratch->Model.WriteSkeletontoDisk(name);
	}
	if (scratch->Data->AlwaysSaveAnim)
	{
		sprintf(name,"%s%sModels%s%s",scratch->Data->Output,
				DIR_SEPARATOR,
				DIR_SEPARATOR,scratch->Data->AnimName);
		scratch->Model.WriteAnimationtoDisk(name);
	}

	delete scratch;
}

// Description
static char *describe(LC_Instance inst)
{
	UnrealData *data = (UnrealData *)inst;
	if (data->MeshName != 0 &&
		data->MeshName[0] != 0)
		return data->MeshName;

	return data->AnimName;
}

////////////////////////////////////
// XPanel functions - this is a VIEW panel
////////////////////////////////////

enum {  TABS_MAIN = 0x8000, TABS_LOOPS,

		// General data
		ID_OUTPUTFOLDER, 

		// Mesh controls
		GROUPID_MESH,
		ID_MESHNAME, ID_CHKSAVEMESH,

		// Mesh controls
		GROUPID_ANIM,
		ID_ANIMNAME, ID_CHKSAVEANIM,

		// Script controls
		GROUPID_SCRIPT,
		ID_SCRIPTNAME, ID_CHKSAVESCRIPT,

		// Per-loop controls
		GROUPID_LOOP,
		ID_LOOPNAME, ID_GROUPNAME, ID_ANIMRATE,
		ID_STARTBONE, ID_KEYREDUCE, ID_ROOTINCLUDE
};

static void ClickButton (LWXPanelID pan, int cid)
{
}

// Overall value
#define STR_OutputFolder_TEXT	"Base Output Folder"

// Mesh only controls
#define STR_MeshName_TEXT		"Mesh Name"
#define STR_ChkSaveMesh_TEXT	"Save w/ All"

// Anim only controls
#define STR_AnimName_TEXT		"Animation Name"
#define STR_ChkSaveAnim_TEXT	"Save w/ All"

// Script only controls
#define STR_ScriptName_TEXT		"Scripting Setup File"
#define STR_ChkSaveScript_TEXT	"Save w/ All"

// Per frame loop controls
#define STR_LoopName_TEXT		"Sequence Name"
#define STR_GroupName_TEXT		"Group Name"
#define STR_StartBone_TEXT		"Start Bone"
#define STR_RootInclude_TEXT	"Separate Root Track"
#define STR_KeyReduce_TEXT		"Key Reduction"
#define STR_AnimRate_TEXT		"Rate (frames/sec)"

static void *xgetval( void *inst, unsigned long vid )
{
	// Find the index for the selected loop
	UnrealData *data = (UnrealData *)inst;
	UnrealLoop *loop = data->ActiveLoop < 0 ? 0 : SpecificLoopData[data->ActiveLoop];

	void *v = 0;
	if (vid > GROUPID_LOOP && loop == 0)
	{	// nothing there, return
		return v;
	}

	// Return the index that matches the ref we have
	switch ( vid ) {
      case ID_OUTPUTFOLDER:
		v = (void *)&(data->Output);
		break;

// Mesh controls
      case ID_MESHNAME:
		v = (void *)&(data->MeshName);
		break;
      case ID_CHKSAVEMESH:
		v = (void *)&(data->AlwaysSaveMesh);
		break;

// Script controls
      case ID_SCRIPTNAME:
		v = (void *)&(data->ScriptName);
		break;
      case ID_CHKSAVESCRIPT:
		v = (void *)&(data->AlwaysSaveScript);
		break;

// Animation controls
      case ID_ANIMNAME:
		v = (void *)&(data->AnimName);
		break;
      case ID_CHKSAVEANIM:
		v = (void *)&(data->AlwaysSaveAnim);
		break;

// Individual Animation controls
	  case ID_LOOPNAME:
		v = (void *)&(loop->LoopName);
		break;
      case ID_GROUPNAME:
		v = (void *)&(loop->LoopGroup);
		break;
      case ID_ANIMRATE:
		v = (void *)&(loop->LoopRate);
		break;
      case ID_STARTBONE:
		v = (void *)&(loop->StartBone);
		break;
      case ID_KEYREDUCE:
		v = (void *)&(loop->KeyReduction);
		break;
      case ID_ROOTINCLUDE:
		v = (void *)&(loop->RootInclude);
		break;
      default:
		  ;
	}

	return v;
}

static LWXPRefreshCode xsetval( void *inst, unsigned long vid, void *value )
{
	// Use the active loop
	UnrealData *data = (UnrealData *)inst;
	UnrealLoop *loop = data->ActiveLoop < 0 ? 0 :SpecificLoopData[data->ActiveLoop];
	if (vid > GROUPID_LOOP && loop == 0)
	{	// nothing there, return
		return LWXPRC_DFLT;;
	}
	
	// Return the index that matches the ref we have
	switch ( vid ) {
      case ID_OUTPUTFOLDER:
		strcpy((char *)data->Output, (char *)value);
		break;

// Mesh controls
      case ID_MESHNAME:
		strcpy((char *)data->MeshName, (char *)value);
		break;
      case ID_CHKSAVEMESH:
		data->AlwaysSaveMesh = *(( int * ) value );
		break;

// Animation controls
      case ID_ANIMNAME:
		strcpy((char *)data->AnimName, (char *)value);
		break;
      case ID_CHKSAVEANIM:
		data->AlwaysSaveAnim = *(( int * ) value );
		break;

// Script controls
      case ID_SCRIPTNAME:
		strcpy((char *)data->ScriptName, (char *)value);
		break;
      case ID_CHKSAVESCRIPT:
		data->AlwaysSaveScript = *(( int * ) value );
		break;

// Individial Animation (loop) controls
      case ID_LOOPNAME:
		strcpy((char *)loop->LoopName, (char *)value);
		break;
      case ID_GROUPNAME:
		strcpy((char *)loop->LoopGroup, (char *)value);
		break;
      case ID_ANIMRATE:
		loop->LoopRate = *(( double * ) value );
		break;
      case ID_STARTBONE:
		loop->StartBone = *(( int * ) value );
		break;
      case ID_KEYREDUCE:
		loop->KeyReduction = *(( double * ) value );
		break;
      case ID_ROOTINCLUDE:
		loop->RootInclude = *(( int * ) value );
		break;

	  default:
			return LWXPRC_NONE;
	}
	return LWXPRC_DFLT;
}

// When this is destroyed, delete the model
XCALL_( void )
xpanelDestroy(void *inst)
{
	UnrealData *data = (UnrealData *)inst;
	delete data;
}

// Potentially feeding with 'seed' data
static LWXPanelID buildpanel(void *seeddata)
{
	UnrealData *data = (UnrealData *)seeddata;
	if (data == (UnrealData *)0)
	{
		data = new UnrealData;
	}
	
	static LWXPanelControl xctl[] = {
		{ ID_OUTPUTFOLDER,	STR_OutputFolder_TEXT,	"sFileName" },

		{ ID_MESHNAME,		STR_MeshName_TEXT,		"string" },
		{ ID_CHKSAVEMESH,	STR_ChkSaveMesh_TEXT,	"iBoolean" },

		{ ID_ANIMNAME,		STR_AnimName_TEXT,		"string" },
		{ ID_CHKSAVEANIM,	STR_ChkSaveAnim_TEXT,	"iBoolean" },

		{ ID_SCRIPTNAME,	STR_ScriptName_TEXT,	"string" },
		{ ID_CHKSAVESCRIPT,	STR_ChkSaveScript_TEXT,	"iBoolean" },

		// Add a single set of 'starter' Loop items for initial setup
		{ ID_LOOPNAME,		STR_LoopName_TEXT,		"string" },
		{ ID_GROUPNAME,		STR_GroupName_TEXT,		"string" },
		{ ID_ANIMRATE,		STR_AnimRate_TEXT,		"float" },
		{ ID_STARTBONE,		STR_StartBone_TEXT,		"integer" },
		{ ID_KEYREDUCE,		STR_KeyReduce_TEXT,		"float" },
		{ ID_ROOTINCLUDE,	STR_RootInclude_TEXT,	"iBoolean" },
		{ 0 }
	};

	static LWXPanelDataDesc xdata[] = {
		{ ID_OUTPUTFOLDER,	STR_OutputFolder_TEXT,	"string" },

		{ ID_MESHNAME,		STR_MeshName_TEXT,		"string" },
		{ ID_CHKSAVEMESH,	STR_ChkSaveMesh_TEXT,	"integer" },

		{ ID_ANIMNAME,		STR_AnimName_TEXT,		"string" },
		{ ID_CHKSAVEANIM,	STR_ChkSaveAnim_TEXT,	"integer" },

		{ ID_SCRIPTNAME,	STR_ScriptName_TEXT,	"string" },
		{ ID_CHKSAVESCRIPT,	STR_ChkSaveScript_TEXT,	"integer" },

		// One group of these per 'Loop'
		{ ID_LOOPNAME,		STR_LoopName_TEXT,		"string" },
		{ ID_GROUPNAME,		STR_GroupName_TEXT,		"string" },
		{ ID_ANIMRATE,		STR_AnimRate_TEXT,		"float" },
		{ ID_STARTBONE,		STR_StartBone_TEXT,		"integer" },
		{ ID_KEYREDUCE,		STR_KeyReduce_TEXT,		"float" },
		{ ID_ROOTINCLUDE,	STR_RootInclude_TEXT,	"integer" },

		{ 0 }
	};


	// TRy and arrange some things
	static LWXPanelHint xhint[] = {
		// Set some limits
		XpMIN(ID_ANIMRATE,0),XpMIN(ID_STARTBONE,0),XpMIN(ID_KEYREDUCE,0),
		XpXREQCFG(ID_OUTPUTFOLDER,
					LWXPREQ_DIR,
					"Root folder for Build",
					0),

		// Mesh group
		XpGROUP_(GROUPID_MESH),
			XpH(ID_MESHNAME),
			XpH(ID_CHKSAVEMESH),
		XpEND,
		XpLABEL(GROUPID_MESH,"Mesh Setup"),

		// Animation Group
		XpGROUP_(GROUPID_ANIM),
			XpH(ID_ANIMNAME),
			XpH(ID_CHKSAVEANIM),
		XpEND,
		XpLABEL(GROUPID_ANIM,"Animation Manager"),

		// Script group
		XpGROUP_(GROUPID_SCRIPT),
			XpH(ID_SCRIPTNAME),
			XpH(ID_CHKSAVESCRIPT),
		XpEND,
		XpLABEL(GROUPID_SCRIPT,"Script Setup"),

		// Tabs - 1 row with mesh/animation/script
		XpTABS_(TABS_MAIN),
			XpH(GROUPID_MESH),
			XpH(GROUPID_ANIM),
			XpH(GROUPID_SCRIPT),
		XpEND,

		// Loop hints
		XpGROUP_(GROUPID_LOOP),
			XpH(ID_LOOPNAME),
			XpH(ID_GROUPNAME),
			XpH(ID_ANIMRATE),
			XpH(ID_STARTBONE),
			XpH(ID_KEYREDUCE),
			XpH(ID_ROOTINCLUDE),
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

	// Stash the XPanelID in this data for later
	data->XPanelID = xpanel;

	return xpanel;
};

// Update loops - negative index means clear it out
static void activeloop(LC_Instance inst, LC_FrameLoop *loop)
{	// Make sure we have a Loop object for each
	UnrealLoop *l = 0;
	UnrealData *data = (UnrealData *)inst;
	if (loop->ID == LOOP_END)
	{	// no selected loop
		data->ActiveLoop = -1;
	}
	else if (loop->ID < 0)
	{	// removing
		l = SpecificLoopData[-loop->ID];
		if (l != 0)
			delete l;
		SpecificLoopData[-loop->ID] = 0;
	}
	else
	{	// adding/updating
		l = SpecificLoopData[loop->ID];
		if (l == 0)
			l = new UnrealLoop(loop->ID);
		SpecificLoopData[loop->ID] = l;
		if (l->LoopName[0] == 0 && loop->Name[0] != 0)
			strcpy(l->LoopName,loop->Name);
		data->ActiveLoop = loop->ID;
	}

	// refresh everything
	LW_xpanFuncs->viewRefresh (data->XPanelID);
}
////////////////////////////////////

////////////////////////////////////
//Input / Output functions
////////////////////////////////////


// Lump values for Load/Save
#define ID_USKD  LWID_( 'U','S','K','D' )
#define ID_USKL  LWID_( 'U','S','K','L' )
static LWBlockIdent idmain[] = {
	ID_USKD, "USkelData",
	ID_USKL, "USkelLoop",
	0
};

#define ID_USKG  LWID_( 'U','S','K','G' )
#define ID_USKM  LWID_( 'U','S','K','M' )
#define ID_USKA  LWID_( 'U','S','K','A' )
#define ID_USKS  LWID_( 'U','S','K','S' )
static LWBlockIdent idloopelement[] = {
	ID_USKG, "General",
	ID_USKM, "Model",
	ID_USKA, "Animation",
	ID_USKS, "Script",
	0
};

// Save data
static void store( const LWSaveState *save, void *data )
{
	UnrealData *d = (UnrealData *)data;
	short s[5];
	float f[5];
	f[0] = PLUGINVERSION;
	LWSAVE_FP(save,f,1);			// Spit out version

	if (data == 0)
	{	// Save generic data - roll through the loops
		for (unsigned int loopidx = 0; loopidx < SpecificLoopData.Next(); loopidx ++)
		{
			UnrealLoop *l = SpecificLoopData[loopidx];
			if (l == 0)
				continue;
			LWSAVE_BEGIN( save, &idmain[ 1 ], 1 );	// ID_USKL
			s[0] = l->index;
			s[1] = l->StartBone;
			s[2] = l->RootInclude;
			s[3] = s[4] = 0;
			 LWSAVE_I2( save, s, 5);
			 LWSAVE_STR(save, l->LoopName);
			 LWSAVE_STR(save, l->LoopGroup);
			f[0] = (float)l->LoopRate;
			f[1] = (float)l->KeyReduction;
			f[2] = f[3] = f[4] = 0.0f;
			 LWSAVE_FP( save, f, 5);

			LWSAVE_END( save );						// ID_USKL
		}
	}
	else
	{	// Save model data
		LWSAVE_BEGIN( save, &idmain[ 0 ], 0 );		// ID_USKD
		{
			s[1] = s[2] = 0;
			LWSAVE_BEGIN( save, &idloopelement[ 0 ], 1 );
			 LWSAVE_STR(save, d->Output);
			LWSAVE_END( save );

			LWSAVE_BEGIN( save, &idloopelement[ 1 ], 1 );
			 s[0] = d->AlwaysSaveMesh;
			 LWSAVE_I2(save, s,3);
			 LWSAVE_STR(save, d->MeshName);
			LWSAVE_END( save );

			LWSAVE_BEGIN( save, &idloopelement[ 2 ], 1 );
			 s[0] = d->AlwaysSaveAnim;
			 LWSAVE_I2(save, s,3);
			 LWSAVE_STR(save, d->AnimName);
			LWSAVE_END( save );

			LWSAVE_BEGIN( save, &idloopelement[ 3 ], 1 );
			 s[0] = d->AlwaysSaveScript;
			 LWSAVE_I2(save, s,3);
			 LWSAVE_STR(save, d->ScriptName);
			LWSAVE_END( save );
		}
		LWSAVE_END( save );							// ID_USKD
	}
}

// Load data
static void *load( const LWLoadState *load )
{

	UnrealData *data = 0;
	UnrealLoop *loop = 0;
	short s[5];
	float f[5];
	LWLOAD_FP(load,f,1);		// Snag version

	while ( LWID id = LWLOAD_FIND( load, idmain ))
	{
		switch ( id )
		{
		case ID_USKD:			// New model
			data = new UnrealData();
			while ( LWID mid = LWLOAD_FIND( load, idloopelement ))
			{
				switch ( mid ) {
				case ID_USKG:		// General
					LWLOAD_STR(load,data->Output,256);
					break;
				case ID_USKM:		// Model
					LWLOAD_I2(load,s,3);
					LWLOAD_STR(load,data->MeshName,LOOP_NAME_MAX);
					data->AlwaysSaveMesh = s[0];
					break;
				case ID_USKA:		// Animation
					LWLOAD_I2(load,s,3);
					LWLOAD_STR(load,data->AnimName,LOOP_NAME_MAX);
					data->AlwaysSaveAnim = s[0];
					break;
				case ID_USKS:		// Scripting
					LWLOAD_I2(load,s,3);
					LWLOAD_STR(load,data->ScriptName,LOOP_NAME_MAX);
					data->AlwaysSaveScript = s[0];
					break;
				}
				LWLOAD_END( load );
			}
			break;

		case ID_USKL:			// Per-Loop data
			 LWLOAD_I2(load,s,5);
			loop = new UnrealLoop(s[0]);
			loop->StartBone = s[1];
			loop->RootInclude = s[2];
			 LWLOAD_STR(load,loop->LoopName,LOOP_NAME_MAX);
			 LWLOAD_STR(load,loop->LoopGroup,LOOP_NAME_MAX);
			 LWLOAD_FP(load,f,5);
			loop->LoopRate = f[0];
			loop->KeyReduction = f[1];
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
	"Unreal Skeletal",
	GLOBALPLUGINNAME
};

LWMRBExportType *getFunc()
{
	return &_type;
}



/**************************************
 *
 *  ObjImp.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Handles the real meat of the program for importing
 *  Unreal Skeletal formats
 *
 *  Unreal Tounament is a trademark of Epic MegaGames, Inc.
 *
 **************************************/

#include "sys_math.h"

extern "C"
{
#include "ImportUnrealSkel.h"
}

#include "ParseUnrealSkl.h"

static unsigned char surfacechunk[20] =
{
	'C','O','L','R',	// 4 letter chunk tag
		0,14,			// Size of the chunk
		0x3F,0x00,0x00,0x00,	// Red   (float across 4 bytes)
		0x3F,0x00,0x00,0x00,	// Green (float across 4 bytes)
		0x3F,0x00,0x00,0x00,	// Blue  (float across 4 bytes)
		0,0				// Attached envelope Vidx (none)
};

Vector<float> RefigureBone(UNSKEL_FNamedBoneBinary& bone, Vector<float> V)
{
	// Rotate by parent's Orientation
	Vector<float>	V2 = V;
	Vector<float>	Vres;
	Quaternion<float> Q(bone.BonePos.Orientation.X,
						bone.BonePos.Orientation.Y,
						bone.BonePos.Orientation.Z,
						bone.BonePos.Orientation.W);
	Vres = Q.getMatrix() * V2;

	// Push into parent's coordinate space
	Vres += bone.BonePos.Position;

	return Vres;
}

void FillLWVector(Vector<float>& start, LWFVector& res)
{
	static Matrix<float> Zrot(	Vector<float>(-1, 0, 0),
								Vector<float>( 0,-1, 0),
								Vector<float>( 0, 0, 1));
	// Each vector needs to be rotated into LW
	Vector<float> tmp = ~LW_TO_UNSKEL_Coords * start;
	res[0] = tmp[0];
	res[1] = tmp[1];
	res[2] = tmp[2];
}

void Build(LWObjectImport *funcs, UnrealSkeletalModel& model)
{
	LWFVector		pivot = {0.0f};
	int				pointcount = model.PointCount();
	unsigned int	idx = 0;

	// Map pointindex to LWPntID (points + bone points)
	LWPntID	*PointIDs = new LWPntID[pointcount];
//										+ (model.BoneCount() * 2)];

	// Hang onto material names as well
	char **SurfaceNames = new char *[model.MaterialCount() +1];

	// ===Now build the object inside Lightwave===
	funcs->layer(funcs->data, 0,0);
	funcs->pivot(funcs->data, pivot );

	// Create some Surfaces
	for (idx = 0; idx < model.MaterialCount(); idx ++)
	{
		UNSKEL_VMaterial&	m = model.Material(idx);

		// Build an Unreal-happy name: SKIN##[flags]
		SurfaceNames[idx] = new char[128];

		sprintf(SurfaceNames[idx],"SKIN%2.2d",idx);

		for (int flagidx = 0; flagidx < MAX_UT_MATERIAL_CHUNKS; flagidx++)
		{
			if ((flagidx < MAX_EXCLUSIVE_MAT_CHUNK 
				 && (m.PolyFlags & 0xFF) == UT_NameChunks[flagidx].BitFlag)
				|| (m.PolyFlags & UT_NameChunks[flagidx].BitFlag) != 0)
			{
				sprintf(SurfaceNames[idx] + strlen(SurfaceNames[idx]),
							"_%s",
							UT_NameChunks[flagidx].Name);
			}
		}

//		sprintf(SurfaceNames[idx],"Mat%2.2d-%s",idx,m.MaterialName);
		funcs->surface(funcs->data,
						SurfaceNames[idx],(const char *)0,	// name, parent name
						20,(void *)surfacechunk);			// size of chunk, chunk
		SurfaceNames[idx +1] = 0;
	}

	// Add points ...
	for (idx = 0; idx < pointcount; idx ++)
	{
		UNSKEL_VPoint&	p = model.Point(idx);
		Vector<float>	V(p.Point);
		Vector<float>	res;

		// Fix coordinate system
		res = ~LW_TO_UNSKEL_Coords * V;

		LWFVector		ptBuf;
		ptBuf[0] = res[0];
		ptBuf[1] = res[1];
		ptBuf[2] = res[2];

		PointIDs[idx] = funcs->point(funcs->data,ptBuf);
	}


	char **UPs = 0;
//	UPs = new char *[model.BoneCount()];
/*
	// If there are bones, let's add some skelegons in.
	// Skelegons consist of 2 point polys of type LWPOLTYPE_BONE that
	// define the +z (long) side, a PTag 'BONE' with the name, and a 
	// PTag 'BNUP' that helps define the pitch plane
	for (idx = 0; idx < model.BoneCount(); idx ++)
	{
		UNSKEL_FNamedBoneBinary&	bone = model.Bone(idx);

		// Get the location out of the bone ... and another point it points at
		LWFVector		ptBuf;
		Vector<float>	Vstart = bone.BonePos.Position;
		Vector<float>   Vend = LW_TO_UNSKEL_Coords * Vector<float>(0,0,10);
		Vector<float>	Vup = LW_TO_UNSKEL_Coords * Vector<float>(0,10,0);

		// Find the position of the bone and move it into 
		// its parent's coordinate space (if this isn't the root one)
		int pidx = bone.ParentIndex;
		while (idx != 0)
		{
			UNSKEL_FNamedBoneBinary&	pbone = model.Bone(pidx);

			// Rotate by parent's Orientation
			Vstart	= RefigureBone(pbone,Vstart);
			Vend	= RefigureBone(pbone,Vend);
			Vup		= RefigureBone(pbone,Vup);

			if (pidx == 0)
				break;

			// Get that parent's parent and repeat
			pidx = pbone.ParentIndex;
		}

		// Move result to LW coordinates and add the points
		FillLWVector(Vstart,ptBuf);
		PointIDs[pointcount++] = funcs->point(funcs->data,ptBuf);

		FillLWVector(Vend,ptBuf);
		PointIDs[pointcount++] = funcs->point(funcs->data,ptBuf);

		FillLWVector(Vup,ptBuf);
		UPs[idx] = new char[128];
		sprintf(UPs[idx],"%f %f %f",ptBuf[0],ptBuf[1],ptBuf[2]);
	}
*/
	// Then add polys ...
	for (idx = 0; idx < model.TriangleCount(); idx ++)
	{
		UNSKEL_VTriangle&	tri = model.Triangle(idx);

		LWPntID 	ptBuf[3];

		// Triangles have wedge index points
		ptBuf[0] = PointIDs[model.Wedge(tri.WedgeIndex[0]).PointIndex];
		ptBuf[1] = PointIDs[model.Wedge(tri.WedgeIndex[1]).PointIndex];
		ptBuf[2] = PointIDs[model.Wedge(tri.WedgeIndex[2]).PointIndex];

		LWPolID polyid = funcs->polygon(funcs->data,
										LWPOLTYPE_FACE,
										0, 3,
										ptBuf);
		// Add in the VMAPs on a per-vertex basis based on the wedge
		int uvMatIdx = 0;
		for (int j = 0; j < 3; j++)
		{
			UNSKEL_VVertex&		wedge = model.Wedge(tri.WedgeIndex[j]);
			float	vbuf[2];
			vbuf[0] = wedge.U;
			vbuf[1] = 1.0 - wedge.V;	// why is this upside down?

			uvMatIdx = wedge.MatIndex;
			funcs->vmap(funcs->data,
						LWVMAP_TXUV,2,	// type, dimension
						SurfaceNames[wedge.MatIndex]);
			funcs->vmapPDV(funcs->data,ptBuf[j],polyid,vbuf);
		}

		// Hook the poly to the surface
		funcs->polTag(funcs->data, polyid,
						LWPTAG_SURF,	// type
						SurfaceNames[uvMatIdx]);

	}
/*
	// And a bone poly indicating the bone (direction, etc)
	pointcount = model.PointCount();
	for (idx = 0; idx < model.BoneCount(); idx ++)
	{
		UNSKEL_FNamedBoneBinary&	bone = model.Bone(idx);
		LWPntID 	ptBuf[3];
		LWPolID		polyid;

		// 2 points for each bone [origin][point on axis]
		ptBuf[0] = PointIDs[pointcount++];
		ptBuf[1] = PointIDs[pointcount++];

//		ptBuf[1] = PointIDs[pointcount + (idx * 2)];
//		ptBuf[0] = PointIDs[pointcount + (bone.ParentIndex * 2)];

		polyid = funcs->polygon(funcs->data,
								LWPOLTYPE_BONE,
								0, 2,
								ptBuf);
		funcs->polTag(funcs->data, polyid,
						LWPOLTYPE_BONE,	// type
						bone.Name);

		// And the up vector (got this from the file)
#define LWPOLTAG_BNUP  LWID_('B','N','U','P')
		funcs->polTag(funcs->data, polyid,
						LWPOLTAG_BNUP,	// type
						UPs[idx]);
		// Attach a weight map (got this from the file)
#define LWPOLTAG_BNWT  LWID_('B','N','W','T')
		funcs->polTag(funcs->data, polyid,
						LWPOLTAG_BNWT,	// type
						bone.Name);
	}
*/
	// And add weightmaps for each bone
	for (idx = 0; idx < model.WeightCount(); idx ++)
	{
		UNSKEL_VRawBoneInfluence&	wght = model.Weight(idx);
		UNSKEL_FNamedBoneBinary&	bone = model.Bone(wght.BoneIndex);
		float	vbuf[1];
		vbuf[0] = wght.Weight;
		
		funcs->vmap(funcs->data,
					LWVMAP_WGHT,1,	// type, dimension
					bone.Name);
		funcs->vmapVal(funcs->data,PointIDs[wght.PointIndex],vbuf);
	}

	if (PointIDs) delete PointIDs;

	if (UPs)
	{
		for (idx = 0; idx < model.BoneCount(); idx ++)
		{
			if (UPs[idx])			delete UPs[idx];
		}
		delete UPs;
	}

	if (SurfaceNames)
	{
		for (idx = 0; idx < model.MaterialCount(); idx ++)
		{
			if (SurfaceNames[idx])	delete SurfaceNames[idx];
		}
		delete SurfaceNames;
	}
}

int Load_PSA(LWObjectImport *funcs)
{
	UnrealSkeletalModel		model(funcs->filename);
	Build(funcs, model);

	// Everything worked OK
	funcs->result = LWOBJIM_OK;
	return AFUNC_OK;
}

int Load_PSK(LWObjectImport *funcs)
{
	UnrealSkeletalModel		model(funcs->filename);
	Build(funcs, model);

	// Everything worked OK
	funcs->result = LWOBJIM_OK;
	return AFUNC_OK;
}


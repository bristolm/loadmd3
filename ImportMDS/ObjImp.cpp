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
#include "ImportMDS.h"
}

#include "ParseMDS.h"

static unsigned char surfacechunk[20] =
{
	'C','O','L','R',	// 4 letter chunk tag
		0,14,			// Size of the chunk
		0x3F,0x00,0x00,0x00,	// Red   (float across 4 bytes)
		0x3F,0x00,0x00,0x00,	// Green (float across 4 bytes)
		0x3F,0x00,0x00,0x00,	// Blue  (float across 4 bytes)
		0,0				// Attached envelope Vidx (none)
};

Vector<float>	*BoneSpots = 0;
Matrix<float>	*BoneDirs = 0;

void Build(LWObjectImport *funcs, MDS& model, GUIData& user)
{
	LWFVector		pivot = {0.0f};

	// ===Now build the object inside Lightwave===

	// Problem here is that the vertex positions are relative to
	// the Bone positions, and since each vertex may hook to more than 1 
	// bone .... it's a little weird

	// Use the user's frame to determine a base

	BoneSpots = new Vector<float>[model.BoneCount()];
	BoneDirs = new Matrix<float>[model.BoneCount()];

	unsigned int	bidx = 0;
	for (bidx = 0; bidx < model.BoneCount(); bidx ++)
	{
		MDS_BoneFrame& bonefr = model.Frame(user.FrameForImport).Bone(bidx);

		BoneSpots[bidx] = bonefr.Position;
		BoneDirs[bidx] = Matrix<float>(bonefr.Orientation);

		// Augment by parent
		int pbidx = model.Bone(bidx).ParentBoneIndex;
		if (pbidx < 0)
			continue;

		// Unrotate and move to a non-boned position
		BoneDirs[bidx] = BoneDirs[bidx] * ~BoneDirs[pbidx];

		BoneSpots[bidx] = ~BoneDirs[pbidx] * BoneSpots[bidx];
		BoneSpots[bidx] = BoneSpots[bidx] + BoneSpots[pbidx];
	}

	// For each LOD, run through the same crap,
	unsigned int	lodidx = 0;
	for (lodidx = 0; lodidx < model.LODCount(); lodidx ++)
	{
		MDS_LOD& lod = model.LOD(lodidx);

		// Work on another layer for eacn LOD
		funcs->layer(funcs->data, lodidx,0);
		funcs->pivot(funcs->data, pivot );

		unsigned int	surfidx = 0;
		for (surfidx = 0; surfidx < lod.SurfaceCount(); surfidx ++)
		{
			MDS_Surface& surf = lod.Surface(surfidx);

			// Map pointindex to LWPntID (points + bone points)
			LWPntID	*PointIDs = new LWPntID[surf.VertexCount()];

			// Make sure we have a surface
			funcs->surface(funcs->data,
							surf.Name(),(const char *)0,	// name, parent name
							20,(void *)surfacechunk);		// size of chunk, chunk

			// Add points ...
			unsigned int	vidx = 0;
			for (vidx = 0; vidx < surf.VertexCount(); vidx ++)
			{
				MDS_Vertex&	p = surf.Vertex(vidx);

				// Position using the first weight
				MDS_Weight& weight0 = p.Weight(0);
				Vector<float> res = BoneDirs[weight0.boneIndex] 
											* weight0.offset;
				res = res + BoneSpots[weight0.boneIndex];
				res = ~LW_TO_MDS_Coords * res;

				LWFVector		ptBuf;
				ptBuf[0] = res[0];
				ptBuf[1] = res[1];
				ptBuf[2] = res[2];

				PointIDs[vidx] = funcs->point(funcs->data,ptBuf);

				// Add a per-vertex UV map
				float	vbuf[2];
				vbuf[0] = p.uv.tex[0];
				vbuf[1] = 1.0 - p.uv.tex[1];		// why is this upside down?

				funcs->vmap(funcs->data,
							LWVMAP_TXUV,2,	// type, dimension
							surf.Name());	// Name
				funcs->vmapVal(funcs->data,PointIDs[vidx],vbuf);

				// Add in weight maps too
				for (unsigned int widx = 0; widx < p.WeightCount(); widx++)
				{
					MDS_Weight& weight = p.Weight(widx);
					float	wbuf[1];
					wbuf[0] = weight.boneWeight;
					
					funcs->vmap(funcs->data,
								LWVMAP_WGHT,1,	// type, dimension
								model.Bone(weight.boneIndex).Name);	// Name
					funcs->vmapVal(funcs->data,PointIDs[vidx],wbuf);
				}
			}

			// Add triangles ...
			unsigned int pidx = 0;
			for (pidx = 0; pidx < surf.PolygonCount(); pidx ++)
			{
				MDS_Poly& poly = surf.Polygon(pidx);

				LWPntID 	ptBuf[3];

				// Triangles have wedge index points
				ptBuf[0] = PointIDs[poly.vind[0]];
				ptBuf[1] = PointIDs[poly.vind[1]];
				ptBuf[2] = PointIDs[poly.vind[2]];

				LWPolID polyid = funcs->polygon(funcs->data,
												LWPOLTYPE_FACE,
												0, 3,
												ptBuf);

				// Hook the poly to the surface
				funcs->polTag(funcs->data, polyid,
								LWPTAG_SURF,	// type
								surf.Name());

			}
			if (PointIDs) delete PointIDs;
		}
	}

	delete BoneDirs;	BoneDirs = 0;
	delete BoneSpots;	BoneSpots = 0;
}

extern GUIData *GetDataFromUser(MDS& model);

int Load_MDS(LWObjectImport *funcs)
{
	MDS		model(funcs->filename);

	// Do Panel stuff
	GUIData *UserInput =  GetDataFromUser(model);
	if (!UserInput)
	{
		return AFUNC_BADAPP;
	}

	Build(funcs, model, *UserInput);

	// Everything worked OK
	funcs->result = LWOBJIM_OK;
	return AFUNC_OK;
}


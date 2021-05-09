extern "C"
{
#include "lwmeshes.h"
#include "lwpanel.h"
#include "lw_base.h"
#include "LoadQuakeMD.h"
}
#include <stdio.h>
#include <string.h>
#include "ModelMD3.h"

/* MD3 points are in a 3DS coordinate system
 * - the polys are flipped (point order is reversed)
 * - the coordiantes are rotated:
 *		+X is front			[1  0  0]
 *		+Y is left			[0  1  0]
 *		+Z is top			[0  0  1]
 *   By the same token, lightwave likes
 *		+X is right			[0 -1  0]
 *		+Y is top			[0  0  1]
 *		+Z is front			[1  0  0]
 */

// Use these to effect all the vertex operations

// Rotate by inverse vector and convert to incoming object's coordinate system
void BuildFuncs::MoveVecbyOffsets(float *v)
{
	Vector<float>	result;
	Vector<float>	V(v[0],v[1],v[2]);

	// un-rotate things around the incoming model's anchor (if necesary)
	if (_RotationTag)
	{
		V = V - _RotationTag->Position;
		result = ~(_RotationTag->Rotation) * V;

//		SubtractVector(V,RotationTag->Position,V);
//		RotateByINVMatrix(V,RotationTag->Rotation,result);
	}
	else
	{
		result = V;
//		CopyVector(V,result);
	}

	// Rotate around the internal anchor (if necesary)
	if (_AnchorTag)
	{
		V = _AnchorTag->Rotation * result;
		result = V + _AnchorTag->Position;
//		RotateByMatrix(result,AnchorTag->Rotation,V);
//		AddVector(V,AnchorTag->Position,result);
	}

	// Then rotate things to lightwave's coordinate system before returning
	V = ~LW_TO_MD3_Coords * result;
//	RotateByINVMatrix(result,LW_TO_MD3_Coords,V);

	for (int i = 0; i < 3; i ++)
		v[i] = V[i];
}

int BuildFuncs::BuildMD3(MD3& mdl)
{
	_PointCount = mdl.PointCount() + (mdl.TagCount() * 3);
	_PointIDs = new LWPntID[_PointCount +1];
	
	int		i = 0, j = 0, k = 0;

	// Setup rotation/translation stuff
	if (mdl.TagCount() > 0 && _Data->AnchorTagIndex > 0 )
		_RotationTag = &(mdl.TagsAtFrame(_Data->FrameForImport)[_Data->AnchorTagIndex -1]);
	else
		_RotationTag = (MD3_Tag *)NULL;

	// ----------- Points
	// Add the mesh(es) points in (may be more than one)
	for (i = 0;
		 i < mdl.MeshCount();
		 i ++)
	{	// Add the Skin vertex points in if we need to (same order as the others)
		k = (int)(mdl.Mesh(i).PointCount());
		for (j = 0; j < k; j ++)
		{	// Add the real mesh points first, THEN the skin points
			addPoint(mdl.Mesh(i).PointsAtFrame(_Data->FrameForImport)[j],0);
		}
	}

	// Then add all the extra tag points in 
	if (mdl.TagCount() > 0)
	{
		j = mdl.TagCount();
		for (i = 0; i < j; i ++)
		{
			addTagPoints(mdl.TagsAtFrame(_Data->FrameForImport)[i],0);
		}
	}

	// ----------- Skin UV
	// Add the skin values in
	ResetPointCounter();
	for (i = 0;
		 i < mdl.MeshCount();
		 i ++)
	{	// Add the Skin vertex points in if we need to (same order as the others)
		k = (int)(mdl.Mesh(i).PointCount());
		for (j = 0; j < k; j ++)
		{	// Add the real mesh points first, THEN the skin points
			addSkinPoint(mdl.Mesh(i).SkinPoint(j),mdl.Mesh(i).Name());
		}
	}

	// Now link up the polys - real polys first
	ResetPointCounter();
	int	vertoffset = 0;
	k = (int)(mdl.MeshCount());
	for (i = 0;	 i < k; i++)
	{
		int	srfIdx = addSurf((char *)mdl.Mesh(i).Name(), SURFACE_DEFAULT);
		UINT32 l = mdl.Mesh(i).PolyCount();
		
		for (j = 0;j < l; j++)
			addPoly(mdl.Mesh(i).Poly(j),vertoffset,srfIdx);

		vertoffset += mdl.Mesh(i).PointCount();
	}

	// And then the tag polys
	if (mdl.TagCount() > 0)
	{
		k = (int)(mdl.TagCount());
		for (i = 0;	 i < k; i++)
		{
			int			srfIdx = addSurf(mdl.TagsAtFrame(0)[i].Name, SURFACE_TAG);

			MD3_Poly	p;
			p.vind[0] = 0;
			p.vind[1] = 1;
			p.vind[2] = 2;

			addPoly(p,vertoffset,srfIdx);
			vertoffset += 3;
		}
	}

	// Start adding the morph targets
	if (_Data->AnimCFG[0] == 0
		|| _Data->ModelType == 0)
	{
		return AFUNC_OK;
	}

	// Open un animation.cfg, and read until we start finding Frames
	FILE	*anim = fopen(_Data->AnimCFG,"rt");
	char	fbuf[8192] = {0};
	char	*buf = fgets(fbuf,8192,anim);

	// Read until we find a line starting with a number.
	// 0	29	0	20		// BOTH_DEATH1 

	int skipped = 0;
	int maxfridx = 0;

#define TOKEN_STRING	"\t\n\r"

	int frsetidx = 0;
	while (buf = fgets(fbuf,8192,anim))
	{
		// See if this starts with a number
		if (buf[0] < '0' || buf[0] > '9')
		{
			continue;
		}

		// Get the next 'spaces' things
		char *start = buf;
		while (*buf != '\t' && *buf != ' ')	buf++;
		while (*buf == '\t' || *buf == ' ')	*buf++ = 0;

		char *len = buf;
		while (*buf != '\t' && *buf != ' ')	buf++;
		while (*buf == '\t' || *buf == ' ')	*buf++ = 0;

		char *loop = buf;
		while (*buf != '\t' && *buf != ' ')	buf++;
		while (*buf == '\t' || *buf == ' ')	*buf++ = 0;

		char *fps = buf;
		while (*buf != '\t' && *buf != ' ')	buf++;
		while (*buf == '\t' || *buf == ' ')	*buf++ = 0;

		// Name, trim the ends off
		char *name = buf;
		if (name[2] = ' ')
		{
			while (*buf != '\t' && *buf != ' ')	buf++;
			while (*buf == '\t' || *buf == ' ')	*buf++ = 0;

			name = buf;
		}
		while (*buf != '\t' && *buf != ' '
				&& *buf != '\n' && *buf != '\r') buf++;
		*buf = 0;

		int count = atoi(len);
		int fridx = atoi(start);

		// Now that we have it, do we want it ?
		if ((_Data->ModelType == 1 && 
			(name[0] == 'L' || name[0] == 'l'))
			||
			(_Data->ModelType == 2 && 
			(name[0] == 'T' || name[0] == 't')) )
		{
			int iskip = fridx + count - maxfridx;
			skipped += iskip;
			if (count + fridx > maxfridx)
				maxfridx = count + fridx;
			continue;
		}

		// Find our real starting point
		fridx -= skipped;

		// Get the frame data
		++frsetidx;
		for (int idx = 0; idx < count; idx++, fridx++)
		{
			int vtxidx = 0;
			char group[64] = {0};
			sprintf(group,"%s.Frame_%03d",name,idx);
			ResetPointCounter();

			// Add in all the points for all meshes
			for (i = 0;
				 i < mdl.MeshCount();
				 i ++)
			{
				k = (int)(mdl.Mesh(i).PointCount());
				for (j = 0; j < k; j++)
				{	// These are MORPH (delta) changes
					MD3_Point_Frame dPoint;
					MD3_Point_Frame pnt = mdl.Mesh(i).PointsAtFrame(fridx)[j];
					MD3_Point_Frame base = mdl.Mesh(i).PointsAtFrame(_Data->FrameForImport)[j];

					dPoint.v[0] = pnt.v[0] - base.v[0];
					dPoint.v[1] = pnt.v[1] - base.v[1];
					dPoint.v[2] = pnt.v[2] - base.v[2];

					// Grab the proper group
					addPoint(dPoint ,group);
				}
			}

			// ...And for all the tags
			if (mdl.TagCount() > 0)
			{
				k = mdl.TagCount();
				for (j = 0; j < k; j ++)
				{
					MD3_Tag dTag;
					MD3_Tag& tag = mdl.TagsAtFrame(fridx)[j];
					MD3_Tag& base = mdl.TagsAtFrame(_Data->FrameForImport)[j];

					dTag.Rotation = tag.Rotation;
					dTag.Position = mdl.TagsAtFrame(fridx)[j].Position 
									- mdl.TagsAtFrame(_Data->FrameForImport)[j].Position;

					addTagPoints(dTag,group);
				}
			}
		}
		if (fridx > maxfridx)
			maxfridx = fridx;
	}

	// Reset offset structures
	return AFUNC_OK;
}

//serv_w.obj server.lib libcmtd.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib shell32.lib oldnames.lib winmm.lib comctl32.lib ole32.lib uuid.lib urlmon.lib
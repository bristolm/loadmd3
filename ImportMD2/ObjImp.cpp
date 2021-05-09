/**************************************
 *
 *  ObjImp.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Handles the real meat of the program for
 *  importing Quake2 MD2 format files
 *
 **************************************/

#include "sys_math.h"

extern "C"
{
#include "ImportMD2.h"
}

#include "ParseMD2.h"

static unsigned char surfacechunk[20] =
{
	'C','O','L','R',	// 4 letter chunk tag
		0,14,			// Size of the chunk
		0x3F,0x00,0x00,0x00,	// Red   (float across 4 bytes)
		0x3F,0x00,0x00,0x00,	// Green (float across 4 bytes)
		0x3F,0x00,0x00,0x00,	// Blue  (float across 4 bytes)
		0,0				// Attached envelope Vidx (none)
};

void Build(LWObjectImport *funcs, MD2& model)
{
	LWFVector		pivot = {0.0f};
	int				pointcount = model.PointCount();
	unsigned int	idx = 0;

	// Map pointindex to LWPntID
	LWPntID	*PointIDs = new LWPntID[pointcount];

	// 1 surface and UV map
	char *SurfaceName = "MD2";

	funcs->surface(funcs->data,
					SurfaceName,(const char *)0,	// name, parent name
					20,(void *)surfacechunk);		// size of chunk, chunk

	// ===Now build the object inside Lightwave===
	funcs->layer(funcs->data, 0,0);
	funcs->pivot(funcs->data, pivot );

	// Get frame to load
	GUIData *data = Get_FrameNum(model.FrameCount() -1);
	int fridx = data->frame;

	// Add points ...
	MD2_Frame& fra = model.Frame(fridx);
	MD2_FramePoints& basepnts = model.PointsAtFrame(fridx);
	for (idx = 0; idx < pointcount; idx ++)
	{
		MD2_Point_Frame& pnt = basepnts[idx];

		Vector<float>	V(pnt.v);
		Vector<float> res = ~LW_TO_MD2_Coords * V;

		LWFVector		ptBuf;
		ptBuf[0] = res[0];
		ptBuf[1] = res[1];
		ptBuf[2] = res[2];
		
		PointIDs[idx] = funcs->point(funcs->data,ptBuf);
	}

	// Then add polys ...
	for (idx = 0; idx < model.PolyCount(); idx ++)
	{
		MD2_Poly& poly = model.Poly(idx);

		LWPntID 	ptBuf[3];

		// Triangles have wedge index points
		ptBuf[0] = PointIDs[poly.vertex[0]];
		ptBuf[1] = PointIDs[poly.vertex[1]];
		ptBuf[2] = PointIDs[poly.vertex[2]];

		LWPolID polyid = funcs->polygon(funcs->data,
										LWPOLTYPE_FACE,
										0, 3,
										ptBuf);
		funcs->polTag(funcs->data, polyid,
						LWPTAG_SURF,	// type
						SurfaceName);

		// Add in the VMAPs on a per-poly basis based on the point
		for (int j = 0; j < 3; j++)
		{
			MD2_Point_Skin& skpnt = model.SkinPoint(poly.texvec[j]);
			float	vbuf[2];
			vbuf[0] = skpnt.tex[0];
			vbuf[1] = 1.0f - skpnt.tex[1];

			funcs->vmap(funcs->data,
						LWVMAP_TXUV,2,	// type, dimension
						SurfaceName);
			funcs->vmapPDV(funcs->data,ptBuf[j],polyid,vbuf);
		}
	}

	if (data->makemorphs == 0)
	{
		if (PointIDs) delete PointIDs;
		return;
	}

#define FR_DIV		"Frame_"
	// Now try and setup some morphmaps
	for (fridx = 0; fridx < model.FrameCount(); fridx++)
	{
		// get the name first
		char group[128] = {0};
		char *p = &(model.Frame(fridx).Creator[0]);
		char *n = &(group[0]);
		while (*p)
		{
			if (*p >= '0' && *p <= '9')
			{	// Number, consider it the 'break'
				*n++ = '.';
				sprintf(n,FR_DIV);
				n += strlen(FR_DIV);
				break;
			}
			*n++ = *p++;
		}

		// Want at least 3 digits ...
		char *tmp = p;
		int iZeros = 3;
		while (*tmp++)			iZeros--;
		while (iZeros-- > 0)	*n++ = '0';

		// now the rest
		while (*p)				*n++ = *p++;
		
		// Terminate it
		*n = 0;

		// now work from there - adding each frame in
		MD2_FramePoints& frpnts = model.PointsAtFrame(fridx);
		for (idx = 0; idx < pointcount; idx ++)
		{
			MD2_Point_Frame& pnt = frpnts[idx];
			Vector<float>	V(pnt.v);

			// MORF is a 'delta' (SPOT is absolute) - find base position
			MD2_Point_Frame& pntbase = basepnts[idx];
			Vector<float>	Vbase(pntbase.v);

			Vector<float>	res = (~LW_TO_MD2_Coords * V)
									- (~LW_TO_MD2_Coords * Vbase);
			LWFVector		ptBuf;
			ptBuf[0] = res[0];
			ptBuf[1] = res[1];
			ptBuf[2] = res[2];

			// Select the VMAP
			funcs->vmap(funcs->data,
						LWVMAP_MORF,3,	// type, dimension
						group);

			// Add the point's position
			funcs->vmapVal(funcs->data,
						PointIDs[idx],
						ptBuf);
		}
	}

	if (PointIDs) delete PointIDs;
}

int Load_MD2(LWObjectImport *funcs)
{
	MD2		model(funcs->filename);
	Build(funcs, model);

	// Everything worked OK
	funcs->failedLen = 0;
	funcs->result = LWOBJIM_OK;
	return AFUNC_OK;
}


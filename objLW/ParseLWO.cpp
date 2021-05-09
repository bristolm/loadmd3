/**************************************
 *
 *  parselwo.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Wrapper of NewTek's LWO file format
 *
 *  LWO file format property of NewTek, Inc.
 *
 **************************************/

#include <stdio.h>
#include <stdlib.h>

#include "ParseLWO.h"
#include "string.h"
#include "math.h"

using namespace LW3D;

// Verts are 3 4byte floats
//
void LWOv1::ParseVerts(IFF::Chunk& b)
{	// <count> sets of 3x4 bytes (X,Y,Z)
	UINT32 vertcount = b.Size() / 12;

	float *p = (float *)(b.Data());
	UINT32 i = 0;

	// REPLACE
	//Verts.LockMax(vertcount);
	for (i = 0; i < vertcount; i ++)
	{
		Vertex& v = Verts[i];
		v.setIndex(i);
#if defined(WIN32) || defined(DJGPP)
		Swap4((unsigned char *)p);
		Swap4((unsigned char *)(p +1));
		Swap4((unsigned char *)(p +2));
#endif
		
		v.pos[0] = *p++;
		v.pos[1] = *p++;
		v.pos[2] = *p++;
	}
}

// Polys have a 2 byte uint count
//    count 2byte uint vertex indeces
//    1 2byte int for the surface index (negative means detail polygons)
//
void LWOv1::ParsePolys(IFF::Chunk& b)
{	// should be <polycount> sets of (1 + 3 + 1) * 2 bytes 
	//		(count, points, surface index)
	UINT32 polycount = b.Size() / 10;
	UINT16 *p = (UINT16 *)(b.Data());

	// REPLACE
	//Polys.LockMax(polycount);

	UINT32 i = 0;

	for (i = 0; i < polycount; i++)
	{	// Get point count for this poly
		Polygon& poly = Polys[i];
		poly.index = i;

#if defined(WIN32) || defined(DJGPP)
		Swap2((unsigned char *)p);
#endif

		int j = *p++ , k = 0;
		if (j != 3)		// We'd LOVE 3 point polys only
			fprintf(stderr,"WARNING: Poly %d (of $d) has %d points%s\n",
						i,polycount,j,
						(j < 3) ? " [IGNORING]" : "");

		if (j > 2)	// ignore 2 point polys
		{
			for (k = 0; k < 3; k++, p++, j--)
			{	// now get the point indeces for it

#if defined(WIN32) || defined(DJGPP)
				Swap2((unsigned char *)p);
#endif
				poly.pntindices[k] = *p;
			}
		}

		while (j-- > 0)
			p++;

		// Now add this to its surface - surfaces in the file are 1,2 ... or -1,-2 ...
		// step their indices down by one to make things consistant
#if defined(WIN32) || defined(DJGPP)
		Swap2((unsigned char *)p);
#endif
		Surface &surf = Surfs[*(INT16 *)p++ -1];
		surf.addPoly(poly);
	}
}

void LWOv1::ParseSurfs(IFF::Chunk& b)
{	// need to count ...
	UINT32			sz = b.Size();
	unsigned char	*p = b.Data();
	int		surfcount = 0;
	while (sz > 0)
	{
		Surfs[surfcount].index = surfcount;
		strcpy(Surfs[surfcount++].name,(const char *)p);


		int i = strlen((const char *)p) +1;
		if (i & 1)
			i ++;

		sz -= i;
		p += i;
	}
}

LWOv1::LWOv1(const char *filename): LWO(filename)
{
	LWO::ParsefromFile();
}

void LWOv1::ProcessNextChunkfromFile(IFF::Chunk& work)
{
	UINT32 i = work.Type();

	if (i == *(UINT32 *)LW_CHNK_PNT)
		ParseVerts(work);
	else if (i == *(UINT32 *)LW_CHNK_POLY)
		ParsePolys(work);
	else if (i == *(UINT32 *)LW_CHNK_SRFS)
		ParseSurfs(work);
}

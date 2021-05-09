/**************************************
 *
 *  parselwo2.cpp
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Wrapper of NewTek's LWO2 (LW 6+) file format
 *
 *  LWO2 file format property of NewTek, Inc.
 *
 **************************************/

#include <stdio.h>
#include <stdlib.h>

#include "ParseLWO2.h"
#include "string.h"
#include "math.h"

using namespace LW3D;

// LWO2 uses a variable length index field alot - it works like this:
// numbers below 65,280 are 2 byte, numbers above are 4 byte.  So,
// if the first byte is 0xFF, read as 4 bytes.
UINT32 LW3D::ParseVIndex(UINT16 **p)
{

#if defined(WIN32) || defined(DJGPP)
		Swap2((unsigned char *)*p);
#endif

	UINT32 i = 0;
	if (~*((unsigned char *)*p) == 0)
	{		// 4 bytes
		i |= **p & 0x00FF;
		i <<= 16;
		++(*p);

#if defined(WIN32) || defined(DJGPP)
		Swap2((unsigned char *)*p);
#endif
	}

	i |= **p;
	++(*p);

	return i;
}

// Verts are 3 4byte floats
//
void LWOv2::ParseVerts(IFF::Chunk& b)
{	// <vertcount> sets of 3x4 bytes (X,Y,Z)
	UINT32 vertcount = b.Size() / 12;

	float *p = (float *)(b.Data());
	UINT32 i = 0;

//	Verts.LockMax(vertcount);
	for (i = 0; i < vertcount; i ++)
	{
		Vertex& v = Verts[i];
		v.setIndex(i);
#ifdef WIN32
		Swap4((unsigned char *)p);
		Swap4((unsigned char *)(p +1));
		Swap4((unsigned char *)(p +2));
#endif
		
		v.pos[0] = *p++;
		v.pos[1] = *p++;
		v.pos[2] = *p++;
	}
}

// LWO2 poly structures are different than LWO
// 4 char 'type' string
// 2 byte point count string - 0xFC00 is a type | 0x03FF is the count
// List of 'count' variable length indexes

void LWOv2::ParsePolys(IFF::Chunk& b)
{
	const char *c = (const char *)(b.Data());
	if (strncmp(c,LW_CHNK_FACE,4) != 0)
	{
		fprintf(stderr,"...Ignoring polygon chunk '%c%c%c%c'\n",
					*c, *(c+1), *(c+2), *(c+3));
					
		return;
	}

	UINT16 *p = (UINT16 *)(b.Data() +4);

	// We're going to assume 3 points each again (1 * 2bytes count + 3 * 2bytes)
	UINT32 polycount = b.Size();
	polycount -= 4;
	polycount /= 8;

	UINT32 i = 0, u = 0;

	for (i = 0; i < polycount; i++)
	{	// Get point count for this poly
		Polygon& poly = Polys[i];
		poly.index = i;

#if defined(WIN32) || defined(DJGPP)
		Swap2((unsigned char *)p);
#endif

		int j = *p++ & 0x3FF, k = 0;
		if (j != 3)		// We'd LOVE 3 point polys only
			fprintf(stderr,"WARNING: Poly %d (of %d) has %d points%s\n"
						,i,polycount,j,(j < 3) ? " [IGNORING]" : "");

		if (j > 2)	// ignore 2 point polys
		{
			for (k = 0; k < 3; k++, j--)
			{	// now get the point indices for it
				poly.pntindices[k] = ParseVIndex(&p);
			}
		}

		while (j-- > 0)
			(void)ParseVIndex(&p);

		// Surface indices are not here in LWO2 files

		// See if this polygon is in any VMaps - if all the points are in the
		// VMap,  then it uses it.
		auto_vector<Surface>::iterator uv = VMaps.begin();
		for (u = 0;uv != VMaps.end(); ++uv, ++u)
		{
			for (k = 0; k < 3; k++)
			{
				if (Verts[poly.pntindices[k]].vmaps.is_valid(u) == 0)
					break;
			}

			if (k == 3)
				uv->addPoly(poly);
		}
	}
}

/* VMaps are an object-level thing.  They can span surfaces, so we 
 * force the application to tell us which one to look in - or we'll 
 * give it whatever we find first
 */
void LWOv2::ParseVmap(IFF::Chunk& b)
{	// Surface map points
	const char *c = (const char *)(b.Data());
/*
	fprintf(stderr,"...Vmap chunk '%c%c%c%c'\n",
				*c, *(c+1), *(c+2), *(c+3));
*/
	if (*(UINT32 *)c != *(UINT32 *)LW_CHNK_TXUV)		// TXUV tag
		return;
	c += 4;

#if defined(WIN32) || defined(DJGPP)
	Swap2((unsigned char *)c);
#endif

	if (*(UINT16 *)c != 2)								// dimensions per point
		return;
	c += 2;					// 'c' is now at the start of the VMap name string


	UINT16		*start= (UINT16 *)(c + (strlen(c) & ~1) + 2); 
	UINT16		*end = (UINT16 *)(b.Data() + b.Size());

	int vmapcount = UVMapCount();
	Surface& vmap = getVMap(vmapcount);
	strcpy(vmap.name,c);
	vmap.index = vmapcount;

/*
	fprintf(stderr,"   - Mapped %d points in '%s'\n",
						UVMaps[i]->MappedPointCount(),
						UVMaps[i]->Name());
 */

	// now we repeat ( vert[VX], value[F4] # dimension )*  until we're done
	UINT32		abs_vertidx = 0, u = 0, i = 0;
	while (start < end)
	{	// get the vertex index
		abs_vertidx = ParseVIndex(&start);
		Vertex& vert = getVertex(abs_vertidx);
		//if (!(vert.vmaps[vmapcount] == LWO_vert_uvmap::INVALID))
		//	fprintf(stderr,"--- repeat UV coord for vertex index %d\n",abs_vertidx);

		// get the point values (careful of alignment)
		for (i = 0; i < 2; i ++)
		{
#if defined(WIN32) || defined(DJGPP)
			Swap2((unsigned char *)(start));
#endif
			u = *start++;
#if defined(WIN32) || defined(DJGPP)
			Swap2((unsigned char *)(start));
#endif
			u <<= 0x10;
			u |= *start++;

			vert.vmaps[vmapcount].index = vmapcount;
			vert.vmaps[vmapcount].uv[i] = *(float*)&u;
		}
	}
}

/* A VMAD Chunk is a discontinuous poly chunk.  It can span surfaces,
 * and it basically gives yet another UV point to a given vertex.
 * I need to get the vertex index, copy it, assign the UV point, and 
 * then re-focus the specified polygon's vertex on that new point instead
 * Trick is that the chunks are not necessarily in order ... for now I'll
 * assume they are - i.e. we have the Vertexes, and the Polygons before
 * these show up
 *
 * VMAD { type[ID4], dimension[U2], name[S0],
		( vert[VX], poly[VX], value[F4] # dimension )* } 
 */
void LWOv2::ParsePVmap(IFF::Chunk& b)
{	// Surface map points
	const char *c = (const char *)(b.Data());
/*
	fprintf(stderr,"...Vmap chunk '%c%c%c%c'\n",
				*c, *(c+1), *(c+2), *(c+3));
*/
	if (*(UINT32 *)c != *(UINT32 *)LW_CHNK_TXUV)		// TXUV VMAD chunk
		return;
	c += 4;

#if defined(WIN32) || defined(DJGPP)
	Swap2((unsigned char *)c);
#endif

	if (*(UINT16 *)c != 2)								// dimensions per point
		return;
	c += 2;					// 'c' is now at the start of the VMad name string


	UINT16		*start= (UINT16 *)(c + (strlen(c) & ~1) + 2); 
	UINT16		*end = (UINT16 *)(b.Data() + b.Size());

	// See if there is a map of this name already
	unsigned int vmapcount = 0;
	auto_vector<Surface>::iterator uv = VMaps.begin();
	for (; uv!= VMaps.end(); ++uv, ++vmapcount)
	{
		if (strcmp(uv->name,c) == 0)
			break;
	}

	// Ignore stuff we can't find
	if (vmapcount > UVMapCount())
		return;

	Surface& vmap = getVMap(vmapcount);
	strcpy(vmap.name,c);
	vmap.index = vmapcount;

/*
	fprintf(stderr,"   - Mapped %d points in '%s'\n",
						UVMaps[i]->MappedPointCount(),
						UVMaps[i]->Name());
 */

	// now we repeat ( vert[VX], poly[VX], value[F4] # dimension )*  until we're done
	UINT32		u = 0, i = 0;
	while (start < end)
	{	// get the point/polygon indices (careful of alignment)
		UINT32 abs_vertidx = ParseVIndex(&start);
		UINT32 abs_polyidx = ParseVIndex(&start);

		UV  tmpUV;
		for (i = 0; i < 2; i ++)
		{
#if defined(WIN32) || defined(DJGPP)
			Swap2((unsigned char *)(start));
#endif
			u = *start++;
#if defined(WIN32) || defined(DJGPP)
			Swap2((unsigned char *)(start));
#endif
			u <<= 0x10;
			u |= *start++;

			tmpUV.index = vmapcount;
			tmpUV.uv[i] = *(float*)&u;
		}

		// Compare against the current values for this vertex
		UINT32 new_vertidx = INVALID_VERTEX_IDX;
		unsigned int xidx = 0;

		Vertex oVert = getVertex(abs_vertidx);
		for (; xidx < oVert.otherindexes.size(); xidx++)
		{
			Vertex& vert = getVertex(oVert.otherindexes[xidx]);
			UV& vertVmap = vert.vmaps[vmapcount];
			if (vertVmap.index == vmapcount &&
				vertVmap.uv[0] == tmpUV.uv[0] &&
				vertVmap.uv[1] == tmpUV.uv[1])
			{	// Same, leave things as they are ...
				new_vertidx = vert.getIndex();
			}
		}

		if (new_vertidx == abs_vertidx)
			return;

		if (new_vertidx == INVALID_VERTEX_IDX)
		{
			new_vertidx = VertexCount();
			oVert.addNewPseudoIndex(new_vertidx);
		}

		// Different, find the new vertex, and refocus the polygon
		Vertex& newVert = getVertex(new_vertidx);
		newVert.setIndex(new_vertidx);
		newVert.vmaps[vmapcount].index = vmapcount;
		for (i = 0; i < 3; i ++)
		{
			newVert.pos[i] = oVert.pos[i];
			newVert.vmaps[vmapcount].uv[i] = tmpUV.uv[i];
		}

		Polygon& poly = getPolygon(abs_polyidx);
		for (i = 0; i < 3; i ++)
		{	// If we're alredy at the enw one, quit
			if (poly.pntindices[i] == new_vertidx)
				break;

			// If this isn't the old one, try again
			if (poly.pntindices[i] != abs_vertidx)
				continue;

			// Refocus the polygon, and quit
			poly.pntindices[i] = new_vertidx;
			break;
		}
	}
}

void LWOv2::ParseTag(IFF::Chunk& b)
{	// This is where the surface names are listed for a 6.0 object
	UINT32			sz = b.Size();
	unsigned char	*p = b.Data();
	int		j = 0;
	while (sz > 0)
	{
		Surface& surf = getSurface(j++);
		surf.index = j;
		strcpy(surf.name,(const char *)p);

		int i = strlen((const char *)p) +1;
		if (i & 1)
			i ++;

		sz -= i;
		p += i;

	}
}

// This is the polygon <-> surface mapping portion of a 6.0 object
// [U4 type] {Variable length polygon index, index into TAG list} *

void LWOv2::ParsePTag(IFF::Chunk& b)
{
	UINT16		*p = (UINT16 *)(b.Data());
	UINT16		*end = p + (b.Size() / 2);

/*	fprintf(stderr,"...PTag chunk '%c%c%c%c'\n",
				*c, *(c+1), *(c+2), *(c+3));
*/
	if (*(UINT32 *)p != *(UINT32 *)LW_CHNK_SURF)
		return;

	p += 2;

	INT16		polyidx = 0;
	while (p < end)
	{
		polyidx = (INT16)ParseVIndex(&p);
#if defined(WIN32) || defined(DJGPP)
		Swap2((unsigned char *)p);
#endif
		Surfs[(*p++)].addPoly(Polys[polyidx]);
	}
}

LWOv2::LWOv2(const char *filename): LWO(filename)
{
	LWO::ParsefromFile();
}

void LWOv2::ProcessNextChunkfromFile(IFF::Chunk& work)
{
	UINT32 i = work.Type();

	if (i == *(UINT32 *)LW_CHNK_PNT)
		ParseVerts(work);
	else if (i == *(UINT32 *)LW_CHNK_TAG)
		ParseTag(work);
	else if (i == *(UINT32 *)LW_CHNK_POLY)
		ParsePolys(work);
	else if (i == *(UINT32 *)LW_CHNK_VMAP)
		ParseVmap(work);
	else if (i == *(UINT32 *)LW_CHNK_PTAG)
		ParsePTag(work);
	else if (i == *(UINT32 *)LW_CHNK_VMAD)
		ParsePVmap(work);
}

// Look for the first reference to the UV coordinate of the point
UV&	LWOv2::VertexUVmap(UINT32 vertidx)
{
	Vertex& v = getVertex(vertidx);

	auto_vector<UV>::iterator uv = v.vmaps.begin();
	for (; uv != v.vmaps.end(); ++uv)
	{
		if (v.vmaps.is_valid(*uv))
			return *uv;
	}

	return invalidUV;
}

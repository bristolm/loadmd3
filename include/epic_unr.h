#include "sys_math.h"
#include "sys_extra.h"

#ifndef _EPIC_UNREAL_H
#define _EPIC_UNREAL_H

/*
 * 2 types of output files for Unreal
 * - A data file
 * - An Animation file
 *
 * Thanks for PantherD of Team Panther for assembling this
 * stuff in an easy-to-read package
 */

/*
 * DATA file - named ..._d.3d
 * ( Apparently a lot of these are not necessary ... )
 *
 * Just a header, then a series of Polygon blocks
 */
typedef struct
{
	UINT16			Triangle_num;
	UINT16			Vertex_num;
	UINT16			_rot;
	UINT16			_frame;
	UINT32			_normX;
	UINT32			_normY;
	UINT32			_normZ;
	UINT32			FixScale;
	UINT32			_unused[3];
	UINT8			_unknown[12];
} UNRDATA_Header;

typedef struct dMD2TexVec
{
	UINT8		uv[2];				// 0 - 256 - scale up from 0.0 ... 1.0
} UNRDATA_Skin_Vert;



// Bitflags describing effects for a (classic Unreal & Unreal Tournament) mesh triangle.
// Code taken from Vertebrate.h file (c) Epic MegaGames

enum EJSMeshTriType
{
	// Triangle types. Pick ONE AND ONLY ONE of these.
	MTT_Normal				= 0x00,	// Normal one-sided.
	MTT_NormalTwoSided      = 0x01,    // Normal but two-sided.
	MTT_Translucent			= 0x02,	// Translucent two-sided.
	MTT_Masked				= 0x03,	// Masked two-sided.
	MTT_Modulate			= 0x04,	// Modulation blended two-sided.
	MTT_Placeholder			= 0x08,	// Placeholder triangle for positioning weapon. Invisible.

	// Bit flags. Add any of these you want.
	MTT_Unlit				= 0x10,	// Full brightness, no lighting.
	MTT_Flat				= 0x20,	// Flat surface, don't do bMeshCurvy thing.
	MTT_Alpha				= 0x20,	// This material has per-pixel alpha.
	MTT_Environment			= 0x40,	// Environment mapped.
	MTT_NoSmooth			= 0x80,	// No bilinear filtering on this poly's texture.
};

typedef struct
{
	UINT16				vind[3];
	char				type;		// See UNRPOLY_* values above
	char				color;
	UNRDATA_Skin_Vert	vtxuv[3];
	char				texnum;
	char				flags;
} stUNRDATA_Poly;

/*
 * ANIM file - named ..._a.3d
 *
 * Just a headerw with a few things, then a series of Vertex positions
 * for each frame
 */
typedef struct
{
	UINT16			Frame_num;		// minimum of 1
	UINT16			Frame_size;		// Num vertices * sizeof(UINT32)
} UNRANIM_Header;

typedef struct
{
	UINT32			packedvalue;
} stUNRANIM_Vertex ;

typedef struct
{
	float			v[3];
} stUNRANIM_Work_Vertex;

// Thanks PantherD !!
#define PACK_UNREAL_VERTEX(x,y,z)					\
				 ((int)(x * 8.0) & 0x7ff) |			\
				(((int)(y * 8.0) & 0x7ff) << 11) |	\
				(((int)(z * 4.0) & 0x3ff) << 22)

#endif // _EPIC_UNREAL_H


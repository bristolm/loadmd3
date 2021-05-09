/*
 * Original code Copyright (C) 1999 by Ritual Entertainment, Inc. 
 * Provided as a portion of the FAKK2 toolkit
 *
 ******************************************
 * Wrapper of id software's MD4 file format
 *  Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  MD4 file format property of id software
 *
 **************************************/


#include "sys_math.h"
#include "sys_extra.h"

using namespace MRB;

#ifndef _ID_MD4_H_
#define _ID_MD4_H_

//#pragma pack(1)     // better pack these structures!

#define ALIAS_VERSION2	8
#define ALIAS_VERSION3	15

#define ID_HEADER_MD4		'4PDI'	// big endian: 'IDP4'
#define	MAX_QPATH			64
#define MD4_VERSION			1
#define	MD4_MAX_BONES		128

// ------------------------- md4 file structures -------------------------------
typedef struct
{
	float	tex[2];
} MD4_Vert_Skin;

typedef struct {
	INT32			boneIndex;		// these are indexes into the boneReferences,
	float			boneWeight;		// not the global per-frame bone list
	Vector<float>	offset;
} stMD4_Weight;

typedef struct {
    Vector<float>   v;
//	Vector<float>	normal;
	MD4_Vert_Skin	uv;
	INT32			Weight_num;
	stMD4_Weight	Weights[1];		// [Weights_num]
} stMD4_Vertex;

typedef struct {
	INT32			vind[3];		// Vertex indices
} stMD4_Poly;

typedef INT32 MD4_BoneRef;

typedef struct {
	int			ID;					// ID_HEADER_MD4

	char		Name[MAX_QPATH];	// polyset name
	char		shader[MAX_QPATH];
	INT32		shaderIndex;		// for in-game use

	INT32		Header_Start;		// (negative number)

	INT32		Vert_num;
	INT32		Verts_Start;

	INT32		Triangle_num;
	INT32		Triangles_Start;

	// Bone references are a set of ints representing all the bones
	// present in any vertex weights for this surface.  This is
	// needed because a model may have surfaces that need to be
	// drawn at different sort times, and we don't want to have
	// to re-interpolate all the bones for each surface.
	INT32			BoneReferences_num;
	INT32			BoneReferences_Start;

	INT32			ChunkSize;			// Size of this surface
} MD4_SurfaceHeader;

typedef struct {
	INT32			Surface_num;
	INT32			Surfaces_Start;		// first surface, others follow
	INT32			SurfacesChunkSize;	// Size of this LOD block's surfaces (me + all my surfaces)
} stMD4_LOD;

typedef struct {
	float		matrix[3][4];
} stMD4_Bone;

typedef struct {
	Vector<float>	Max;		// Bounds of one corner
	Vector<float>	Min;		// Bounds of opposite corner
	Vector<float>	Mid;		// midpoint of bounds
	float			Radius;		// dist from localOrigin to corner
	char			Name[16];
	stMD4_Bone		Bones[1];			// [Bones_num]
} stMD4_Frame;

typedef struct {
	INT32			ID;					// ID_HEADER_MD4
	INT32			Version;			// MD4_VERSION

	char			Name[MAX_QPATH];	// model name

	// frames and bones are shared by all levels of detail
	INT32			Frame_num;
	INT32			Bone_num;
	INT32			Frames_Start;		// stMD4_Frame[Frames_num]

	// each level of detail has completely separate sets of surfaces
	INT32			LOD_num;
	INT32			LOD_Start;

	INT32			FileSize;			// end of file
	// [Frames] ( Frames_num * (stMD4_Frame + ((Bone_num - 1) * stMD4_Bone)))
	// [LODs]    (stMD4_LOD + ... )
	//           [Surfaces]    (stMD4_Surface + ... )
	//			               [Verts]  (stMD4_Vertex + ...]
	//									[Weights] (stMD4_Weight * Weight_num)
	//						   [Polys]  (stMD4_Poly * Poly_num)
	//						   [Bone Refs] (INT32 * BoneReferences_num)

} MD4_Header;

#endif // _ID_MD4_H_

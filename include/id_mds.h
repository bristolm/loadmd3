/*
 ******************************************
 * Wrapper of id software's MDS file format
 *  Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  MDS file format property of id software
 *  This was adapted from the MD4 format released with FAKK2 
 *  - code Copyright (C) 1999 by Ritual Entertainment, Inc
 *    Provided as a portion of the FAKK2 toolkit
 **************************************/


#include "sys_math.h"
#include "sys_extra.h"

using namespace MRB;

#ifndef _ID_MDS_H_
#define _ID_MDS_H_

//#pragma pack(1)     // better pack these structures!

#define ALIAS_VERSION2	8
#define ALIAS_VERSION3	15

#define ID_HEADER_MDS		'4PDI'	// big endian: 'IDP4'
#define	MAX_QPATH			64
#define MDS_VERSION			4
#define	MDS_MAX_BONES		128

// ------------------------- MDS file structures -------------------------------
typedef struct
{
	float	tex[2];
} MDS_Vert_Skin;

typedef struct {
	INT32			boneIndex;		// these are indexes into the boneReferences,
	float			boneWeight;		// not the global per-frame bone list
	Vector<float>	offset;
} stMDS_Weight;

typedef struct {
	Vector<float>	normal;
	MDS_Vert_Skin	uv;
	INT32			Weight_num;
	INT32			Dummy1;
	INT32			Dummy2;
} stMDS_Vertex;

typedef struct {
	INT32			vind[3];		// Vertex indices
} stMDS_Poly;

typedef INT32 MDS_BoneRef;

typedef struct {
	int			ID;					// ID_HEADER_MDS

	char		Name[MAX_QPATH];	// polyset name
	char		shader[MAX_QPATH];
	INT32		shaderIndex;		// for in-game use

	INT32		Dummy1;				// 170 ?
	INT32		Header_start;		// Back to the file's Header (negative)

	INT32		Vert_num;
	INT32		Vert_start;

	INT32		Triangle_num;
	INT32		Triangles_start;

	INT32		Dummy2;

	// Bone references are a set of ints representing all the bones
	// present in any vertex weights for this surface.  This is
	// needed because a model may have surfaces that need to be
	// drawn at different sort times, and we don't want to have
	// to re-interpolate all the bones for each surface.
	INT32			BoneReferences_num;
	INT32			BoneReferences_start;

	INT32			ChunkSize;			// Size of this surface
} MDS_SurfaceHeader;

typedef struct {
	INT32			Surface_num;
	INT32			Surfaces_start;		// first surface, others follow
	INT32			SurfacesChunkSize;	// Size of this LOD block's surfaces (me + all my surfaces)
} stMDS_LOD;

// 80 bytes
typedef struct {
	char			Name[MAX_QPATH];	// Bone name
	INT32			ParentBoneIndex;	// -1 if root
	INT32			Dummy1;
	INT32			Dummy2;
	INT32			Dummy3;
} stMDS_Bone;

// 12 bytes
typedef struct {
	Vector<short>	Position;		// /64 on input
	Vector<short>	Orientation;
} stMDS_BoneFrame;

// 52 bytes
typedef struct {
	Vector<float>	Min;		// Bounds of one corner
	Vector<float>	Max;		// Bounds of opposite corner
	Vector<float>	Mid;		// midpoint of bounds
	float			Radius;		// dist from local Origin to corner
	float			Dummy[3];
} stMDS_Frame;

// I believe this just advertises which bones to call 'tags' ... in the old sense
typedef struct {
	char			Name[MAX_QPATH];	// model name
	float			Dummy1;
	INT32			BoneIndex;
} stMDS_Tag;

typedef struct {
	INT32			ID;					// ID_HEADER_MDS
	INT32			Version;			// MDS_VERSION

	char			Name[MAX_QPATH];	// model name

	INT32			Dunno1;				// Look like bits - the number is 1111111000...
	INT32			Dunno2;				// 0 - could be an extension of the other one

	// frames and bones are shared by all levels of detail
	INT32			Frame_num;
	INT32			Bone_num;
	INT32			Frames_start;		// stMDS_Frame[Frames_num]
	INT32			Bone_start;			// big number, proobably an offset

	// each level of detail has completely separate sets of surfaces
	INT32			LOD_num;			// LOD maybe?   Just 1 in these files
	INT32			Surface_num;
	INT32			LOD_start;

	INT32			Tag_num;		// end of file
	INT32			Tag_start;		// end of file

	INT32			FileSize;		// end of file

	// [Frames] ( Frames_num * (stMDS_Frame + ((Bone_num - 1) * stMDS_BoneFrame)))
	// [Bones]
	// [LODs]    (stMDS_LOD + ... )
	//           [Surfaces]    (stMDS_Surface + ... )
	//			               [Verts]  (stMDS_Vertex + ...]
	//									[Weights] (stMDS_Weight * Weight_num)
	//						   [Polys]  (stMDS_Poly * Poly_num)
	//						   [Bone Refs] (INT32 * BoneReferences_num)
      // [Tags]  (stMDS_Tag * Tag_num)

} MDS_Header;

#endif // _ID_MDS_H_

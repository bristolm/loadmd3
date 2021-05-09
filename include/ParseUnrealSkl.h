/**************************************
 *
 *  parseunrealskel.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Import/Export object for Epic MegaGames, Inc.
 *  Unreal Tounament Skeletal file format
 *
 *  Unreal Tounament is a trademark of Epic MegaGames, Inc.
 *
 **************************************/

#ifndef  _PARSE_UNREALSKL_H_
#define  _PARSE_UNREALSKL_H_

#include "string.h"
#include "stdio.h"

#include "sys_math.h"
#include "sys_extra.h"
#include "epic_base.h"

using namespace MRB;

// Setup so the next header file loads ok
typedef		Vector<float>		FVector;
typedef		_WORD				WORD;

#define     TArray				AutoArray;
#define		MAX_PATH			1024

#include "epic_skl.h"

// some flag stuff
typedef struct
{
	char	*Name;
	int		BitFlag;
} stExtraFlags;

#define MAX_UT_MATERIAL_CHUNKS		8
#define MAX_EXCLUSIVE_MAT_CHUNK		4
static stExtraFlags UT_NameChunks [MAX_UT_MATERIAL_CHUNKS] = {
	{"twosided",MTT_NormalTwoSided},
	{"trans",MTT_Translucent},
	{"mask",MTT_Masked},
	{"modul",MTT_Modulate},

	{"unlit",MTT_Unlit},
	{"envir",MTT_Environment},
	{"mirro",MTT_Environment},
	{"nosmo",MTT_NoSmooth}
};

#define MAX_EXTRA_UV_FLAGS		5
static stExtraFlags UV_ExtraFlags [MAX_EXTRA_UV_FLAGS] = {
	{"FLAG_unlit",MTT_Unlit},
	{"FLAG_flat",MTT_Flat},
	{"FLAG_alpha",MTT_Alpha},
	{"FLAG_env",MTT_Environment},
	{"FLAG_nofilter",MTT_NoSmooth}
};

// coordinate stuff
extern Matrix<float>	LW_TO_UNSKEL_Coords;

typedef		VChunkHdr			UNSKEL_VChunkHdr;
typedef		VMaterial			stUNSKEL_VMaterial;
typedef		VJointPos			stUNSKEL_VJointPos;
typedef		FNamedBoneBinary	stUNSKEL_FNamedBoneBinary;
typedef		AnimInfoBinary		stUNSKEL_AnimInfoBinary;
typedef		VQuatAnimKey		stUNSKEL_VQuatAnimKey;
typedef		VRawBoneInfluence 	stUNSKEL_VRawBoneInfluence ;
typedef		VPoint				stUNSKEL_VPoint;
typedef		VVertex				stUNSKEL_VVertex;
typedef		VTriangle			stUNSKEL_VTriangle;

WRAP_STRUCT(UNSKEL_VPoint)
};
WRAP_STRUCT(UNSKEL_VVertex)
};
WRAP_STRUCT(UNSKEL_VTriangle)
};
WRAP_STRUCT(UNSKEL_VMaterial)
};
WRAP_STRUCT(UNSKEL_VRawBoneInfluence)
};

WRAP_STRUCT(UNSKEL_FNamedBoneBinary)
};
typedef stUNSKEL_AnimInfoBinary UNSKEL_AnimInfoBinary;
WRAP_STRUCT(UNSKEL_VQuatAnimKey)
};

class UNSKEL_Animation : public UNSKEL_AnimInfoBinary
{
public:
	AutoArray<UNSKEL_VQuatAnimKey>		KeyFrames;

	UNSKEL_Animation() :
	  KeyFrames(UNSKEL_VQuatAnimKey::INVALID)
	{
	  Name[0] = 0;
	  Group[0] = 0;
	}

	inline UINT32 KeyFrameCount()
	{	return KeyFrames.Next();		}
	UNSKEL_VQuatAnimKey&	KeyFrame(UINT32 kidx)
	{	return (KeyFrames[kidx]);		}

	static UNSKEL_Animation INVALID;
};

class UnrealSkeletalModel
{
private:
	char	BaseFile[512];

	// SKL (Skeltal) file members
	AutoArray<UNSKEL_VPoint>			Points;		// Array of Points
	AutoArray<UNSKEL_VVertex>			Wedges;		// Array of UV Vertex data
	AutoArray<UNSKEL_VTriangle>			Triangles;	// Array of Triangles
	AutoArray<UNSKEL_VMaterial>			Materials;	// Array of Materials
	AutoArray<UNSKEL_VRawBoneInfluence>	Weights;	// Array of Point Weightmaps

	// Shared by both
	AutoArray<UNSKEL_FNamedBoneBinary>	Bones;		// Array of bones and bone setup info

	// SKA (Animation) file members
	AutoArray<UNSKEL_Animation>			Animations;		// Array of different animation sequences
public:

	UnrealSkeletalModel();						// Empty creation
	UnrealSkeletalModel(const char *filename);	// populate from file

	// Query functions
	inline char *getName()
	{return BaseFile;}

	inline UINT32 FrameCount()
	{return Animations.Next();}

	inline const UINT32 PointCount()
	{ return Points.Next();}

	inline const UINT32 TriangleCount()
	{ return Triangles.Next();}

	inline const UINT32 WedgeCount()
	{ return Wedges.Next();}

	inline const UINT32 MaterialCount()
	{ return Materials.Next();}

	inline const UINT32 WeightCount()
	{ return Weights.Next();}

	inline const UINT32 BoneCount()
	{ return Bones.Next();}

	inline const UINT32 AnimationCount()
	{ return Animations.Next();}

	// Entity retrieval
	inline UNSKEL_VPoint&		Point(UINT32 pidx)
	{	return (Points[pidx]);		}

	inline UNSKEL_VVertex&		Wedge(UINT32 pidx)
	{	return (Wedges[pidx]);		}

	inline UNSKEL_VTriangle&	Triangle(UINT32 pidx)
	{	return (Triangles[pidx]);		}

	inline UNSKEL_VMaterial&	Material(UINT32 midx)
	{	return (Materials[midx]);		}

	inline UNSKEL_VRawBoneInfluence& Weight(UINT32 widx)
	{	return (Weights[widx]); }

	inline UNSKEL_FNamedBoneBinary& Bone(UINT32 bidx)
	{	return (Bones[bidx]); }

	inline UNSKEL_Animation& Animation(UINT32 aidx)
	{	return (Animations[aidx]); }

	// Output
	void WritetoDisk(const char *basefile = 0);
	void WriteSkeletontoDisk(const char *basefile = 0);
	void WriteAnimationtoDisk(const char *basefile = 0);

	// Input
	int Parse(const char *filename);
	int ParseAnimation(const char *filename);
	int ParseSkeleton(const char *filename);
};

#endif //_PARSE_UNREALSKL_H_
/**************************************
 *
 *  parseunreal.h
 *
 *  Import/Export object for Epic MegaGames, Inc.
 *  Unreal file format
 *
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Unreal is a trademark of Epic MegaGames, Inc.
 *
 **************************************/


#ifndef  _PARSE_UNREAL_H_
#define  _PARSE_UNREAL_H_

#include "string.h"
#include "stdio.h"
#include "epic_unr.h"

WRAP_STRUCT(UNRDATA_Poly)			};
WRAP_STRUCT(UNRANIM_Vertex)			};
WRAP_STRUCT(UNRANIM_Work_Vertex)	};

// Just some aliasing for blocks
typedef Block<UNRANIM_Vertex>			UNRANIM_FramePoints;
typedef Block<UNRANIM_Work_Vertex>		UNRANIM_Work_FramePoints;

class UnrealModel
{
private:
	char	BaseFile[512];

	// DATA file members
	UNRDATA_Header				DataHeader;
	AutoArray<UNRDATA_Poly>		Polys;			// Array of Polygons

	// ANIM file members
	UNRANIM_Header				AnimHeader;
	AutoArray<UNRANIM_Work_FramePoints>	WorkPoints;	// Array of 'real' pre-packed points
	AutoArray<UNRANIM_FramePoints>		FramePoints;// Array of points for each from - [#Frames][#Points]

public:

	UnrealModel();						// Empty creation
	UnrealModel(const char *filename);	// populate from file

	// Query functions
	inline UINT32 FrameCount()
	{return AnimHeader.Frame_num;}

	inline const UINT32 PointCount()
	{ return DataHeader.Vertex_num;}

	inline const UINT32 PolyCount()
	{ return DataHeader.Triangle_num;}

	void UpdateHeaderSizes();

	// Entity retrieval
	inline UNRDATA_Poly&		Poly(UINT32 pidx)
	{	return (Polys[pidx]);		}

	inline UNRANIM_Work_FramePoints& PointsAtFrame(UINT32 FrameNum)
	{	return (WorkPoints[FrameNum]); }

	// Output
	void WritetoDisk(const char *basefile = 0);		// Open and dump out data to a stream
};

#endif //_PARSE_UNREAL_H_
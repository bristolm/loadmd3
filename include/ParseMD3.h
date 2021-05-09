#ifndef  _PARSE_MD3_H_
#define  _PARSE_MD3_H_
/**************************************
 *
 *  parsemd3.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Import/Export object for id software's MD3 file format
 *
 *  MD3 file format property of id software
 *
 **************************************/

#include "string.h"
#include "stdio.h"
#include "id_md3.h"
#include "ParseMDCommon.h"

#define TAG_POLY_LEN	4
#define TAG_POLY_WIDTH	2

WRAP_STRUCT(MD3_Vert_Skin)		};
WRAP_STRUCT(MD3_Tag)			};
WRAP_STRUCT(MD3_Frame)			};
WRAP_STRUCT(MD3_Skin)			};
WRAP_STRUCT(MD3_Poly)			};
WRAP_STRUCT(MD3_Point_Frame)
	void setNormal(Vector<float> &v);
};

extern	Matrix<float>	LW_TO_MD3_Coords;

// Just some aliasing
typedef Block<MD3_Point_Frame>		MD3_FramePoints;
typedef Block<MD3_Tag>				MD3_FrameTags;

class Prep_PointsforTag
{
public:
	double		    nrml[3];				// Normal of tag poly
	Vector<float>	pos[3];					// Positions of the 3 points from Lightwave

	void CalculateNormal();			// Use this to figure normal from points
	void FillTag(MD3_Tag *t);		// Convert to a MD3_Tag
	void HackFillTag(MD3_Tag *t);	// temporary - to just figure something out 
};

class  MD3_Mesh 
{
	MD3_MeshHeader	Header;

	AutoArray<MD3_Poly>				Polys;			// Array of Polygons
	AutoArray<MD3_Skin>				Skins;			// Array of Skins
	AutoArray<MD3_FramePoints>		FramePoints;	// Array of points for each from - [#Frames][#Points]
	AutoArray<MD3_Vert_Skin>		SkinPoints;		// Array of Skin vertices

	MD3_Mesh(FILE *fp):
		Polys(MD3_Poly::INVALID),
		Skins(MD3_Skin::INVALID),
		FramePoints(MD3_FramePoints::INVALID),
		SkinPoints(MD3_Vert_Skin::INVALID)
	{
		Parse(fp);
	}

public:
	static	MD3_Mesh	INVALID;

	// Assembly/creation functions
	MD3_Mesh(const char *name = 0);

	~MD3_Mesh()		{;}

	int Parse(FILE *fp);

	// Query functions
	inline const UINT32 FrameCount()	{return FramePoints.Next();}
	inline const UINT32 SkinCount()		{return Skins.Next();}
	inline const UINT32 PolyCount()		{return Polys.Next();}

	inline const UINT32 PointCount()
	{ return SkinPoints.Next();}

	inline const UINT32 SkinPointCount()
	{ return SkinPoints.Next();}

	inline const UINT32 MeshSize()
	{
		UpdateHeaderSizes();
		return Header.ChunkSize;
	}

	inline char *Name()		{ return (char *)Header.Name;}
	void UpdateName(const char *c);

	// clearing functions
	inline void ClearSkins()
	{
		Skins.Clear();

		Header.Skin_num = 0;
		UpdateHeaderSizes();
	}

	// Retrieval functions
	MD3_Poly&			Poly(UINT32 idx)			{return Polys[idx]; }
	MD3_Skin&			Skin(UINT32 idx)			{return Skins[idx]; }
	MD3_Vert_Skin&		SkinPoint(UINT32 idx)		{return SkinPoints[idx]; }

	inline MD3_FramePoints& PointsAtFrame(UINT32 FrameNum)
	{												return FramePoints[FrameNum]; }

	// Building functions
	UINT32 UpdateHeaderSizes();

	// Adding functions
	void AddSkin(const char *name);

	// Output functions
	void WritetoDisk(FILE *fp);				// Dump out data to a stream
};

class MD3
{
private:
	FILE		*fp;

	MD3_Header	Header;
	AutoArray<MD3_Frame>		Frames;	// Array of pointers to Frames
	AutoArray<MD3_Mesh>			Meshes;	// Array of pointers to Meshes
	AutoArray<MD3_FrameTags>	Tags;	// Array of pointers to points for each from - [#Frames][#Points]

public:
	char	FileName[1024];			// Path and everything

	MD3();									// Empty creation
	MD3(const char *filename);				// populate from file
	~MD3()	{;}

	// Query functions
	const char *File()  {return FileName;}
	int isLoaded();
	char *TypeName(char *);
	int	 MeshNames(char **);

	inline UINT32 Type()
	{return Header.ID;}

	inline const char *ModelName()	{return Header.Name;}
	void UpdateName(const char *c);

	inline UINT32 FrameCount()		{return Frames.Next();}
	inline UINT32 MeshCount()		{return Meshes.Next();}
	inline UINT32 TagCount()
	{	return Tags.Next() == 0 ? 0 : Tags[0].Next();	}

	UINT32 PointCount();
	UINT32 PolyCount();

	// Entity retrieval
	inline MD3_Frame&		Frame(UINT32 fidx)
	{	return (Frames[fidx]);		}

	inline MD3_Mesh&		Mesh(UINT32 meshidx)
	{	return (Meshes[meshidx]);	}

	inline MD3_FrameTags&	TagsAtFrame(UINT32 FrameNum)
	{	return Tags[FrameNum];		}

	// Output
	UINT32 UpdateHeaderSizes();
	void WritetoDisk(const char *filename = 0);		// Open and dump out data to a stream
};


#endif	//_PARSE_MD3_H_
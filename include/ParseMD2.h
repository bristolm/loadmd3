/**************************************
 *
 *  parsemd2.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Import/Export object for id software's MD2 file format
 *
 *  MD2 file format property of id software
 *
 **************************************/

#ifndef  _PARSE_MD2_H_
#define  _PARSE_MD2_H_

#include "string.h"
#include "stdio.h"
#include "id_md2.h"
#include "ParseMDCommon.h"

using namespace MRB;

#define TAG_POLY_LEN	4
#define TAG_POLY_WIDTH	2

#define LW_X		2
#define LW_Y		0
#define LW_Z		1

#define ID_X		0
#define ID_Y		1
#define ID_Z		2

extern	Matrix<float>	LW_TO_MD2_Coords;

typedef		MD2Header				MD2_Header;
typedef		MD2Frame				stMD2_Frame;
typedef		MD2Skin					stMD2_Skin;
typedef		MD2Poly					stMD2_Poly;

// Wrap the real MD2Point_Frame so we
// can use double values externally
typedef	struct
{
	MD2Point_Frame	innards;
	float			v[3];
} stMD2_Point_Frame;

// Wrap the real MD2Point_Skin so we can use
// 0.0 --> 1.0 UV values externally
typedef	struct
{
	MD2Point_Skin	innards;
	float			tex[2];
} stMD2_Point_Skin;


WRAP_STRUCT(MD2_Point_Skin)		};
WRAP_STRUCT(MD2_Frame)			};
WRAP_STRUCT(MD2_Skin)			};
WRAP_STRUCT(MD2_Poly)			};

// Thsi guy gets and extra function
WRAP_STRUCT(MD2_Point_Frame)
	void setNormal(Vector<float> &v);
};

typedef		MD2StripItem			MD2_StripItem;

typedef struct
{
	int		strip_xyz[128];
	int		strip_st[128];
	int		strip_tris[128];
	int		used[MAX_MD2TRIANGLES];
} GL_Work;


// Just some aliasing
typedef Block<MD2_Point_Frame>		MD2_FramePoints;

class MD2
{
private:
	FILE		*fp;

	MD2_Header	Header;

	// Just one mesh ...
	AutoArray<MD2_Frame>		Frames;			// Array of Frames
	AutoArray<MD2_Poly>			Polys;			// Array of Polygons
	AutoArray<MD2_Skin>			Skins;			// Array of Skins
	AutoArray<MD2_FramePoints>	FramePoints;	// Array of points for each frame - [#Frames][#Points]
	AutoArray<MD2_Point_Skin>	SkinPoints;		// Array of Skin vertices

	AutoArray<MD2_StripItem>	GL_Strips;		// Array of GL commands
	void BuildGLStrips(void);
	int	StripLength (GL_Work& work, int starttri, int startv);
	int	FanLength (GL_Work& work, int starttri, int startv);

public:
	const char	*FileName;

	MD2();						// Empty creation
	MD2(const char *filename);	// populate from file

	// Query functions
	int isLoaded();
	char *TypeName(char *);

	inline UINT32 Type()
	{return Header.ID;}

	inline const char *ModelName()
	{return FileName;}
	
	inline UINT32 FrameCount()
	{return FramePoints.Next();}

	inline UINT32 MeshCount()
	{return isLoaded() ? 1 : 0;}

	inline const UINT32 SkinCount()
	{ return Skins.Next();}

	inline const UINT32 PointCount()
	{ return FramePoints[0].Next();}

	inline const UINT32 SkinPointCount()
	{ return SkinPoints.Next();}

	inline const UINT32 PolyCount()
	{ return Polys.Next();}

	inline const UINT32 Skinheight()
	{ return Header.Skin_height;}
	inline void Skinheight(UINT32 i)
	{ Header.Skin_height = i;}

	inline const UINT32 Skinwidth()
	{ return Header.Skin_width;}
	inline void Skinwidth(UINT32 i)
	{ Header.Skin_width = i;}

	UINT32 UpdateHeaderSizes();

	// Entity retrieval
	inline MD2_Frame& Frame(UINT32 fidx)
	{	return (Frames[fidx]);		}

	inline MD2_Poly&		Poly(UINT32 pidx)
	{	return (Polys[pidx]);		}

	inline MD2_Skin&		Skin(UINT32 skidx)
	{	return (Skins[skidx]);	}

	inline MD2_Point_Skin&	SkinPoint(UINT32 skidx)
	{	return (SkinPoints[skidx]);		}

	inline MD2_FramePoints& PointsAtFrame(UINT32 FrameNum)
	{	return (FramePoints[FrameNum]); }

	// Output
	void WritetoDisk(const char *filename = 0);		// Open and dump out data to a stream
};


#endif // _PARSE_MD2_H_
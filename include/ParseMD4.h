/**************************************
 *
 *  parsemd4.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Import/Export object for id software's MD4 file format
 *
 *  MD4 file format property of id software
 *
 **************************************/

#ifndef  _PARSE_MD4_H_
#define  _PARSE_MD4_H_

#include "string.h"
#include "stdio.h"
#include "id_md4.h"
#include "ParseMDCommon.h"

WRAP_STRUCT(MD4_Bone)		};
WRAP_STRUCT(MD4_Poly)		};
WRAP_STRUCT(MD4_Weight)		};

class MD4_Vertex : public stMD4_Vertex
{
	AutoArray<MD4_Weight>	Weights;
public:
	MD4_Vertex():
		Weights(MD4_Weight::INVALID)
	{;}

	inline UINT32	WeightCount()		{return Weights.Next();}
	MD4_Weight&		Weight(UINT32 idx)		{return Weights[idx]; }

	INT32 size()	{ return sizeof(stMD4_Vertex) 
										+ (sizeof(stMD4_Weight) * (WeightCount() -1) ); }

	static MD4_Vertex INVALID;
	static int sizeofBase() {return sizeof(stMD4_Vertex);}
};

class MD4_Surface
{
	MD4_SurfaceHeader			Header;
	AutoArray<MD4_Vertex>		Vertices;
	AutoArray<MD4_Poly>			Polygons;
	AutoArray<MD4_BoneRef>		BoneReferences;

public:
	MD4_Surface();

	inline UINT32	VertexCount()		{return Vertices.Next();}
	MD4_Vertex&		Vertex(UINT32 idx)	{return Vertices[idx]; }

	inline UINT32	PolygonCount()		{return Polygons.Next();}
	MD4_Poly&		Polygon(UINT32 idx)	{return Polygons[idx]; }

	inline UINT32	BoneRefCount()		{return BoneReferences.Next();}
	INT32&			BoneRef(UINT32 idx)	{return BoneReferences[idx]; }

	static MD4_Surface INVALID;
	static int sizeofBase() {return sizeof(MD4_SurfaceHeader);}

	void Name(char *);
	inline const char *Name()
	{ return Header.Name;}

	// Input/Output functions
	INT32 UpdateHeaderSizes(int offset);
	int Parse(FILE *fp);
	INT32 WritetoDisk(FILE *fp);				// Dump out data to a stream
};

class MD4_LOD
{
	AutoArray<MD4_Surface>		Surfaces;	// Array of Bones

public:
	stMD4_LOD					Header;

	MD4_LOD():
		Surfaces(MD4_Surface::INVALID)
	{	memset(&Header,0,sizeof(stMD4_LOD));}


	inline UINT32	SurfaceCount()		{return Surfaces.Next();}
	MD4_Surface&	Surface(UINT32 idx)	{return Surfaces[idx]; }

	static MD4_LOD INVALID;
	static int sizeofBase() {return sizeof(stMD4_LOD);}
};

class MD4_Frame : public stMD4_Frame
{
	AutoArray<MD4_Bone>	Bones;	// Array of Bones

public:
	MD4_Frame():
		Bones(MD4_Bone::INVALID)
	{	memset(this,0,sizeof(stMD4_Frame));}

	inline UINT32	BoneCount()			{return Bones.Next();}
	MD4_Bone&		Bone(UINT32 idx)	{return Bones[idx]; }

	static MD4_Frame INVALID;
	static int sizeofBase() {return sizeof(stMD4_Frame);}
};

class MD4
{
private:
	FILE		*fp;

	MD4_Header	Header;
	AutoArray<MD4_Frame>		Frames;	// Array of Frames
	AutoArray<MD4_LOD>			LODs;	// Array of LODs

public:
	char	FileName[1024];				// Path and sverything

	MD4();									// Empty creation
	MD4(const char *filename);				// populate from file
	~MD4() {;}

	// Query functions
	const char *File()  {return FileName;}
	int isLoaded();
	char *TypeName(char *);

	inline UINT32 Type()
	{return Header.ID;}

	inline const char *ModelName()
	{return Header.Name;}
	void UpdateName(const char *c);


	// Entity retrieval
	inline UINT32		FrameCount()		{return Frames.Next();}
	inline MD4_Frame&	Frame(UINT32 fidx)	{return (Frames[fidx]);	}

	inline UINT32		LODCount()			{return LODs.Next();}
	inline MD4_LOD&		LOD(UINT32 meshidx) {return (LODs[meshidx]);	}

	// Input/Output
	INT32 UpdateHeaderSizes();
	void WritetoDisk(const char *filename = 0);		// Open and dump out data to a stream
};



#endif // _PARSE_MD4_H_

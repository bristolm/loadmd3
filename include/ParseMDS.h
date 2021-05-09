/**************************************
 *
 *  parsemds.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Import/Export object for id software's MDS file format
 *
 *  MDS file format property of id software
 *  MDS is from the Return to Castle Wolfenstein release
 *
 **************************************/

#ifndef  _PARSE_MDS_H_
#define  _PARSE_MDS_H_

#include "string.h"
#include "stdio.h"
#include "id_mds.h"
#include "ParseMDCommon.h"

extern Matrix<float>	LW_TO_MDS_Coords;

WRAP_STRUCT(MDS_Bone)		};
WRAP_STRUCT(MDS_Poly)		};
WRAP_STRUCT(MDS_Weight)		};
WRAP_STRUCT(MDS_Tag)		};

class MDS_Vertex : public stMDS_Vertex
{
	AutoArray<MDS_Weight>	Weights;
public:
	MDS_Vertex():
		Weights(MDS_Weight::INVALID)
	{;}

	inline UINT32	WeightCount()		{return Weights.Next();}
	MDS_Weight&		Weight(UINT32 idx)		{return Weights[idx]; }

	INT32 size()	{ return sizeof(stMDS_Vertex) 
										+ (sizeof(stMDS_Weight) * (WeightCount() -1) ); }

	static MDS_Vertex INVALID;
	static int sizeofBase() {return sizeof(stMDS_Vertex);}
};

class MDS_Surface
{
	MDS_SurfaceHeader			Header;
	AutoArray<MDS_Vertex>		Vertices;
	AutoArray<MDS_Poly>			Polygons;
	AutoArray<MDS_BoneRef>		BoneReferences;

public:
	MDS_Surface();

	inline UINT32	VertexCount()		{return Vertices.Next();}
	MDS_Vertex&		Vertex(UINT32 idx)	{return Vertices[idx]; }

	inline UINT32	PolygonCount()		{return Polygons.Next();}
	MDS_Poly&		Polygon(UINT32 idx)	{return Polygons[idx]; }

	inline UINT32	BoneRefCount()		{return BoneReferences.Next();}
	INT32&			BoneRef(UINT32 idx)	{return BoneReferences[idx]; }

	static MDS_Surface INVALID;
	static int sizeofBase() {return sizeof(MDS_SurfaceHeader);}

	void Name(char *);
	inline const char *Name()
	{ return Header.Name;}

	// Input/Output functions
	INT32 UpdateHeaderSizes(int offset);
	int Parse(FILE *fp);
	INT32 WritetoDisk(FILE *fp);				// Dump out data to a stream
};

class MDS_LOD
{
	AutoArray<MDS_Surface>		Surfaces;	// Array of Bones

public:
	stMDS_LOD					Header;

	MDS_LOD():
		Surfaces(MDS_Surface::INVALID)
	{	memset(&Header,0,sizeof(stMDS_LOD));}


	inline UINT32	SurfaceCount()		{return Surfaces.Next();}
	MDS_Surface&	Surface(UINT32 idx)	{return Surfaces[idx]; }

	static MDS_LOD INVALID;
	static int sizeofBase() {return sizeof(stMDS_LOD);}
};

class MDS_BoneFrame
{

public:
	Vector<float>	Position;
	Vector<float>	Orientation;

	MDS_BoneFrame():
	  Position(Vector<float>()),
	  Orientation(Vector<float>())
	{;}

	MDS_BoneFrame(stMDS_BoneFrame& in)
	{
		for (int i = 0; i < 3; i++)
		{
			Position[i] = ((float)(in.Position[i])) / 64.0f;
			Orientation[i] = ((float)(in.Orientation[i])) / 256.0f;
		}
	}

	const stMDS_BoneFrame& getInnards()
	{
		stMDS_BoneFrame innards;

		for (int i = 0; i < 3; i++)
		{
			innards.Orientation[i] = (short)(Position[0] * 64);
			innards.Position[i] = (short)(Orientation[0] * 256);
		}
	}

	static MDS_BoneFrame INVALID;
	static int sizeofBase() {return sizeof(stMDS_BoneFrame);}
};

class MDS_Frame : public stMDS_Frame
{
	AutoArray<MDS_BoneFrame>	Bones;	// Array of BonePositions

public:
	MDS_Frame():
		Bones(MDS_BoneFrame::INVALID)
	{	memset(this,0,sizeof(stMDS_Frame));}

	inline UINT32		BoneCount()			{return Bones.Next();}
	MDS_BoneFrame&		Bone(UINT32 idx)	{return Bones[idx]; }

	static MDS_Frame INVALID;
	static int sizeofBase() {return sizeof(stMDS_Frame);}
};

class MDS
{
private:
	FILE		*fp;

	MDS_Header	Header;
	AutoArray<MDS_Frame>		Frames;	// Array of Frames
	AutoArray<MDS_LOD>			LODs;	// Array of LODs
	AutoArray<MDS_Bone>			Bones;	// Array of Bone information
	AutoArray<MDS_Tag>			Tags;	// Array of Tag references

public:
	char	FileName[1024];				// Path and sverything

	MDS();									// Empty creation
	MDS(const char *filename);				// populate from file
	~MDS() {;}

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
	inline MDS_Frame&	Frame(UINT32 fidx)	{return (Frames[fidx]);	}

	inline UINT32		LODCount()			{return LODs.Next();}
	inline MDS_LOD&		LOD(UINT32 lidx)	 {return (LODs[lidx]);	}

	inline UINT32		BoneCount()			{return Bones.Next();}
	inline MDS_Bone&	Bone(UINT32 bidx)	{return (Bones[bidx]);	}

	inline UINT32		TagCount()			{return Tags.Next();}
	inline MDS_Tag&		Tag(UINT32 tidx)	{return (Tags[tidx]);	}

	// Input/Output
	INT32 UpdateHeaderSizes();
	void WritetoDisk(const char *filename = 0);		// Open and dump out data to a stream
};



#endif // _PARSE_MDS_H_

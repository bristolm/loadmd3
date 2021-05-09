/**************************************
 *
 *  parselwocommon.h
 *  Copyright (c) 2000,2001 Michael Bristol
 *  mbristol@bellatlantic.net
 *
 *  Common header for LWO-based file formats
 *
 *  LWO file format property of NewTek, Inc.
 *
 **************************************/

#ifndef  _PARSE_LWO_COMMON_H_
#define  _PARSE_LWO_COMMON_H_

#include "sys_math.h"
#include "sys_extra.h"

#include <vector>

using namespace MRB;

namespace IFF
{
	class Chunk
	{

	private:
		unsigned char	*buffer;
		UINT32			bufsize;
		UINT32			bufinc;

		UINT32			totalread;

	public:
		class ChunkFunction
		{
			const char	*name;
			void operator () (Chunk *);
		};

		Chunk(size_t szbuf);

		inline unsigned char *Data()
		{
			return buffer + 8;
		}

		inline UINT32 TotalDataRead()
		{
			return totalread;
		}

		inline UINT32 Size()
		{
			UINT32	u;
			unsigned char	*uc = (unsigned char	*)&u;

			uc[0] = *(buffer + 4);
			uc[1] = *(buffer + 5);
			uc[2] = *(buffer + 6);
			uc[3] = *(buffer + 7);

			return u;
		}

		inline UINT32 Type()
		{
			UINT32	u;
			unsigned char	*uc = (unsigned char	*)&u;

			uc[0] = *(buffer + 0);
			uc[1] = *(buffer + 1);
			uc[2] = *(buffer + 2);
			uc[3] = *(buffer + 3);

			return u;
		}

		UINT32 Read(FILE *fp, const ChunkFunction *f);	
		UINT32 getSome(unsigned char *buff, FILE *fp, UINT32 len);
	};
}

namespace LW3D
{
	static const UINT32 INVALID_VMAP_IDX	= ((UINT32)(-1));
	static const UINT32 INVALID_SURFACE_IDX	= ((UINT32)(-1));
	static const UINT32 INVALID_VERTEX_IDX	= ((UINT32)(-1));
	static const UINT32 INVALID_POLYGON_IDX	= ((UINT32)(-1));

	// Main IFF id
	static const char *IFF_FILE_ID	= "FORM";

	// Main LW chunks
	static const char *LW_CHNK_PNT	= "PNTS";
	static const char *LW_CHNK_POLY	= "POLS";
	static const char *LW_CHNK_SRFS	= "SRFS";
	static const char *LW_CHNK_SURF	= "SURF";

	// Main LW2 chunks
	static const char *LW_CHNK_TAG	= "TAGS";
	static const char *LW_CHNK_CLIP	= "CLIP";
	static const char *LW_CHNK_ENV	= "ENVL";
	static const char *LW_CHNK_LAY	= "LAYR";
	static const char *LW_CHNK_BOX	= "BBOX";
	static const char *LW_CHNK_PTAG	= "PTAG";
	static const char *LW_CHNK_VMAP	= "VMAP";
	static const char *LW_CHNK_TXUV	= "TXUV";
	static const char *LW_CHNK_VMAD	= "VMAD";

	class UV
	{	// really just use 2 coordinates
	public:
		UINT32			index;			// index of VMap I belong to
		Vector<float>	uv;

		UV(UINT32 idx = INVALID_VMAP_IDX):
			index(idx)
			{;}

		int operator == ( const UV& rhs) const
		{
			return (index == rhs.index);
		}
	};

	class Polygon
	{
	public:
		UINT32			index;			// my unique index
		Vector<UINT16>	pntindices;		// vertex indices in this polygon

		UINT32	getIndex()	{return index;}

		Polygon(UINT32 idx = INVALID_POLYGON_IDX):
			index(idx)
			{;}

		int operator == ( const Polygon& rhs) const
		{
			return (index == rhs.index);
		}
	};

	class Surface
	{
		auto_vector<UINT8>	polyindexes;	// polygons in this surface (indexed)
		auto_vector<UINT32>	polygons;		// polygons in this surface (ordered)

	public:
		UINT32				index;			// my unique index

		char				name[64];		// surface name

		Surface(UINT32 idx = INVALID_SURFACE_IDX):
			polyindexes(-1),
			polygons(-1),
			index(idx)
		{
			name[0] = 0;
		}

		inline UINT32 getPolygonIndex(UINT32 i)
		{
			return polyindexes[i];
		}

		inline UINT32  PolygonCount()
		{
			return polygons.size();
		}

		int operator == ( const Surface& rhs) const
		{
			return (index == rhs.index);
		}

		void addPoly(Polygon& p)
		{
			if (polyindexes[p.getIndex()] == 1)
				return;
			polyindexes[p.getIndex()] = 1;
			polygons.push_back(p.getIndex());
		}
	};

	class Vertex
	{
		UINT32					index;			// my unique index
	public:
		Vector<float>			pos;			// my location
		auto_vector<UV>			vmaps;			// List of VMaps for this vertex

		// VMAD causes a problem - we need a new vertex, but still need to tie 
		// to this one, we'll 'promote' this one with extra indexes
		// This should be done in some sort of LWO2_Vert, but I'm not in the mood ..
		auto_vector<UINT32>		otherindexes;	// For discontinous maps

		void	setIndex(UINT32 i = 0)
		{
			index = i;
			otherindexes[0] = i;
		}

		UINT32	getIndex(UINT32 i = 0)
		{
			return otherindexes[i];
		}

		void	addNewPseudoIndex(UINT32 i = 0)
		{
			otherindexes.push_back(i);
		}

		Vertex(int idx = INVALID_VERTEX_IDX):
			index(idx),
			vmaps(),
			otherindexes()
			{;}

		int operator == ( const Vertex& rhs) const
		{
			return (index == rhs.index);
		}
	};

	class  LWO
	{
	protected:
		UV		invalidUV;

		unsigned char	tmp[16];

		FILE		*fp;
		int			valid;

		IFF::Chunk	work;

		auto_vector<Vertex>		Verts;
		auto_vector<Polygon>	Polys;
		auto_vector<Surface>	Surfs;
		auto_vector<Surface>	VMaps;

		void ParsefromFile();
		virtual void ProcessNextChunkfromFile(IFF::Chunk& b) = 0;

		virtual const char *fileID() = 0;
		virtual const char *fileType() = 0;

	public:
		const char	*FileName;

		LWO(const char *filename);
		virtual ~LWO();

		inline const char *Name()
		{
			return FileName;
		}

		inline int isValid()
		{
			return (valid == 1);
		}

		inline UINT32 VertexCount()		{return Verts.size();}
		inline UINT32 PolyCount()		{return Polys.size();}
		inline UINT32 SurfaceCount()	{return Surfs.size();}
		inline UINT32 UVMapCount()		{return VMaps.size();}

		Vertex&		getVertex(UINT32 idx)
		{
			return Verts[idx];
		}
		Polygon&	getPolygon(UINT32 idx)
		{
			return Polys[idx];
		}
		Surface&	getSurface(UINT32 idx)
		{
			return Surfs[idx];
		}
		Surface&	getVMap(UINT32 idx)
		{
			return VMaps[idx];
		}

		void	GetPolyNormal(UINT32 polyidx, double d[]);			// fill in a polygon's normal

		virtual UV&		VertexUVmap(UINT32 vertidx)
		{
			return invalidUV;
		}

		const char *FileID()
		{
			return fileID();
		}
		const char *FileType()
		{
			return fileType();
		}
	};
}

#endif	//_PARSE_LWO_COMMON_H_
